/**
 * MIT License
 *
 * @brief Snapshot defining configuration state for mode and gear selection.
 *
 * @file ConfigBus.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-02
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <SnapshotBus.h>
#include <cstdint>

/**
 * @brief Logical operating mode selected via hardware configuration (optionally latched at startup).
 */
enum class CarMode : uint8_t
{
    RC,       ///< Full remote control mode.
    Joystick, ///< Joystick mode for disabled children.
    OpenSlot, ///< Placeholder.
    Max40,    ///< Direct (child) control, maximum 40% power.
    Max55,    ///< Direct (child) control, maximum 55% power.
    Max70,    ///< Direct (child) control, maximum 70% power.
    Max85,    ///< Direct (child) control, maximum 85% power.
    Max100    ///< Direct (child) control, maximum 100% power.
};

/**
 * @brief Logical gear selection derived from the physical contacts.
 * @note The 7-wire shifter does not provide separate Park or D2 contacts.
 *       Park and D2 are derived from Reverse, Forward, D1 and D3 contacts.
 */
enum class Gear : uint8_t
{
    D3,     ///< Drive 3.
    D2,     ///< Drive 2 (derived from Forward active with no D1/D3 modifier).
    D1,     ///< Drive 1.
    Park,   ///< Park (derived when Reverse and Forward are both inactive).
    Rev,    ///< Reverse.
    Unknown ///< Invalid/unknown contact combination.
};

/**
 * @brief Effective configuration state derived from hardware selectors.
 */
struct ConfigSnapshot
{
    // ---- Decoded semantics ---- //
    CarMode mode{CarMode::RC}; ///< Mode derived from dip_raw (0..7).
    Gear gear{Gear::Unknown};  ///< Logical gear derived from the physical shifter contacts.

    // ---- Raw switch fields ---- //
    uint8_t dip_raw{0};  ///< 3-bit DIP field (0..7), from GPIOA bits 0..2.
    uint8_t gear_raw{0}; ///< Physical shifter contact bits: bit0=R, bit1=FWD, bit2=D1, bit3=D3.

    // ---- Policy / metadata ---- //
    bool dip_locked{false}; ///< True once DIP mode has been latched; mode no longer changes.
    uint64_t stamp_us{0};   ///< Timestamp (us) captured when the snapshot was produced.
};

// Type alias for the SnapshotBus that transports config frames.
using ConfigBus = snapshot::SnapshotBus<ConfigSnapshot>;
