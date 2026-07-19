/**
 * MIT License
 *
 * @brief Implementation of ConfigPublisher.
 *
 * @file ConfigPublisher.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-02
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#include "ConfigPublisher.h"

#include <Arduino.h>
#include <Publisher.h>

namespace srt = snapshot::rtos; ///< Short alias for SnapshotRTOS helpers.

namespace
{
    /**
     * @brief Change detector for configuration frames.
     *
     * IMPORTANT: Ignore stamps so heartbeat doesn't cause spurious 'change' publishes.
     */
    struct ConfigChanged
    {
        [[nodiscard]] bool operator()(const ConfigSnapshot &a, const ConfigSnapshot &b) const noexcept
        {
            return (a.mode != b.mode) ||
                   (a.gear != b.gear) ||
                   (a.dip_raw != b.dip_raw) ||
                   (a.gear_raw != b.gear_raw) ||
                   (a.dip_locked != b.dip_locked);
        }
    };
} ///< namespace

// Construct the ConfigPublisher task.
ConfigPublisher::ConfigPublisher(McpBus &mcp,
                                 ConfigBus &out,
                                 uint32_t period_ms,
                                 uint32_t min_publish_ms) noexcept
    : mcp_{mcp}, bus_{out}, period_ms_{period_ms}, min_publish_ms_{min_publish_ms}, mcp_view_(mcp)
{
}

// Initialize configuration state and start the ConfigPublisher task.
void ConfigPublisher::begin() noexcept
{
    // Give MCP a short window to come online.
    for (uint8_t i = 0; i < 50u; ++i) ///< ~500 ms.
    {
        mcp_view_.refresh();
        if (mcp_view_.ok())
        {
            break;
        }
        delay(10);
    }

    // Cache latest MCP snapshot.
    mcp_view_.refresh();

    if (!mcp_view_.ok())
    {
        fatalWaitForMcpAndReboot_(); ///< Blocks forever or reboots.
    }

    // Normal init path.
    dip_.sync();
    gear_.sync();

    // Latch DIP once for the session (boot config locked).
    settleAndLatchDip_();

    // Publish initial snapshot immediately.
    ConfigSnapshot init{};
    fillSnapshot_(init);
    bus_.publish(init);

    // Start publisher task (SnapshotRTOS owns the loop).
    utils::publisher::start_cb<ConfigSnapshot>(
        bus_,                                ///< Target ConfigBus.
        this,                                ///< Context pointer (ConfigPublisher instance for callbacks).
        &ConfigPublisher::cb_update,         ///< Update callback (advance bank state).
        &ConfigPublisher::cb_read,           ///< Read callback (fill snapshot).
        &ConfigPublisher::cb_ok,             ///< Health callback (report MCP availability).
        srt::AlwaysPublish{},                ///< Publish every tick.
        cfg::task::ConfigPublisher.name,     ///< Task name.
        cfg::task::ConfigPublisher.stack,    ///< Stack size.
        cfg::task::ConfigPublisher.priority, ///< Task priority.
        period_ms_,                          ///< Polling period in milliseconds.
        cfg::task::ConfigPublisher.core      ///< Core affinity.
    );
}

// SwitchBank reader callback for MCP23017 virtual pins.
bool ConfigPublisher::readMcp(void *ctx, uint8_t key) noexcept
{
    auto *self = static_cast<ConfigPublisher *>(ctx); ///< Recover owning publisher instance.

    configASSERT(key < 16u); ///< MCP virtual pins indices 0..15 (GPIOA = 0..7; GPIOB = 8..15).

    return self->mcp_view_.level_high(key); ///< SwitchBank expects electrical level: true == HIGH.
}

// Decode 3-bit DIP value into CarMode.
CarMode ConfigPublisher::decodeMode(uint8_t dip_value) noexcept
{
    return static_cast<CarMode>(dip_value & cfg::switches::DIP_VALUE_MASK);
}

