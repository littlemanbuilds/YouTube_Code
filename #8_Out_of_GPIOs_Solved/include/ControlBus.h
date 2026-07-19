/**
 * MIT License
 *
 * @brief Snapshot payload and bus for high-level control commands.
 *
 * @file ControlBus.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-12-30
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#pragma once

#include <SnapshotBus.h>
#include <cstdint>

/**
 * @brief Direction command for drivetrain.
 * @note This is a *semantic* drive direction (Fwd/Rev), not a motor electrical polarity.
 *       Motor drivers can map this to CW/CCW as required by wiring.
 */
enum class DriveDir : std::uint8_t
{
    Fwd = 0,
    Rev = 1,
};

/**
 * @brief Control intent derived from inputs and authority limits.
 */
struct ControlSnapshot
{
    /// @brief Indicator mode requested by the control core.
    enum class Indicator : std::uint8_t
    {
        Off = 0,
        Left,
        Right,
        Hazard
    };

    float throttle_cmd_pct{0.0f};            ///< Throttle command in percent (0..100).
    DriveDir dir_cmd{DriveDir::Fwd};         ///< Commanded drive direction.
    bool horn_cmd{false};                    ///< True if horn is pressed.
    Indicator indicator_cmd{Indicator::Off}; ///< Indicator mode.
    std::uint32_t stamp_ms{0};               ///< Timestamp (ms) captured when snapshot was produced.
};

// Type alias for the SnapshotBus that transports control frames.
using ControlBus = snapshot::SnapshotBus<ControlSnapshot>;
