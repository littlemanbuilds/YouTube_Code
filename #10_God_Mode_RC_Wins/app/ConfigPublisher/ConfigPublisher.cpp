/**
 * MIT License
 *
 * @brief Decodes physical mode and shifter switches into safe configuration state.
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

ConfigPublisher::ConfigPublisher(McpBus &mcp,
                                 ConfigBus &out,
                                 uint32_t period_ms) noexcept
    : bus_{out}, period_ms_{period_ms > 0u ? period_ms : 1u}, mcp_view_(mcp)
{
    configASSERT(period_ms > 0u);
}

void ConfigPublisher::begin() noexcept
{
    // Allow the MCP23017 up to roughly 500 ms to become ready at boot.
    for (uint8_t i = 0; i < 50u; ++i) // Approximately 500 ms.
    {
        mcp_view_.refresh();
        if (mcp_view_.ok())
        {
            break;
        }
        delay(10);
    }

    mcp_view_.refresh();

    if (!mcp_view_.ok())
    {
        fatalWaitForMcpAndReboot_(); // Keep publishing a safe state until hardware recovery.
    }

    // Synchronize the debouncers to the real pin levels before looking for changes.
    dip_.sync();
    gear_.sync();

    // Lock the operating mode only after its switch contacts have settled.
    settleAndLatchDip_();

    // Publish immediately so downstream policy does not wait for the first task cycle.
    ConfigSnapshot init{};
    fillSnapshot_(init);
    bus_.publish(init);

    // SnapshotRTOS owns the periodic loop after the safe boot sequence completes.
    utils::publisher::start_cb<ConfigSnapshot>(
        bus_,                                // Target ConfigBus.
        this,                                // Context pointer (ConfigPublisher instance for callbacks).
        &ConfigPublisher::cb_update,         // Update callback (advance bank state).
        &ConfigPublisher::cb_read,           // Read callback (fill snapshot).
        &ConfigPublisher::cb_ok,             // Health callback (report MCP availability).
        srt::AlwaysPublish{},                // Continuous frames make freshness independent of switch changes.
        cfg::task::ConfigPublisher.name,     // Task name.
        cfg::task::ConfigPublisher.stack,    // Stack size.
        cfg::task::ConfigPublisher.priority, // Task priority.
        period_ms_,                          // Polling period in milliseconds.
        cfg::task::ConfigPublisher.core      // Core affinity.
    );
}

bool ConfigPublisher::readMcp(void *ctx, uint8_t key) noexcept
{
    // SwitchBank supplies the publisher object registered as its callback context.
    auto *self = static_cast<ConfigPublisher *>(ctx);

    configASSERT(key < 16u); // MCP virtual pin indices 0..15 (GPIOA = 0..7; GPIOB = 8..15).

    return self->mcp_view_.level_high(key); // SwitchBank expects electrical level: true == HIGH.
}

CarMode ConfigPublisher::decodeMode(uint8_t dip_value) noexcept
{
    return static_cast<CarMode>(dip_value & cfg::switches::DIP_VALUE_MASK);
}

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

void ConfigPublisher::settleAndLatchDip_() noexcept
{
    const uint32_t start_ms = utils::clock::now_ms();

    while (static_cast<uint32_t>(utils::clock::now_ms() - start_ms) < (cfg::switches::DEBOUNCE_MS * 2u))
    {
        updateBanks_();
        vTaskDelay(1); // Let other tasks run while the debounce window elapses.
    }

    dip_latched_ = static_cast<uint8_t>(dip_.peekValue() & cfg::switches::DIP_VALUE_MASK);
    dip_locked_ = true;
}

void ConfigPublisher::updateBanks_() noexcept
{
    const uint32_t now_ms = utils::clock::now_ms();

    mcp_view_.refresh(); // One hardware snapshot feeds both switch banks.

    // Invalid hardware data must not advance a debouncer or create a false switch change.
    if (!mcp_view_.ok())
    {
        mcp_healthy_ = false;
        return;
    }

    // After communication returns, resynchronize to the current pins instead of
    // replaying the disconnect as a sequence of switch edges.
    if (!mcp_healthy_)
    {
        mcp_healthy_ = true;
        dip_.sync();
        gear_.sync();
    }

    // Keep sampling the locked DIP alongside the gear so both banks remain synchronized.
    dip_.update(now_ms);
    gear_.update(now_ms);
}

void ConfigPublisher::fillSnapshot_(ConfigSnapshot &s) noexcept
{
    configASSERT(dip_locked_);

    s.dip_raw = dip_latched_;
    s.mode = decodeMode(s.dip_raw);

    if (!mcp_healthy_) // Without trustworthy contacts, Unknown is the only safe gear.
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
    s.stamp_us = utils::clock::now_us(); // Lets consumers reject the configuration if updates stop.
}

[[noreturn]] void ConfigPublisher::fatalWaitForMcpAndReboot_() noexcept
{
    TickType_t last = xTaskGetTickCount();
    uint8_t ok_count{0}; ///< Consecutive healthy samples required before rebooting.

    for (;;)
    {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(1000));

        // Keep downstream control alive but immobile: Unknown gear produces a zero
        // output cap. This path may run before the DIP mode has been locked.
        ConfigSnapshot safe{};
        safe.dip_raw = dip_locked_ ? dip_latched_ : 0u;
        safe.mode = decodeMode(safe.dip_raw);
        safe.gear_raw = 0u;
        safe.gear = Gear::Unknown;
        safe.dip_locked = dip_locked_;
        safe.stamp_us = utils::clock::now_us();
        bus_.publish(safe);

        mcp_view_.refresh();

        ok_count = mcp_view_.ok() ? static_cast<uint8_t>(ok_count + 1u) : 0u;

        // Require three consecutive healthy probes before rebooting into normal startup.
        if (ok_count >= 3u) // Approximately three seconds.
        {
            vTaskDelay(pdMS_TO_TICKS(50));
            esp_restart();
        }
    }
}