// Decode physical shift contact bits into a logical Gear value.
Gear ConfigPublisher::decodeGear(uint8_t gear_bits) noexcept
{
    using namespace cfg::switches;

    const bool gearR = (gear_bits & GEAR_R_MASK) != 0u;
    const bool gearFwd = (gear_bits & GEAR_FWD_MASK) != 0u;
    const bool gearD1 = (gear_bits & GEAR_D1_MASK) != 0u;
    const bool gearD3 = (gear_bits & GEAR_D3_MASK) != 0u;

    const bool gearP = !gearR && !gearFwd;
    const bool gearD2 = gearFwd && !gearD1 && !gearD3;

    if (gearP)
    {
        return Gear::Park;
    }

    if (gearR && !gearFwd && !gearD1 && !gearD3)
    {
        return Gear::Rev;
    }

    if (!gearR && gearFwd && gearD1 && !gearD3)
    {
        return Gear::D1;
    }

    if (!gearR && gearD2)
    {
        return Gear::D2;
    }

    if (!gearR && gearFwd && !gearD1 && gearD3)
    {
        return Gear::D3;
    }

    return Gear::Unknown;
}

// Allow debouncers to settle, then latch DIP once for the session.
void ConfigPublisher::settleAndLatchDip_() noexcept
{
    const uint32_t start_ms = utils::clock::now_ms();

    while (static_cast<uint32_t>(utils::clock::now_ms() - start_ms) < (cfg::switches::DEBOUNCE_MS * 2u))
    {
        updateBanks_();
        vTaskDelay(1); ///< Yield during settle.
    }

    dip_latched_ = static_cast<uint8_t>(dip_.peekValue() & cfg::switches::DIP_VALUE_MASK);
    dip_locked_ = true;
}

// Refresh cached MCP sample and advance debounce engines.
void ConfigPublisher::updateBanks_() noexcept
{
    const uint32_t now_ms = utils::clock::now_ms();

    mcp_view_.refresh(); ///< Refresh cached MCP sample once per loop.

    // If MCP is unhealthy, do not advance debouncers based on bad reads.
    if (!mcp_view_.ok())
    {
        mcp_healthy_ = false;
        return;
    }

    // On first valid MCP sample after unhealthy, resync to avoid false edges.
    if (!mcp_healthy_)
    {
        mcp_healthy_ = true;
        dip_.sync();
        gear_.sync();
    }

    // DIP is latched at boot, but keep debouncer running to maintain stable state.
    dip_.update(now_ms);
    gear_.update(now_ms);
}

// Build a ConfigSnapshot from latched DIP + current debounced shifter contacts.
void ConfigPublisher::fillSnapshot_(ConfigSnapshot &s) noexcept
{
    configASSERT(dip_locked_);

    s.dip_raw = dip_latched_;
    s.mode = decodeMode(s.dip_raw);

    if (!mcp_healthy_) ///< MCP unhealthy => gear is untrusted.
    {
        s.gear_raw = 0u;
        s.gear = Gear::Unknown;
    }
    else
    {
        s.gear_raw = static_cast<uint8_t>(gear_.peekValue() & cfg::switches::GEAR_VALUE_MASK);
        s.gear = decodeGear(s.gear_raw);
    }

    s.dip_locked = dip_locked_;
    s.stamp_us = utils::clock::now_us(); ///< Critical: config output liveness (monotonic).
}

// Fatal safe loop: publish SAFE config while waiting for MCP recovery, then reboot.
[[noreturn]] void ConfigPublisher::fatalWaitForMcpAndReboot_() noexcept
{
    TickType_t last = xTaskGetTickCount();
    uint8_t ok_count{0}; ///< Consecutive count of healthy MCP samples (qualification window).

    for (;;)
    {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(1000));

        // Publish SAFE config regularly. Gear is forced 'Unknown' while MCP is unhealthy.
        // Do not call fillSnapshot_() here: boot-time MCP failure can happen before DIP is latched.
        ConfigSnapshot safe{};
        safe.dip_raw = dip_locked_ ? dip_latched_ : 0u;
        safe.mode = decodeMode(safe.dip_raw);
        safe.gear_raw = 0u;
        safe.gear = Gear::Unknown;
        safe.dip_locked = dip_locked_;
        safe.stamp_us = utils::clock::now_us();
        bus_.publish(safe);

        // Check MCP health.
        mcp_view_.refresh();

        ok_count = mcp_view_.ok() ? static_cast<uint8_t>(ok_count + 1u) : 0u;

        // Require stable MCP before reboot.
        if (ok_count >= 3u) ///< ~3 seconds.
        {
            vTaskDelay(pdMS_TO_TICKS(50));
            esp_restart();
        }
    }
}
