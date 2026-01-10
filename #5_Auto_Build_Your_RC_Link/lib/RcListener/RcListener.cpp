/**
 * MIT License
 *
 * @brief Implementation of RC listener (iBUS → RcLink → SnapshotBus).
 *
 * @file RcListener.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-10-08
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#include "RcListener.h"

// Constructor.
RcListener::RcListener(uint32_t period_ms) noexcept : loop_ticks_(to_ticks_ms(period_ms)) {}

// Configure RCLink (axes, switches, etc.).
void RcListener::begin() noexcept
{
    rclink_.begin(Serial2, cfg::rc::BAUD, cfg::rc::UART_RX, cfg::rc::UART_TX); ///< Start iBUS UART on Serial2.

    RC_CONFIG(RC, cfg);          ///< Build configuration.
    RC_CFG_MAP_DEFAULT(RC, cfg); ///< Map roles in declared order to channels.

    // Axes.
    cfg.axis(RC::steering).raw(1000, 2000, 1500).deadband_us(8).out(-100.f, 100.f).done();
    cfg.axis(RC::direction).raw(1000, 2000, 1500).deadband_us(8).out(-100.f, 100.f).done();
    cfg.axis(RC::speed).raw(1000, 2000, 1000).deadband_us(8).out(0.f, 100.f).done();
    cfg.axis(RC::indicators).raw(1000, 2000, 1500).deadband_us(8).out(-100.f, 100.f).done();
    cfg.axis(RC::volume).raw(1000, 2000, 1500).deadband_us(4).out(0.f, 100.f).done();
    cfg.axis(RC::power).raw(1000, 2000, 1500).deadband_us(4).out(0.f, 100.f).done();

    // Switches.
    cfg.sw(RC::override).raw_levels({1000, 2000}).values({0.f, 1.f}).done();
    cfg.sw(RC::lights).raw_levels({1000, 2000}).values({0.f, 1.f}).done();
    cfg.sw(RC::mode).raw_levels({1000, 1500, 2000}).values({0.f, 1.f, 2.f}).done();
    cfg.sw(RC::obstacle).raw_levels({1000, 2000}).values({0.f, 1.f}).done();

    rclink_.apply_config(cfg); ///< Apply configuration.
}

// Main run loop.
void RcListener::run() noexcept
{
    configASSERT(loop_ticks_ > 0); ///< Timing must be configured.

    TickType_t last = xTaskGetTickCount(); ///< Reference tick for periodic task scheduling.

    uint32_t last_log_ms = 0; ///< Last time printed.

    for (;;)
    {
        rclink_.update(); ///< Update link and get new frame (if available).

        const uint32_t now_ms = millis(); ///< Millisecond counter.
        if (now_ms - last_log_ms >= 250)  ///< Delay of 250 ms.
        {
            RC_PRINT_ALL(rclink_, RC); ///< Dump all RC roles with their current mapped values.
            last_log_ms = now_ms;      ///< Update time printed.
        }

        vTaskDelayUntil(&last, loop_ticks_); ///< Pace loop.
    }
}