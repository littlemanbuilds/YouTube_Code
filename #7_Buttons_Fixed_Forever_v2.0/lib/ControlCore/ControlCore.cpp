/**
 * MIT License
 *
 * @brief Control core implementation.
 *
 * @file ControlCore.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-12-30
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#include "ControlCore.h"

// Main run loop.
void ControlCore::run() noexcept
{
    configASSERT(in_ != nullptr && out_ != nullptr); ///< Sanity check: buses in_ and out_ must be valid.
    configASSERT(loop_ticks_ > 0);                   ///< Timing must be configured.

    TickType_t last_wake = xTaskGetTickCount();

    for (;;)
    {
        const InputState cur = in_->peek();

        // Input event logging.
        if (has_prev_)
        {
            logButtonEvents(prev_, cur);
        }

        // Build control commands.
        ControlSnapshot out{};
        out.throttle_cmd_pct = cur.buttons.test(idx(kBtnAccel)) ? kMaxPct : kMinPct;
        out.horn_cmd = cur.buttons.test(idx(kBtnHorn));

        out.indicator_cmd = ControlSnapshot::Indicator::Off;

        if (cur.buttons.test(idx(kBtnLeft)))
            out.indicator_cmd = ControlSnapshot::Indicator::Left;
        else if (cur.buttons.test(idx(kBtnRight)))
            out.indicator_cmd = ControlSnapshot::Indicator::Right;

        out.stamp_ms = cur.stamp_ms;

        out_->publish(out);

        // Update previous snapshot for next edge detection.
        prev_ = cur;
        has_prev_ = true;

        vTaskDelayUntil(&last_wake, loop_ticks_);
    }
}