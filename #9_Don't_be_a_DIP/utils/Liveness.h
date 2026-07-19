/**
 * MIT License
 *
 * @brief Timestamp freshness helpers for SnapshotBus frames.
 * @note Provides wrap-safe age checks for millisecond timestamps (e.g., millis()).
 *       Use with cfg::safety::stale::* thresholds to implement consistent “fresh + healthy”
 *       gating in safety-critical consumers.
 *
 * @file Liveness.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-19
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <cstdint>

/**
 * @brief Liveness helpers for timestamped SnapshotBus frames.
 * @note Uses unsigned wrap-safe subtraction. Works across millis() rollover.
 */
namespace utils::liveness
{
    // ---- Millisecond stamps (wrap-safe) ---- //

    /**
     * @brief Check freshness of a millisecond timestamp (wrap-safe).
     *
     * @param now_ms Current time in milliseconds.
     * @param stamp_ms Timestamp to check (milliseconds).
     * @param max_age_ms Maximum allowed age (milliseconds).
     */
    [[nodiscard]] inline bool fresh_ms(uint32_t now_ms,
                                       uint32_t stamp_ms,
                                       uint32_t max_age_ms) noexcept
    {
        return static_cast<uint32_t>(now_ms - stamp_ms) <= max_age_ms;
    }

    template <typename FrameT>
    [[nodiscard]] inline bool fresh_ms(uint32_t now_ms,
                                       const FrameT &f,
                                       uint32_t max_age_ms) noexcept
    {
        return fresh_ms(now_ms, static_cast<uint32_t>(f.stamp_ms), max_age_ms);
    }

    // ---- Microsecond stamps (monotonic) ---- //

    /**
     * @brief Check freshness of a microsecond timestamp (monotonic clock).
     * @note max_age_ms is specified in milliseconds and internally converted to microseconds.
     *
     * @param now_us Current time in microseconds.
     * @param stamp_us Timestamp to check (microseconds).
     * @param max_age_ms Maximum allowed age (milliseconds).
     */
    [[nodiscard]] inline bool fresh_us(uint64_t now_us,
                                       uint64_t stamp_us,
                                       uint32_t max_age_ms) noexcept
    {
        const uint64_t max_age_us = static_cast<uint64_t>(max_age_ms) * 1000u;
        return (now_us >= stamp_us) && ((now_us - stamp_us) <= max_age_us);
    }

    template <typename FrameT>
    [[nodiscard]] inline bool fresh_us(uint64_t now_us,
                                       const FrameT &f,
                                       uint32_t max_age_ms) noexcept
    {
        return fresh_us(now_us, static_cast<uint64_t>(f.stamp_us), max_age_ms);
    }
} ///< namespace utils::liveness
