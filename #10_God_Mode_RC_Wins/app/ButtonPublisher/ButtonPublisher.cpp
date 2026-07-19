/**
 * MIT License
 *
 * @brief Samples and debounces the car's physical buttons.
 *
 * @file ButtonPublisher.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-09-12
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#include "ButtonPublisher.h"

#include <Arduino.h>
#include <Publisher.h>

namespace srt = snapshot::rtos; ///< Short alias for SnapshotRTOS helpers.

ButtonPublisher::ButtonPublisher(McpBus &mcp,
                                 ButtonBus &bus,
                                 uint32_t period_ms) noexcept
    : bus_(bus),
      buttons_(BUTTON_PINS,
               &ButtonPublisher::readMcp,
               this,
               ButtonTimingConfig{cfg::button::BTN_DEBOUNCE_MS,
                                  cfg::button::BTN_SHORT_MS,
                                  cfg::button::BTN_LONG_MS},
               true,
               &utils::clock::now_ms),
      period_ms_(period_ms > 0u ? period_ms : 1u),
      mcp_view_(mcp)
{
    configASSERT(period_ms > 0u);
}

void ButtonPublisher::begin() noexcept
{
    // SnapshotRTOS owns the periodic sampling loop; these callbacks provide the
    // hardware update, frame copy, and health result for each cycle.
    utils::publisher::start_cb<ButtonState>(
        bus_,                                // Target ButtonBus.
        this,                                // Context pointer (ButtonPublisher instance for callbacks).
        &ButtonPublisher::cb_update,         // Update callback (refresh cache + debounce).
        &ButtonPublisher::cb_read,           // Read callback (copy debounced state).
        &ButtonPublisher::cb_ok,             // Health callback (report MCP validity).
        srt::AlwaysPublish{},                // Continuous frames make freshness independent of button changes.
        cfg::task::ButtonPublisher.name,     // Task name.
        cfg::task::ButtonPublisher.stack,    // Stack size.
        cfg::task::ButtonPublisher.priority, // Task priority.
        period_ms_,                          // Polling period in milliseconds.
        cfg::task::ButtonPublisher.core      // Core affinity.
    );
}

bool ButtonPublisher::readMcp(void *ctx, uint8_t id) noexcept
{
    // Universal_Button supplies the publisher object registered as its callback context.
    auto *self = static_cast<ButtonPublisher *>(ctx);
    return self->mcp_view_.asserted(id); // INPUT_PULLUP wiring: LOW == asserted.
}

bool ButtonPublisher::refreshCache_() noexcept
{
    mcp_view_.refresh();

    if (!mcp_view_.ok())
    {
        buttons_.reset(); // Prevent a disconnected input expander from creating phantom presses.
        return false;
    }

    return true;
}
