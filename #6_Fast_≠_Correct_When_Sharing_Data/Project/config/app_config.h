/**
 * MIT License
 *
 * @brief Configuration file for all ESP32 parameters.
 *
 * @file app_config.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-08-01
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#pragma once

#include <Arduino.h>
#include <cstdint>
#include <RCLink.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

// ---- Debugging templates ---- //

#define DEBUGGING true

#if DEBUGGING
template <typename T>
inline void debug(const T &x) { Serial.print(x); }

template <typename T>
inline void debugln(const T &x) { Serial.println(x); }

// Overloads for float with precision.
inline void debug(float x, int digits) { Serial.print(x, digits); }
inline void debugln(float x, int digits) { Serial.println(x, digits); }

// printf-style debug macros.
#define debugf(...) Serial.printf(__VA_ARGS__)
#define debugfln(fmt, ...) Serial.printf(fmt "\n", ##__VA_ARGS__)

#else
template <typename T>
inline void debug(const T &) {}
template <typename T>
inline void debugln(const T &) {}
inline void debug(float, int) {}
inline void debugln(float, int) {}
#define debugf(...)
#define debugfln(...)
#endif

// ---- SnapshotBus scheduling parameters ---- //

#ifndef SNAPSHOTBUS_SPIN_LIMIT
// Bound reader spin before yielding; 32–128 is typical for ESP32.
#define SNAPSHOTBUS_SPIN_LIMIT 64
#endif

#ifndef SNAPSHOTBUS_YIELD
// Yield only when NOT in an ISR (safe for FreeRTOS). Keeps readers fair under contention.
static inline void snapshotbus_maybe_yield()
{
    if (!xPortInIsrContext())
    {
        taskYIELD();
    }
}
#define SNAPSHOTBUS_YIELD() snapshotbus_maybe_yield()
#endif

// ---- Timebase ---- //

/**
 * @brief Monotonic microsecond clock (preferred for stamps & durations).
 * @note Uses esp_timer_get_time() under Arduino-ESP32 (64-bit, monotonic).
 */
inline uint64_t now_us() { return esp_timer_get_time(); }

/// @brief Convenience 32-bit millisecond time (wraps ~49 days).
inline uint32_t now_ms32() { return static_cast<uint32_t>(now_us() / 1000ULL); }

/// @brief Convert milliseconds to FreeRTOS ticks.
inline TickType_t to_ticks_ms(uint32_t ms) { return pdMS_TO_TICKS(ms); }

// ---- Application configuration settings ---- //
namespace cfg
{
    // ---- Canonical task cadences ---- //
    namespace tick
    {
        constexpr uint32_t LOOP_MS = 10;                   ///< Standard loop cadence.
        constexpr uint32_t LOOP_INTERVAL_TEST_SHORT = 100; ///< Short test ms.
        constexpr uint32_t LOOP_INTERVAL_TEST_LONG = 1000; ///< Long test ms.
    } ///< Namespace tick.

    // ---- Button Timings ---- //
    namespace button
    {
        constexpr uint32_t BTN_DEBOUNCE_MS = 50;
        constexpr uint32_t BTN_SHORT_MS = 200;
        constexpr uint32_t BTN_LONG_MS = 1000;
    } ///< Namespace button.

    // ---- Motor (MCPWM) ---- //
    namespace motor
    {
        constexpr int RPWM_PIN = 37;
        constexpr int LPWM_PIN = 38;
        constexpr int EN_PIN = 39;
    } ///< Namespace motor.

    // ---- Remote Control (RCLink) ---- //
    namespace rc
    {
        constexpr int UART_RX = 18;       ///< iBUS data in.
        constexpr int UART_TX = -1;       ///< Not required for iBUS (disabled).
        constexpr uint32_t BAUD = 115200; ///< iBUS baud rate.
    } ///< Namepsace rc.
} ///< Namespace cfg.

// ---- Application button mapping ---- //
#define BUTTON_LIST(X) \
    X(Accelerator, 6)

// ---- Remote control channel mapping ---- //
#define RC_ROLES(X)             \
    X(steering)   /* Ch1_RH */  \
    X(direction)  /* Ch2_RV */  \
    X(speed)      /* Ch3_LV */  \
    X(indicators) /* Ch4_LH */  \
    X(volume)     /* Ch5_VrA */ \
    X(power)      /* Ch6_VrB */ \
    X(override)   /* Ch7_SwA */ \
    X(lights)     /* Ch8_SwB */ \
    X(mode)       /* Ch9_SwC */ \
    X(obstacle)   /* Ch10_SwD */

RC_DECLARE_ROLES(RC, RC_ROLES) ///< RCLink enum builder.