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

#include <cstdint>
#include <SnapshotBus.h>

/**
 * @brief High-level intent produced by ControlCore.
 *
 * Services (motor, steering, lights, etc.) should consume this bus and NOT
 * consume raw input sources directly (InputBus / RcBus). That keeps policy
 * and authority decisions centralized.
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

    float throttle_cmd_pct{0.0f};            ///< 0..100 (%). Services may clamp.
    bool horn_cmd{false};                    ///< True if horn is pressed.
    Indicator indicator_cmd{Indicator::Off}; ///< Indicator mode.
    std::uint32_t stamp_ms{0};               ///< Timestamp (ms).
};

/**
 * @brief Type alias for the SnapshotBus that transports control frames.
 */
using ControlBus = snapshot::SnapshotBus<ControlSnapshot>;

/**
 * @brief Single, shared ControlBus instance.
 */
namespace buses
{
    inline ControlBus &control() noexcept ///< Return reference to the shared ControlBus.
    {
        static ControlBus bus{}; ///< One (only) ControlBus instance.
        return bus;              ///< Return reference to shared bus.
    }
}