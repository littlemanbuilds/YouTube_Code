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

// Main run loop.
void PowerDriveHandler::run() noexcept
{
    configASSERT(motor_ != nullptr && bus_ != nullptr); ///< Sanity check: motor_ and bus_ must be valid.
    configASSERT(loop_ticks_ > 0);                      ///< Timing must be configured.

    TickType_t last_wake = xTaskGetTickCount(); ///< Reference tick for periodic task scheduling.

    for (;;)
    {
        const ControlSnapshot cur = bus_->peek();

        // Target selection.
        const float targetPct = fminf(fmaxf(cur.throttle_cmd_pct, kMinPct), kMaxPct); ///< Clamp to avoid nonsense values.

        // ---- Simple acceleration/deceleration (rate-based) ---- //
        const float dt_sec =
            (static_cast<float>(loop_ticks_) * static_cast<float>(portTICK_PERIOD_MS)) / 1000.0f;

        const float ramp_step_pct = kRampRatePctPerSec * dt_sec;

        if (current_pct_ < targetPct)
        {
            current_pct_ = fminf(current_pct_ + ramp_step_pct, targetPct);
        }
        else if (current_pct_ > targetPct)
        {
            current_pct_ = fmaxf(current_pct_ - ramp_step_pct, targetPct);
        }

        motor_->setSpeedPercent(current_pct_, kDir);
        // debugfln("Speed: %.1f %%", current_pct_);

        vTaskDelayUntil(&last_wake, loop_ticks_); ///< Pace loop.
    }
}