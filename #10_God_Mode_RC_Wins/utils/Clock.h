/**
 * MIT License
 *
 * @brief Provides one monotonic clock for timestamps and stale-data checks.
 * @note The ESP high-resolution timer only moves forward, so it is suitable for
 *       measuring age without depending on wall-clock time.
 *
 * @file Clock.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-19
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <esp_timer.h>
#include <cstdint>

namespace utils::clock
{
    /// @brief Monotonic time in microseconds since boot.
    [[nodiscard]] inline uint64_t now_us() noexcept
    {
        return static_cast<uint64_t>(esp_timer_get_time());
    }

    /// @brief Monotonic time in milliseconds since boot.
    [[nodiscard]] inline uint32_t now_ms() noexcept
    {
        return static_cast<uint32_t>(now_us() / 1000ULL);
    }
} ///< namespace utils::clock
