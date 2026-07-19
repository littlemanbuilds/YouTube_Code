/**
 * MIT License
 *
 * @brief Project-wide monotonic time access helpers.
 *
 * @note Provides lightweight, inline access to a monotonic time source
 *       based on the ESP high-resolution timer.
 *
 * @file Clock.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2026-01-19
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
