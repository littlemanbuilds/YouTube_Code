/**
 * MIT License
 *
 * @brief MCU and peripheral pin assignments.
 *
 * @file AppIO.h
 * @author Little Man Builds
 * @date 2026-01-21
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <RCLink.h>

#include <cstddef>
#include <cstdint>

namespace cfg
{
    /// @brief Pins (MCU pin map).
    namespace pins
    {
        constexpr int I2C_SDA = 8;
        constexpr int I2C_SCL = 9;
    } ///< namespace pins

    /// @brief Devices (I2C peripherals, etc.).
    namespace mcp23017
    {
        constexpr std::uint8_t ADDR = 0x20; ///< I2C address for MCP23017 (0x20..0x27).

        constexpr uint8_t btnAccel = 8; ///< GPIOB0.
        constexpr uint8_t btnHorn = 9;  ///< GPIOB1.
        constexpr uint8_t btnIndL = 10; ///< GPIOB2.
        constexpr uint8_t btnIndR = 11; ///< GPIOB3.

        /// @brief Pins configured as INPUT_PULLUP by MCP23017 setup.
        inline constexpr uint8_t INPUT_PINS[] = {btnAccel, btnHorn, btnIndL, btnIndR};
    }

    // ---- Motor(MCPWM) ---- //
    namespace motor
    {
        constexpr int RPWM_PIN = 37;
        constexpr int LPWM_PIN = 38;
        constexpr int EN_PIN = 39;
    } ///< namespace motor

    // ---- Remote Control (RCLink / iBUS) ---- //
    namespace rc
    {
        constexpr int UART_RX = 18;       ///< iBUS data in.
        constexpr int UART_TX = -1;       ///< Not required for iBUS (disabled).
        constexpr uint32_t BAUD = 115200; ///< iBUS baud rate.
    } ///< namespace rc
} ///< namespace cfg

// ---- Application mappings ---- //

/**
 * @brief Application button mapping.
 */
#define BUTTON_LIST(X)                       \
    X(Accelerator, cfg::mcp23017::btnAccel)  \
    X(Horn, cfg::mcp23017::btnHorn)          \
    X(IndicatorLeft, cfg::mcp23017::btnIndL) \
    X(IndicatorRight, cfg::mcp23017::btnIndR)

/**
 * @brief Remote control channel mapping.
 */
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