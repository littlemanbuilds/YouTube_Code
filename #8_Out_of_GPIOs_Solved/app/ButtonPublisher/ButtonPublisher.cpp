/**
 * MIT License
 *
 * @brief Implementation of ButtonPublisher.
 *
 * @file ButtonPublisher.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-09-12
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#include "ButtonPublisher.h"

#include <Arduino.h>
#include <Publisher.h>

namespace srt = snapshot::rtos; ///< Short alias for SnapshotRTOS helpers.

// Construct a ButtonPublisher task.
ButtonPublisher::ButtonPublisher(McpBus &mcp,
                                 ButtonBus &bus,
                                 uint32_t period_ms,
                                 uint32_t min_publish_ms) noexcept
    : mcp_(mcp),
      bus_(bus),
      period_ms_(period_ms),
      min_publish_ms_(min_publish_ms),
      buttons_(BUTTON_PINS,
               &ButtonPublisher::readMcp,
               this,
               ButtonTimingConfig{cfg::button::BTN_DEBOUNCE_MS,
                                  cfg::button::BTN_SHORT_MS,
                                  cfg::button::BTN_LONG_MS},
               true,
               &utils::clock::now_ms),
      mcp_view_(mcp)
{
}

// Start the SnapshotRTOS publisher task.
void ButtonPublisher::begin() noexcept
{
    // Start publisher task (SnapshotRTOS owns the loop).
    utils::publisher::start_cb<ButtonState>(
        bus_,                                ///< Target ButtonBus.
        this,                                ///< Context pointer (ButtonPublisher instance for callbacks).
        &ButtonPublisher::cb_update,         ///< Update callback (refresh cache + debounce).
        &ButtonPublisher::cb_read,           ///< Read callback (copy debounced state).
        &ButtonPublisher::cb_ok,             ///< Health callback (report MCP validity).
        srt::AlwaysPublish{},                ///< Publish every tick.
        cfg::task::ButtonPublisher.name,     ///< Task name.
        cfg::task::ButtonPublisher.stack,    ///< Stack size.
        cfg::task::ButtonPublisher.priority, ///< Task priority.
        period_ms_,                          ///< Polling period in milliseconds.
        cfg::task::ButtonPublisher.core      ///< Core affinity.
    );
}

// Universal_Button reader callback using cahced MCP23017 samples.
bool ButtonPublisher::readMcp(void *ctx, uint8_t id) noexcept
{
    auto *self = static_cast<ButtonPublisher *>(ctx); ///< Recover owning publisher instance.
    return self->mcp_view_.asserted(id);              ///< INPUT_PULLUP wiring: LOW == asserted.
}

// Refresh cached MCP23017 samples and return health status.
bool ButtonPublisher::refreshCache_() noexcept
{
    mcp_view_.refresh();

    if (!mcp_view_.ok())
    {
        buttons_.reset(); ///< Prevent phantom edges if source is unhealthy.
        return false;
    }

    return true;
}