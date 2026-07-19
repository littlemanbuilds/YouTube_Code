/**
 * MIT License
 *
 * @brief Snapshot defining authority ownership and control limits.
 *
 * @file AuthorityBus.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-19
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <SnapshotBus.h>

#include <cstdint>

/**
 * @brief Authority sources for a control axis (per-axis ownership).
 * @note A source may be primary for the current mode, and still delegate control
 *       to another source on specific axes.
 */
enum class Owner : uint8_t
{
    None,    ///< Transitional or fault state (safe output only).
    RC,      ///< Remote control (may delegate per-axis).
    Direct,  ///< Direct child-operated analog control (accelerator, steering wheel).
    Joystick ///< Assistive joystick control (delegated).
};

/**
 * @brief Effective authority decision governing ownership and control limits.
 */
struct AuthoritySnapshot
{
    // ---- Decisions ---- //
    Owner owner_steer{Owner::None}; ///< Authority for steering.
    Owner owner_dir{Owner::None};   ///< Authority for direction (Fwd/Rev).
    Owner owner_speed{Owner::None}; ///< Authority for speed.
    Owner owner_power{Owner::None}; ///< Authority for power.

    // ---- Reasons (debug/telemetry) ---- //
    uint64_t stamp_us{0}; ///< Timestamp (us) captured when snapshot was produced.
};

// Type alias for the SnapshotBus that transports authority frames.
using AuthorityBus = snapshot::SnapshotBus<AuthoritySnapshot>;