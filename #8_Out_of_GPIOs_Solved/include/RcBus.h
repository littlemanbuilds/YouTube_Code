/**
 * MIT License
 *
 * @brief Snapshot payload and bus for RCLink → SnapshotBus integration.
 *
 * @file RcBus.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-10-25
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <SnapshotBus.h>

#include <array>
#include <cstddef>
#include <cstdint>

/**
 * @brief Current remote control input state and decoded commands.
 */
struct RcSnapshot
{
    std::array<float, static_cast<size_t>(RC::Count)> out{}; ///< Per-role mapped outputs (units depend on RCLink mapping).
    bool failsafe{false};                                    ///< True if the link is in failsafe state.
    uint64_t stamp_us{0};                                    ///< Timestamp (us) captured when snapshot was produced.
};

/**
 * @brief Read a single role value from an RcSnapshot.
 *
 * @param f Snapshot frame to read from.
 * @param role Logical RC role (enum value).
 * @return float Mapped value for the requested role (units depend on RCLink mapping).
 */
[[nodiscard]] inline float rc_get(const RcSnapshot &f, RC role) noexcept
{
    return f.out[static_cast<size_t>(role)];
}

// Type alias for the SnapshotBus that transports rc frames.
using RcBus = snapshot::SnapshotBus<RcSnapshot>;
