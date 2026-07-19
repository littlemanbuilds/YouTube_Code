/**
 * MIT License
 *
 * @brief Carries the operating profile and logical gear decoded from physical switches.
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
 * @brief Operating profile selected by the three DIP switches and locked at boot.
 */
enum class CarMode : uint8_t
{
    RC,       ///< RC-primary control profile.
    Joystick, ///< Joystick-primary assistive control profile.
    OpenSlot, ///< Reserved direct-control profile for future expansion.
    Max40,    ///< Direct control with a maximum 40% drive-output cap.
    Max55,    ///< Direct control with a maximum 55% drive-output cap.
    Max70,    ///< Direct control with a maximum 70% drive-output cap.
    Max85,    ///< Direct control with a maximum 85% drive-output cap.
    Max100    ///< Direct control with a maximum 100% drive-output cap.
};

/**
 * @brief Gear position decoded from the shifter's physical contacts.
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
 * @brief One complete view of the decoded and raw configuration switches.
 */
struct ConfigSnapshot
{
    // ---- Decoded meaning ---- //
    CarMode mode{CarMode::RC}; ///< Mode derived from dip_raw (0..7).
    Gear gear{Gear::Unknown};  ///< Logical gear derived from the physical shifter contacts.

    // ---- Raw switch fields ---- //
    uint8_t dip_raw{0};  ///< 3-bit DIP field (0..7), from GPIOA bits 0..2.
    uint8_t gear_raw{0}; ///< Physical shifter contact bits: bit0=R, bit1=FWD, bit2=D1, bit3=D3.

    // ---- Policy / metadata ---- //
    bool dip_locked{false}; ///< True once DIP mode has been latched; mode no longer changes.
    uint64_t stamp_us{0};   ///< Timestamp (us) captured when the snapshot was produced.
};

// Thread-safe bus carrying the latest complete configuration state.
using ConfigBus = snapshot::SnapshotBus<ConfigSnapshot>;
