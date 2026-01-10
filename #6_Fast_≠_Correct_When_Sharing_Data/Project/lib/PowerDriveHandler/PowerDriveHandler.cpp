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
    configASSERT(motor_ != nullptr && bus_ != nullptr); ///< Sanity check: motor and bus must be valid.
    configASSERT(loop_ticks_ > 0);                      ///< Timing must be configured.

    TickType_t last_wake = xTaskGetTickCount(); ///< Reference tick for periodic task scheduling.

    for (;;)
    {
        const InputState cur = bus_->peek();

        // ---- Simple acceleration/deceleration ----
        const bool pressed = cur.buttons.test(btnAccel);
        const float targetPct = pressed ? kMaxPct : kMinPct;

        if (current_pct_ < targetPct)
        {
            current_pct_ += kRampStepPct;
            if (current_pct_ > kMaxPct)
                current_pct_ = kMaxPct;
        }
        else if (current_pct_ > targetPct)
        {
            current_pct_ -= kRampStepPct;
            if (current_pct_ < kMinPct)
                current_pct_ = kMinPct;
        }

        motor_->setSpeedPercent(current_pct_, kDir);
        // debugfln("Speed: %.1f %%", current_pct_);

        vTaskDelayUntil(&last_wake, loop_ticks_); ///< Pace loop.
    }
}