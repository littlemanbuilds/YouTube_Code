/**
 * MIT License
 *
 * @brief MCU and peripheral pin assignments.
 *
 * @file AppIO.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-21
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
        constexpr uint8_t ADDR = 0x20; ///< I2C address for MCP23017 (0x20..0x27).

        // MCP virtual pin allocations (0..15).
        constexpr uint8_t swDIP1 = 0;    ///< GPIOA0.
        constexpr uint8_t swDIP2 = 1;    ///< GPIOA1.
        constexpr uint8_t swDIP3 = 2;    ///< GPIOA2.
        constexpr uint8_t swGearD3 = 3;  ///< GPIOA3 (shifter D3 contact / red, active-low via black common).
        constexpr uint8_t swGearFwd = 4; ///< GPIOA4 (forward contact / yellow, active-low via orange common; active in D1, D2, and D3).
        constexpr uint8_t swGearD1 = 5;  ///< GPIOA5 (shifter D1 contact / green, active-low via black common).
        constexpr uint8_t swGearR = 6;   ///< GPIOA6 (reverse contact / blue, active-low via orange common).
        // Park is derived in software.
        constexpr uint8_t btnAccel = 8; ///< GPIOB0.
        constexpr uint8_t btnHorn = 9;  ///< GPIOB1.
        constexpr uint8_t btnIndL = 10; ///< GPIOB2.
        constexpr uint8_t btnIndR = 11; ///< GPIOB3.

        /// @brief Pins configured as INPUT_PULLUP by MCP23017 setup.
        inline constexpr uint8_t INPUT_PINS[] = {
            swDIP1, swDIP2, swDIP3, swGearD3, swGearFwd, swGearD1,
            swGearR, btnAccel, btnHorn, btnIndL, btnIndR};
    } ///< namespace mcp23017

    /// @brief Switch banks (DIP + Gear).
    namespace switches
    {
        constexpr uint32_t DEBOUNCE_MS = 30;

        // DIP uses inputs (GPIOA0..GPIOA2).
        inline constexpr uint8_t DIP_KEYS[] = {
            cfg::mcp23017::swDIP1,
            cfg::mcp23017::swDIP2,
            cfg::mcp23017::swDIP3};

        // Gear selector uses inputs (GPIOA3..GPIOA6).
        inline constexpr uint8_t GEAR_KEYS[] = {
            cfg::mcp23017::swGearR,
            cfg::mcp23017::swGearFwd,
            cfg::mcp23017::swGearD1,
            cfg::mcp23017::swGearD3};

        inline constexpr uint8_t DIP_COUNT = static_cast<uint8_t>(std::size(DIP_KEYS));
        inline constexpr uint8_t GEAR_COUNT = static_cast<uint8_t>(std::size(GEAR_KEYS));

        // Contact masks indexed by position in GEAR_KEYS[] (physical contacts, not logical gears).
        //
        // bit 0 = Reverse contact (active-low via Orange common).
        // bit 1 = Forward/Drive contact (active-low via Orange common).
        // bit 2 = D1 modifier contact (active-low via Black common).
        // bit 3 = D3 modifier contact (active-low via Black common).
        //
        // Park and D2 do not have dedicated contacts. They are derived in software.
        constexpr uint8_t GEAR_R_MASK = static_cast<uint8_t>(1u << 0);
        constexpr uint8_t GEAR_FWD_MASK = static_cast<uint8_t>(1u << 1);
        constexpr uint8_t GEAR_D1_MASK = static_cast<uint8_t>(1u << 2);
        constexpr uint8_t GEAR_D3_MASK = static_cast<uint8_t>(1u << 3);

        // Value masks: each 1 marks a valid bit position in the corresponding key list.
        constexpr uint32_t DIP_VALUE_MASK = (1u << DIP_COUNT) - 1u;
        constexpr uint32_t GEAR_VALUE_MASK = (1u << GEAR_COUNT) - 1u;
    } ///< namespace switches

    /// @brief Motor(MCPWM)
    namespace motor
    {
        constexpr int RPWM_PIN = 37;
        constexpr int LPWM_PIN = 38;
        constexpr int EN_PIN = 39;
    } ///< namespace motor

    /// @brief Remote Control (RCLink / iBUS).
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
