/**
 * MIT License
 *
 * @brief Implementation of PowerDriveHandler (motor power and drive handler).
 *
 * @file PowerDriveHandler.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-09-12
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#include "PowerDriveHandler.h"

#include <Clock.h>
#include <Debugging.h>
#include <Liveness.h>
#include <MathFns.h>

#include <cmath>

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

        // Safety:: if upstream control is stale, treat as unsafe and fall back to neutral.
        const uint32_t now_ms = utils::clock::now_ms();
        const bool stale = (cur.stamp_ms == 0u) ||
                           (!utils::liveness::fresh_ms(now_ms, cur.stamp_ms, cfg::safety::stale::CONTROL_MS));

        // Direction selection.
        const DriveDir requested_dir = stale ? active_dir_cmd_ : cur.dir_cmd;

        // Requested selection (percent 0..100, clamped).
        const float requested_pct = stale ? cfg::units::MIN
                                          : utils::mathfns::clamp_pct(cur.throttle_cmd_pct);

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
        /* debugfln(
            "Req=%s Act=%s  Thr=%.1f → %.1f%%  %s",
            (cur.dir_cmd == DriveDir::Rev ? "Rev" : "Fwd"),
            (active_dir_cmd_ == DriveDir::Rev ? "Rev" : "Fwd"),
            current_pct_,
            target_pct,
            stale ? "STALE" : "LIVE"); */
    }
}