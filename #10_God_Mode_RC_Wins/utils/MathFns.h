/**
 * MIT License
 *
 * @brief Small deterministic math helpers shared by control and diagnostics.
 * @note These helpers are deliberately small, deterministic, and side-effect free.
 *       They should remain generic math utilities rather than application policy.
 *
 * @file MathFns.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-19
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
     * @return Clamped value.
     */
    template <typename T>
    constexpr T clamp(T value, T low, T high) noexcept
    {
        return (value < low)    ? low
               : (value > high) ? high
                                : value;
    }

    // ---- Keep percentages and normalized values visibly separate ---- //

    /**
     * @note These named conversions prevent a 0..100 percentage from being
     *       accidentally treated as a normalized 0..1 cap, or vice versa.
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
