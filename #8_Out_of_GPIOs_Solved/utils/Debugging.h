/**
 * MIT License
 *
 * @brief Serial-based debugging output helpers.
 *
 * @note Provides lightweight wrappers around Arduino Serial printing that
 *       compile to no-ops when debugging is disabled.
 *
 * @file Debugging.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2026-01-19
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <Arduino.h>
#include <AppConfig.h>

#if DEBUGGING

/**
 * @brief Print a value to Serial (no newline).
 * @note Requires Serial to be initialized (Serial.begin()).
 */
template <typename T>
inline void debug(const T &x) noexcept
{
    Serial.print(x);
}

/**
 * @brief Print a value to Serial followed by newline.
 * @note Requires Serial to be initialized (Serial.begin()).
 */
template <typename T>
inline void debugln(const T &x) noexcept
{
    Serial.println(x);
}

/**
 * @brief Print a float with explicit precision.
 * @note Requires Serial to be initialized (Serial.begin()).
 */
inline void debug(float x, int digits) noexcept
{
    Serial.print(x, digits);
}

/**
 * @brief Print a float with precision followed by newline.
 * @note Requires Serial to be initialized (Serial.begin()).
 */
inline void debugln(float x, int digits) noexcept
{
    Serial.println(x, digits);
}

/**
 * @brief printf-style formatted debug output (no newline).
 * @note Requires Serial to be initialized (Serial.begin()).
 */
#define debugf(...) Serial.printf(__VA_ARGS__)

/**
 * @brief printf-style formatted debug output with newline.
 * @note Requires Serial to be initialized (Serial.begin()).
 */
#define debugfln(fmt, ...) Serial.printf(fmt "\n", ##__VA_ARGS__)

#else // DEBUGGING disabled.

template <typename T>
inline void debug(const T &) noexcept {}

template <typename T>
inline void debugln(const T &) noexcept {}

inline void debug(float, int) noexcept {}
inline void debugln(float, int) noexcept {}

#define debugf(...)
#define debugfln(...)

#endif
