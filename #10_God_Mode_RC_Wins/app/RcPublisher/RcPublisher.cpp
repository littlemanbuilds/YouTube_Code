/**
 * MIT License
 *
 * @brief Maps raw receiver channels into the project's named RC controls.
 *
 * @file RcPublisher.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-10-08
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#include "RcPublisher.h"

#include <Publisher.h>
#include <Clock.h>

// Short local name for the SnapshotRTOS namespace used below.
namespace srt = snapshot::rtos;

RcPublisher::RcPublisher(RcBus &bus, uint32_t period_ms) noexcept
    : bus_(bus), period_ms_{period_ms > 0u ? period_ms : 1u}
{
    configASSERT(period_ms > 0u);
}

void RcPublisher::begin() noexcept
{
    rclink_.begin(Serial2, cfg::rc::BAUD, cfg::rc::UART_RX, cfg::rc::UART_TX); // Start iBUS UART on Serial2.

    RC_CONFIG(RC, cfg);          // Create an RCLink configuration for the declared roles.
    RC_CFG_MAP_DEFAULT(RC, cfg); // Map those roles to receiver channels in declaration order.

    // Analog controls: raw receiver pulse widths become audience-friendly percentages.
    cfg.axis(RC::steering).raw(1000, 2000, 1500).deadband_us(8).out(-100.f, 100.f).done();
    cfg.axis(RC::direction).raw(1000, 2000, 1500).deadband_us(8).out(-100.f, 100.f).done();
    cfg.axis(RC::speed).raw(1000, 2000, 1000).deadband_us(8).out(0.f, 100.f).done();
    cfg.axis(RC::indicators).raw(1000, 2000, 1500).deadband_us(8).out(-100.f, 100.f).done();
    cfg.axis(RC::volume).raw(1000, 2000, 1500).deadband_us(4).out(0.f, 100.f).done();
    cfg.axis(RC::power).raw(1000, 2000, 1500).deadband_us(4).out(0.f, 100.f).done();

    // Discrete controls: translate two- and three-position switches into small numeric states.
    cfg.sw(RC::override).raw_levels({1000, 2000}).values({0.f, 1.f}).done();
    cfg.sw(RC::lights).raw_levels({1000, 2000}).values({0.f, 1.f}).done();
    cfg.sw(RC::mode).raw_levels({1000, 1500, 2000}).values({0.f, 1.f, 2.f}).done();
    cfg.sw(RC::obstacle).raw_levels({1000, 2000}).values({0.f, 1.f}).done();

    // If the link fails, neutralize every motion/control channel explicitly.
    cfg.setFailsafePolicy(RC::steering, rc::Failsafe::Mode::Value, 0);
    cfg.setFailsafePolicy(RC::direction, rc::Failsafe::Mode::Value, 0);
    cfg.setFailsafePolicy(RC::speed, rc::Failsafe::Mode::Value, 0);
    cfg.setFailsafePolicy(RC::indicators, rc::Failsafe::Mode::Value, 0);
    cfg.setFailsafePolicy(RC::volume, rc::Failsafe::Mode::Value, 0);
    cfg.setFailsafePolicy(RC::power, rc::Failsafe::Mode::Value, 0);
    cfg.setFailsafePolicy(RC::override, rc::Failsafe::Mode::Value, 1); // Preserve the safety override state.
    cfg.setFailsafePolicy(RC::lights, rc::Failsafe::Mode::Value, 0);
    cfg.setFailsafePolicy(RC::mode, rc::Failsafe::Mode::Value, 0); // Return delegation to Full RC.
    cfg.setFailsafePolicy(RC::obstacle, rc::Failsafe::Mode::Value, 0);

    // Declare the link stale quickly enough to stop motion before old commands linger.
    cfg.setLinkTimeout(50); // Use 50 ms instead of the 200 ms default.

    // Some receivers signal failsafe with a recognizable channel pattern. Require
    // that pattern for 50 ms (within ±2 raw units) before accepting it.
    RC_SET_FS_SIGNATURE_SELECTED(RC, rclink_, /* tol */ 2, /* hold_ms */ 50,
                                 {{RC::steering, +100},
                                  {RC::direction, +100},
                                  {RC::speed, +100},
                                  {RC::indicators, -100}});

    rclink_.apply_rxfs_outputs(true); // Apply RX failsafe outputs when the receiver indicates failsafe.
    rclink_.apply_config(cfg);        // Apply configuration.

    utils::publisher::start_cb<RcSnapshot>(
        bus_,                            // Target RcBus.
        &rclink_,                        // Context pointer (RcLink instance for callbacks).
        &RcPublisher::cb_update,         // Update callback (advance RcLink decoding).
        &RcPublisher::cb_read,           // Read callback (copy decoded channels into snapshot).
        &RcPublisher::cb_ok,             // Health callback (report failsafe/link validity).
        srt::AlwaysPublish{},            // Publish every tick.
        cfg::task::RcPublisher.name,     // Task name.
        cfg::task::RcPublisher.stack,    // Stack size.
        cfg::task::RcPublisher.priority, // Task priority.
        period_ms_,                      // Polling period in milliseconds.
        cfg::task::RcPublisher.core      // Core affinity.
    );
}
