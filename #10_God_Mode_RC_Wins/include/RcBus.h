/**
 * MIT License
 *
 * @brief Carries named RC controls, receiver health, and sample time.
 *
 * @file RcBus.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-10-25
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <SnapshotBus.h>

#include <array>
#include <cstddef>
#include <cstdint>

/**
 * @brief Latest receiver frame after RCLink maps channels to project roles.
 */
struct RcSnapshot
{
    std::array<float, static_cast<size_t>(RC::Count)> out{}; ///< Mapped role outputs; units depend on RCLink mapping.
    bool failsafe{false};                                    ///< True if the link is in failsafe state.
    uint64_t stamp_us{0};                                    ///< Timestamp (us) captured when snapshot was produced.
};

/**
 * @brief Read a single role value from an RcSnapshot.
 *
 * @param f Snapshot frame to read from.
 * @param role Logical RC role (enum value).
 * @return Mapped value for the requested role (units depend on RCLink mapping).
 */
[[nodiscard]] inline float rc_get(const RcSnapshot &f, RC role) noexcept
{
    return f.out[static_cast<size_t>(role)];
}

// Thread-safe bus carrying the latest complete remote-control frame.
using RcBus = snapshot::SnapshotBus<RcSnapshot>;
