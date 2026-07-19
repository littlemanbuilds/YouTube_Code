/**
 * MIT License
 *
 * @brief Converts final drive commands into ramped, interlocked motor output.
 *
 * @file PowerDriveHandler.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-09-12
 * @copyright Copyright (c) 2026 Little Man Builds
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
} ///< namespace

void PowerDriveHandler::run() noexcept
{
    TickType_t last_wake = xTaskGetTickCount(); ///< Reference tick for periodic task scheduling.

    for (;;)
    {
        vTaskDelayUntil(&last_wake, loop_ticks_); // Maintain fixed-period scheduling.

        ControlSnapshot cur{};
        const bool stable = bus_.try_peek(cur);

        // If this read overlaps a publish, reuse the last complete command. At
        // startup, use a neutral command until the first complete frame arrives.
        if (stable)
        {
            last_ctl_ = cur;
            has_last_ctl_ = true;
        }
        else if (has_last_ctl_)
        {
            cur = last_ctl_;
        }
        else
        {
            cur = ControlSnapshot{};               // Safe neutral startup command.
            cur.stamp_ms = utils::clock::now_ms(); // Give the safe startup command a valid timestamp.
        }

        // A once-valid command is not valid forever. Missing updates force neutral.
        const uint32_t now_ms = utils::clock::now_ms();
        const bool stale = (cur.stamp_ms == 0u) ||
                           (!utils::liveness::fresh_ms(now_ms, cur.stamp_ms, cfg::safety::stale::CONTROL_MS));

        // Remembering a direction is not permission to move. Movement also requires
        // a recent frame that explicitly enables the drivetrain.
        const bool drive_enabled = !stale && cur.drive_enable;

        // Accept a new driver immediately at rest. While moving, ramp to zero before
        // allowing the new owner's request to reach the motor.
        const DriveOwner requested_owner = drive_enabled ? cur.drive_owner : DriveOwner::None;
        const bool owner_changed = app::drive::updateOwnerHandover(
            owner_handover_, drive_enabled, requested_owner, current_pct_, kDirFlipHoldPct);

        // The stopped threshold absorbs motor dead-band and float noise. If a handover
        // completes with any residual command, emit one explicit zero-output cycle
        // before accepting demand from the new owner.
        const bool owner_change_needs_zero_cycle = owner_changed && (current_pct_ > cfg::units::MIN);
        if (owner_change_needs_zero_cycle)
        {
            current_pct_ = cfg::units::MIN;
        }

        const bool owner_command_allowed =
            !owner_change_needs_zero_cycle &&
            app::drive::ownerCommandAllowed(owner_handover_, drive_enabled, requested_owner);

        // ---- Direction selection ---- //

        // Keep the physical motor direction latched while stopped or handing over.
        // Changing it early would create unnecessary output switching.
        const DriveDir requested_dir = owner_command_allowed ? cur.dir_cmd : active_dir_cmd_;

        // Convert the accepted drive demand into a bounded 0..100% motor target.
        const float requested_pct = owner_command_allowed
                                        ? utils::mathfns::clamp_pct(cur.throttle_cmd_pct)
                                        : cfg::units::MIN;

        // Never reverse a motor that is still producing torque. A direction change
        // first commands zero, waits for the ramp to reach "stopped," then flips.
        float target_pct = requested_pct;
        if (requested_dir != active_dir_cmd_)
        {
            target_pct = cfg::units::MIN;

            // Commit the new direction only after motor output is effectively zero.
            if (current_pct_ <= kDirFlipHoldPct)
            {
                active_dir_cmd_ = requested_dir;
                current_pct_ = cfg::units::MIN; // Change polarity only with an explicit zero command.
            }
        }

        // Derive a fixed per-cycle step from the configured task period so the
        // tuning value remains an intuitive percentage-per-second rate.
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

        current_pct_ = utils::mathfns::clamp_pct(current_pct_); // Defensively prevent overshoot.

        const Dir dir = (active_dir_cmd_ == DriveDir::Rev) ? Dir::CCW : Dir::CW;
        motor_.setSpeedPercent(current_pct_, dir);

        if constexpr (cfg::display::DRIVE_DEBUG)
        {
            const uint32_t debug_now_ms = millis();

            if ((debug_now_ms - last_debug_ms_) >= 250u)
            {
                last_debug_ms_ = debug_now_ms;

                const bool output_enabled = owner_command_allowed;

                const char *state_tok = stale                         ? "Stale"
                                        : owner_handover_.in_progress ? "Handover"
                                        : cur.drive_enable            ? "Live"
                                                                      : "Neutral";

                debugfln(
                    "DRIVE: en=%u owner=%s dir(req)=%s dir(cur)=%s throttle=%.1f%% state=%s speed=%.1f%%->%.1f%%",
                    output_enabled ? 1u : 0u,
                    driveOwnerTok(owner_handover_.active),
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
