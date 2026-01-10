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
#include <cstdint> // Provides fixed-width integer types

/**
 * @brief Debugging macros.
 */
#define DEBUGGING true

#if DEBUGGING == true
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
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
}

/**
 * @brief Application-defined button mapping.
 */
#define BUTTON_LIST(X) \
    X(TestButton1, 6)
