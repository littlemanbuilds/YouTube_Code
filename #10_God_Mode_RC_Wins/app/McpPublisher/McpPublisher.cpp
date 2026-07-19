/**
 * MIT License
 *
 * @brief Owns MCP23017 startup, sampling, disconnect detection, and recovery.
 *
 * @file McpPublisher.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-20
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#include "McpPublisher.h"

#include <Publisher.h>
#include <Wire.h>

namespace srt = snapshot::rtos; ///< Short alias for SnapshotRTOS helpers.

McpPublisher::McpPublisher(McpBus &bus, uint32_t period_ms) noexcept
    : bus_(bus), period_ms_(period_ms > 0u ? period_ms : 1u)
{
    configASSERT(period_ms > 0u);
}

void McpPublisher::begin() noexcept
{
    initMcp_();

    // SnapshotRTOS runs the periodic sampler and publishes each cached pin frame.
    utils::publisher::start_cb<McpState>(
        bus_,                             // Target McpBus.
        this,                             // Context pointer (McpPublisher instance for callbacks).
        &McpPublisher::cb_update,         // Update callback (sample MCP + refresh cache).
        &McpPublisher::cb_read,           // Read callback (copy cached GPIO into snapshot).
        &McpPublisher::cb_ok,             // Health callback (report MCP availability).
        srt::AlwaysPublish{},             // Publish every tick.
        cfg::task::McpPublisher.name,     // Task name.
        cfg::task::McpPublisher.stack,    // Stack size.
        cfg::task::McpPublisher.priority, // Task priority.
        period_ms_,                       // Polling period in milliseconds.
        cfg::task::McpPublisher.core      // Core affinity.
    );
}

void McpPublisher::initMcp_() noexcept
{
    mcp_healthy_ = mcp_.begin_I2C(cfg::mcp23017::ADDR);

    if (!mcp_healthy_)
    {
        return;
    }

    // Active-low switches need pull-ups so an open contact reads HIGH.
    for (uint8_t pin : cfg::mcp23017::INPUT_PINS)
    {
        mcp_.pinMode(pin, INPUT_PULLUP);
    }

    // A successful initialization starts a new health-check window.
    probe_fail_ = 0;
    last_probe_ms_ = utils::clock::now_ms();
}

void McpPublisher::sample_() noexcept
{
    const uint32_t now = utils::clock::now_ms();

    if (!mcp_healthy_)
    {
        // Retry once per second without blocking the rest of the application.
        if (static_cast<uint32_t>(now - last_try_ms_) >= 1000u)
        {
            last_try_ms_ = now;
            initMcp_();
        }

        // HIGH means "not pressed" for these active-low inputs, so all HIGH is
        // the safe disconnected-hardware value.
        gpioa_cached_ = 0xFFu;
        gpiob_cached_ = 0xFFu;

        // The failure streak applies only while an initialized device is online.
        probe_fail_ = 0;
        last_probe_ms_ = now;
        return;
    }

    // ---- Detect a device that disappears after successful startup ---- //
    if (static_cast<uint32_t>(now - last_probe_ms_) >= kProbeIntervalMs)
    {
        last_probe_ms_ = now;

        if (probeAck_())
        {
            probe_fail_ = 0;
        }
        else
        {
            if (probe_fail_ < 0xFFu)
            {
                ++probe_fail_;
            }

            if (probe_fail_ >= kMaxProbeFails)
            {
                mcp_healthy_ = false;
                last_try_ms_ = now; // Start the one-second recovery cadence now.

                // Remove any previously asserted inputs immediately.
                gpioa_cached_ = 0xFFu;
                gpiob_cached_ = 0xFFu;
                return;
            }
        }
    }

    // Read each eight-bit port once so every consumer sees the same cached sample.
    gpioa_cached_ = static_cast<uint8_t>(mcp_.readGPIOA());
    gpiob_cached_ = static_cast<uint8_t>(mcp_.readGPIOB());
}

bool McpPublisher::probeAck_() noexcept
{
    Wire.beginTransmission(cfg::mcp23017::ADDR);
    const uint8_t err = static_cast<uint8_t>(Wire.endTransmission(true));
    return (err == 0u);
}
