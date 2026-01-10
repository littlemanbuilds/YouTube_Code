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

#include <cstddef>
#include <cstdint>
#include <array>
#include <app_config.h>
#include <SnapshotBus.h>

/**
 * @brief Type alias for the snapshotbus that transports RC input frames.
 */
using RcBus = snapshot::SnapshotBus<RcSnapshot>;

/**
 * @brief Application-owned RC snapshot payload transported on SnapshotBus.
 */
struct RcSnapshot
{
    std::array<float, static_cast<size_t>(RC::Count)> out{}; ///< Per-role mapped outputs (engineering units).
    bool failsafe{false};                                    ///< True if the link is in failsafe state.
    uint64_t stamp_us{0};                                    ///< Snapshot timestamp (µs since boot).
};

/**
 * @brief Read a single role value from an RcSnapshot.
 *
 * @param f Snapshot frame to read from.
 * @param role Logical RC role (enum value).
 * @return float Engineering-unit value for role (raw input units).
 */

[[nodiscard]] inline float rc_get(const RcSnapshot &f, RC role) noexcept
{
    return f.out[static_cast<size_t>(role)];
}

/**
 * @brief Single, shared RcBus instance (created on first use).
 */
namespace buses
{
    inline RcBus &rc() noexcept ///< Return refernce to the shared RcBus.
    {
        static RcBus bus{}; ///< One (only) RcBus instance.
        return bus;
    }
}
