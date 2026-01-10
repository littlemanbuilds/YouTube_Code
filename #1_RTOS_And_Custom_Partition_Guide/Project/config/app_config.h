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
    constexpr int LOOP_INTERVAL_TESTING = 1000;
}