/**
 * MIT License
 *
 * @brief Implementation of RC publisher (iBUS → RcLink → SnapshotBus).
 *
 * @file RcPublisher.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-10-08
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#include "RcPublisher.h"

// Short alias for SnapshotRTOS helpers.
namespace srt = snapshot::rtos;

// Constructor.
RcPublisher::RcPublisher(uint32_t period_ms, float epsilon, uint32_t min_interval_ms) noexcept
    : period_ms_{period_ms}, eps_{epsilon}, min_interval_ms_{min_interval_ms}
{
}

// Configure RCLink (axes, switches, etc.).
void RcPublisher::begin() noexcept
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

    // Failsafe policies.
    cfg.setFailsafePolicy(RC::steering, rc::Failsafe::Mode::Value, 0);
    cfg.setFailsafePolicy(RC::direction, rc::Failsafe::Mode::Value, 0);
    cfg.setFailsafePolicy(RC::speed, rc::Failsafe::Mode::Value, 0);
    cfg.setFailsafePolicy(RC::indicators, rc::Failsafe::Mode::Value, 0);
    cfg.setFailsafePolicy(RC::volume, rc::Failsafe::Mode::Value, 0);
    cfg.setFailsafePolicy(RC::power, rc::Failsafe::Mode::Value, 0);
    cfg.setFailsafePolicy(RC::override, rc::Failsafe::Mode::Value, 1); ///< Override car settings.
    cfg.setFailsafePolicy(RC::lights, rc::Failsafe::Mode::Value, 0);
    cfg.setFailsafePolicy(RC::mode, rc::Failsafe::Mode::Value, 0); ///< Default mode.
    cfg.setFailsafePolicy(RC::obstacle, rc::Failsafe::Mode::Value, 0);

    // Link-level failsafe timing.
    cfg.setLinkTimeout(50); ///< 50 ms instead of default (200 ms).

    // Receiver failsafe signature (±2, hold 50 ms).
    RC_SET_FS_SIGNATURE_SELECTED(RC, rclink_, /* tol */ 2, /* hold_ms */ 50,
                                 {{RC::steering, +100},
                                  {RC::direction, +100},
                                  {RC::speed, +100},
                                  {RC::indicators, -100}});

    rclink_.apply_rxfs_outputs(true); ///< Apply RX failsafe outputs when RX indicates failsafe.

    rclink_.apply_config(cfg); ///< Apply configuration.

    RcPublisher::Reader reader{&rclink_}; ///< Adapter: RcLink → float channels for the publisher.

    srt::Policy pol{};  ///< Publishing policy (change gate + min interval).
    pol.epsilon = eps_; ///< Only publish if a channel moves > epsilon.
    pol.min_interval_us = (min_interval_ms_ > 0)
                              ? static_cast<uint32_t>(min_interval_ms_) * 1000u ///< Conversion: works in microseconds.
                              : 0u;                                             ///< Or always publish if set to 0.

    srt::start_publisher<static_cast<size_t>(RC::Count), RcSnapshot>(
        buses::rc(), ///< The bus that snapshots flow into.
        reader,      ///< RcLink-backed reader.
        pol,         ///< Publish policy.
        "RcPub",     ///< Task name (shows up in FreeRTOS debug).
        4096,        ///< Stack size (words → ~16 KB).
        2,           ///< Task priority.
        period_ms_   ///< Polling period in milliseconds.
    );
}