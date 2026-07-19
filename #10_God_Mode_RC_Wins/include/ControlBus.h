/**
 * MIT License
 *
 * @brief Carries the final commands passed from control policy to the actuator tasks.
 *
 * @file ControlBus.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-12-30
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <SnapshotBus.h>
#include <cstdint>

/**
 * @brief Requested car direction, independent of motor wiring.
 * @note Fwd/Rev describes vehicle motion. The motor driver maps it to CW/CCW
 *       according to the installed wiring.
 */
enum class DriveDir : uint8_t
{
    Fwd = 0,
    Rev = 1,
};

/**
 * @brief Source responsible for the current drive command.
 * @note PowerDriveHandler uses this value to stop before accepting drive demand
 *       from a different source.
 */
enum class DriveOwner : uint8_t
{
    None = 0,
    RC,
    Direct,
    Joystick
};

/**
 * @brief Resolved commands after ownership, freshness, and output caps are applied.
 */
struct ControlSnapshot
{
    /// @brief Indicator mode requested by the control core.
    enum class Indicator : uint8_t
    {
        Off = 0,
        Left,
        Right,
        Hazard
    };

    float throttle_cmd_pct{0.0f};             ///< Throttle command in percent (0..100).
    DriveDir dir_cmd{DriveDir::Fwd};          ///< Commanded drive direction when drive_enable is true.
    bool drive_enable{false};                 ///< True only when movement is intentionally requested.
    DriveOwner drive_owner{DriveOwner::None}; ///< Owner of the current drive command.
    bool horn_cmd{false};                     ///< True if horn is pressed.
    Indicator indicator_cmd{Indicator::Off};  ///< Indicator mode.
    uint32_t stamp_ms{0};                     ///< Timestamp (ms) captured when snapshot was produced.
};

// Thread-safe bus carrying the latest complete command set to the actuator layer.
using ControlBus = snapshot::SnapshotBus<ControlSnapshot>;
