/**
 * MIT License
 *
 * @brief Math utility functions for common numeric operations.
 *
 * @file MathFns.h
 * @author Little Man Builds
 * @date 2026-01-19
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <cmath>

namespace utils::mathfns
{
    /**
     * @brief Approximate equality for floats (primarily for diagnostics/logging).
     * @note Avoids noisy 'changed' messages caused by tiny float rounding differences.
     */
    [[nodiscard]] inline bool approx_equal(float a, float b, float eps = 1e-3f) noexcept
    {
        return std::fabs(a - b) <= eps;
    }

    /**
     * @brief Clamp a value between a lower and upper bound.
     *
     * @tparam T Numeric or comparable type.
     * @param value Input value to clamp.
     * @param low Minimum allowable value.
     * @param high Maximum allowable value.
     * @return T The clamped value.
     */
    template <typename T>
    constexpr T clamp(T value, T low, T high) noexcept
    {
        return (value < low)    ? low
               : (value > high) ? high
                                : value;
    }

    // ---- Unit-hardening helpers ---- //

    /**
     * @note Intent:
     *        - Make it trivial to "do the right thing" for percent vs normalized values.
     *        - Prevent accidental clamps of percent values into 0..1 ranges (or vice versa).
     *
     *       Naming convention:
     *        - *_pct  => percent domain [0..MAX_PCT]
     *        - *_01   => normalized domain [0..1]
     */

    /// @brief Clamp a percent value to [0..MAX_PCT].
    [[nodiscard]] constexpr float clamp_pct(float value_pct) noexcept
    {
        return clamp(value_pct, cfg::units::MIN, cfg::units::MAX_PCT);
    }

    /// @brief Clamp a normalized value to [0..1].
    [[nodiscard]] constexpr float clamp_01(float value_01) noexcept
    {
        return clamp(value_01, cfg::units::MIN, cfg::units::MAX);
    }

    /// @brief Convert percent [0..MAX_PCT] to normalized [0..1] (input is clamped).
    [[nodiscard]] constexpr float pct_to_01(float value_pct) noexcept
    {
        // Defensive: if MAX_PCT were ever configured strangely, avoid division by zero.
        return (cfg::units::MAX_PCT > 0.0f) ? (clamp_pct(value_pct) / cfg::units::MAX_PCT) : 0.0f;
    }

    /// @brief Convert normalized [0..1] to percent [0..MAX_PCT] (input is clamped).
    [[nodiscard]] constexpr float norm01_to_pct(float value_01) noexcept
    {
        return clamp_01(value_01) * cfg::units::MAX_PCT;
    }

    /// @brief Multiply a percent command by a normalized cap and clamp back to percent.
    [[nodiscard]] constexpr float apply_cap_pct(float value_pct, float cap_01) noexcept
    {
        return clamp_pct(value_pct * clamp_01(cap_01));
    }
} ///< namespace utils::mathfns
