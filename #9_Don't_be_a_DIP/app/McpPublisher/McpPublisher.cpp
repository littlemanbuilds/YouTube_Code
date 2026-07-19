/**
 * MIT License
 *
 * @brief Implementation of McpPublisher.
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

// Construct a McpPublisher task.
McpPublisher::McpPublisher(McpBus &bus, uint32_t period_ms) noexcept
    : bus_(bus), period_ms_(period_ms)
{
}

// Initialize MCP23017 and start the SnapshotRTOS publisher task.
void McpPublisher::begin() noexcept
{
    initMcp_();

    // Start publisher task (SnapshotRTOS owns the loop).
    utils::publisher::start_cb<McpState>(
        bus_,                             ///< Target McpBus.
        this,                             ///< Context pointer (McpPublisher instance for callbacks).
        &McpPublisher::cb_update,         ///< Update callback (sample MCP + refresh cache).
        &McpPublisher::cb_read,           ///< Read callback (copy cached GPIO into snapshot).
        &McpPublisher::cb_ok,             ///< Health callback (report MCP availability).
        srt::AlwaysPublish{},             ///< Publish every tick.
        cfg::task::McpPublisher.name,     ///< Task name.
        cfg::task::McpPublisher.stack,    ///< Stack size.
        cfg::task::McpPublisher.priority, ///< Task priority.
        period_ms_,                       ///< Polling period in milliseconds.
        cfg::task::McpPublisher.core      ///< Core affinity.
    );
}

// Initialize MCP23017 and configure required pins as INPUT_PULLUP.
void McpPublisher::initMcp_() noexcept
{
    // Try init.
    mcp_healthy_ = mcp_.begin_I2C(cfg::mcp23017::ADDR);

    if (!mcp_healthy_)
    {
        return;
    }

    // Configure pins only when init succeeds.
    for (uint8_t pin : cfg::mcp23017::INPUT_PINS)
    {
        mcp_.pinMode(pin, INPUT_PULLUP);
    }

    // Reset probe state on successful init.
    probe_fail_ = 0;
    last_probe_ms_ = utils::clock::now_ms();
}

// Sample GPIOA and GPIOB into cached members (one I2C read per port).
void McpPublisher::sample_() noexcept
{
    const uint32_t now = utils::clock::now_ms();

    if (!mcp_healthy_)
    {
        // Retry init at 1 Hz.
        if (static_cast<uint32_t>(now - last_try_ms_) >= 1000u)
        {
            last_try_ms_ = now;
            initMcp_();
        }

        // Publish 'all high' cache while offline (safe default for INPUT_PULLUP).
        gpioa_cached_ = 0xFFu;
        gpiob_cached_ = 0xFFu;

        // While offline, clear probe streak.
        probe_fail_ = 0;
        last_probe_ms_ = now;
        return;
    }

    // ---- Lightweight runtime health check ---- //
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
                last_try_ms_ = now; ///< Start 1 Hz retry cadence from now.

                // Publish safe defaults immediately.
                gpioa_cached_ = 0xFFu;
                gpiob_cached_ = 0xFFu;
                return;
            }
        }
    }

    // One I2C read per port.
    gpioa_cached_ = static_cast<uint8_t>(mcp_.readGPIOA());
    gpiob_cached_ = static_cast<uint8_t>(mcp_.readGPIOB());
}

// Lightweight I2C presence probe (ACK check).
bool McpPublisher::probeAck_() noexcept
{
    Wire.beginTransmission(cfg::mcp23017::ADDR);
    const uint8_t err = static_cast<uint8_t>(Wire.endTransmission(true));
    return (err == 0u);
}