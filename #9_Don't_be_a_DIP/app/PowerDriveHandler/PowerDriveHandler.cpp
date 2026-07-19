/**
 * MIT License
 *
 * @brief Implementation of PowerDriveHandler.
 *
 * @file PowerDriveHandler.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-09-12
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#include "PowerDriveHandler.h"

#include <AppDisplay.h>
#include <Clock.h>
#include <Debugging.h>
#include <Liveness.h>
#include <MathFns.h>

#include <cmath>

namespace
{
    /**
     * @brief Compact drive-owner token for debug output.
     *
     * @param owner Drive command owner.
     * @return Short owner token.
     */
    [[nodiscard]] const char *driveOwnerTok(DriveOwner owner) noexcept
    {
        switch (owner)
        {
        case DriveOwner::RC:
            return "RC";
        case DriveOwner::Direct:
            return "CAR";
        case DriveOwner::Joystick:
            return "JOY";
        case DriveOwner::None:
        default:
            return "NONE";
        }
    }
}

// Main run loop.
void PowerDriveHandler::run() noexcept
{
    TickType_t last_wake = xTaskGetTickCount(); ///< Reference tick for periodic task scheduling.

    for (;;)
    {
        vTaskDelayUntil(&last_wake, loop_ticks_); ///< Reference tick for periodic task scheduling.

        ControlSnapshot cur{};
        const bool stable = bus_.try_peek(cur); ///< Stable-only read (bounded).

        // Check for stable control frame.
        if (stable)
        {
            // Cache the last known-good control state.
            last_ctl_ = cur;
            has_last_ctl_ = true;
        }
        else if (has_last_ctl_)
        {
            // Unstable read detected.
            cur = last_ctl_; ///< Reuse last stable state.
        }
        else
        {
            cur = ControlSnapshot{};               ///< No stable frame ever observed - safe default.
            cur.stamp_ms = utils::clock::now_ms(); ///< Provide timestamp for boot.
        }

        // Safety: if upstream control is stale, treat as unsafe and fall back to neutral.
        const uint32_t now_ms = utils::clock::now_ms();
        const bool stale = (cur.stamp_ms == 0u) ||
                           (!utils::liveness::fresh_ms(now_ms, cur.stamp_ms, cfg::safety::stale::CONTROL_MS));

        // Movement is only allowed from a fresh control frame that explicitly enables drive.
        // This separates "the last semantic direction was Fwd" from "move forward now".
        const bool drive_enabled = !stale && cur.drive_enable;

        // Requested owner/source of the drive command.
        // If movement is not enabled, keep the active owner for continuity.
        const DriveOwner requested_owner = drive_enabled ? cur.drive_owner : active_drive_owner_;

        // If this is the first enabled drive command after boot/neutral, accept its owner
        // immediately while the car is still stopped. Without this, the car can begin
        // moving with active_drive_owner_ still set to None, then falsely trigger a
        // handover stop on the next loop.
        if (drive_enabled && !handover_pending_ && active_drive_owner_ == DriveOwner::None &&
            requested_owner != DriveOwner::None && current_pct_ <= kDirFlipHoldPct)
        {
            active_drive_owner_ = requested_owner;
        }

        // Safe ownership handover interlock:
        // - If drive authority changes while moving, force throttle to zero first.
        // - Only once the car is effectively stopped do we accept the new owner.
        // - This prevents direct/local driving from handing straight into RC FULL motion.
        if (drive_enabled && (requested_owner != DriveOwner::None) &&
            (requested_owner != active_drive_owner_) && (current_pct_ > kDirFlipHoldPct))
        {
            pending_drive_owner_ = requested_owner;
            handover_pending_ = true;
        }

        if (handover_pending_)
        {
            if (current_pct_ <= kDirFlipHoldPct)
            {
                active_drive_owner_ = pending_drive_owner_;
                pending_drive_owner_ = DriveOwner::None;
                handover_pending_ = false;
            }
        }

        // Direction selection.
        // When drive is not enabled, or owner handover is pending, keep the current
        // active direction latched. This avoids meaningless direction flipping while stopped/neutral.
        const DriveDir requested_dir = (drive_enabled && !handover_pending_)
                                           ? cur.dir_cmd
                                           : active_dir_cmd_;

        // Requested selection (percent 0..100, clamped).
        const float requested_pct = (drive_enabled && !handover_pending_)
                                        ? utils::mathfns::clamp_pct(cur.throttle_cmd_pct)
                                        : cfg::units::MIN;

        // Safe direction-change interlock:
        // - If the requested direction differs from the currently executed direction,
        //   force throttle to 0, ramp down, and only then commit the dir change.
        // - Avoids violent instantaneous reversals while the motor is producing torque.
        float target_pct = requested_pct;
        if (requested_dir != active_dir_cmd_)
        {
            target_pct = cfg::units::MIN;

            // Commit the direction flip once stopped.
            if (current_pct_ <= kDirFlipHoldPct)
            {
                active_dir_cmd_ = requested_dir;
            }
        }

        // Simple acceleration/deceleration (rate-based).
        const float dt_sec =
            (static_cast<float>(loop_ticks_) * static_cast<float>(portTICK_PERIOD_MS)) / 1000.0f;

        const float ramp_step_pct = kRampRatePerSec * dt_sec;

        if (current_pct_ < target_pct)
        {
            current_pct_ = fminf(current_pct_ + ramp_step_pct, target_pct);
        }
        else if (current_pct_ > target_pct)
        {
            current_pct_ = fmaxf(current_pct_ - ramp_step_pct, target_pct);
        }

        current_pct_ = utils::mathfns::clamp_pct(current_pct_); ///< Defensive to avoid any overshoot.

        const Dir dir = (active_dir_cmd_ == DriveDir::Rev) ? Dir::CCW : Dir::CW;
        motor_.setSpeedPercent(current_pct_, dir);

        if constexpr (cfg::display::DRIVE_DEBUG)
        {
            const uint32_t debug_now_ms = millis();

            if ((debug_now_ms - last_debug_ms_) >= 250u)
            {
                last_debug_ms_ = debug_now_ms;

                const bool output_enabled = drive_enabled && !handover_pending_;

                const char *state_tok = stale               ? "Stale"
                                        : handover_pending_ ? "Handover"
                                        : cur.drive_enable  ? "Live"
                                                            : "Neutral";

                debugfln(
                    "DRIVE: en=%u owner=%s dir(req)=%s dir(cur)=%s throttle=%.1f%% state=%s speed=%.1f%%->%.1f%%",
                    output_enabled ? 1u : 0u,
                    driveOwnerTok(active_drive_owner_),
                    output_enabled ? (cur.dir_cmd == DriveDir::Rev ? "Rev" : "Fwd") : "N/A",
                    (active_dir_cmd_ == DriveDir::Rev ? "Rev" : "Fwd"),
                    output_enabled ? static_cast<double>(utils::mathfns::clamp_pct(cur.throttle_cmd_pct)) : 0.0,
                    state_tok,
                    static_cast<double>(current_pct_),
                    static_cast<double>(target_pct));
            }
        }
    }
}
