/**
 * MIT License
 *
 * @brief Configuration file for all ESP32 parameters.
 *
 * @file app_config.h
 * @author Little Man Builds
 * @date 2025-08-01
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#pragma once

#include <Arduino.h>
#include <cstdint>

/**
 * @brief Debugging templates.
 */
#define DEBUGGING true

#if DEBUGGING
template <typename T>
inline void debug(const T &x) { Serial.print(x); }

template <typename T>
inline void debugln(const T &x) { Serial.println(x); }

// Overloads for float with precision.
inline void debug(float x, int digits) { Serial.print(x, digits); }
inline void debugln(float x, int digits) { Serial.println(x, digits); }
#else
template <typename T>
inline void debug(const T &) {}
template <typename T>
inline void debugln(const T &) {}
inline void debug(float, int) {}
inline void debugln(float, int) {}
#endif

/**
 * @brief Application-wide settings.
 */
namespace cfg
{
    constexpr int LOOP_INTERVAL_MS = 10;
    constexpr int LOOP_INTERVAL_TEST_SHORT = 100;
    constexpr int LOOP_INTERVAL_TEST_LONG = 1000;
    constexpr uint32_t BTN_DEBOUNCE_MS = 50;
    constexpr uint32_t BTN_SHORT_MS = 200;
    constexpr uint32_t BTN_LONG_MS = 1000;

    // ---- Motor (MCPWM) ---- //
    namespace motor
    {
        constexpr int RPWM_PIN = 37;
        constexpr int LPWM_PIN = 38;
        constexpr int EN_PIN = 39;
    } ///< Namespace motor.
} ///< Namespace cfg.

/**
 * @brief Application-defined button mapping.
 */
#define BUTTON_LIST(X) \
    X(TestButton1, 6)
