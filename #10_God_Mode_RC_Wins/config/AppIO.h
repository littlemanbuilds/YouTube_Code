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
    /// @brief ESP32 pin assignments.
    namespace pins
    {
        constexpr int I2C_SDA = 8;
        constexpr int I2C_SCL = 9;
    } ///< namespace cfg::pins

    /// @brief MCP23017 address and virtual input-pin assignments.
    namespace mcp23017
    {
        constexpr uint8_t ADDR = 0x20; ///< I2C address for MCP23017 (0x20..0x27).

        // Virtual pins 0..7 are port A; 8..15 are port B.
        constexpr uint8_t swDIP1 = 0;    ///< GPIOA0.
        constexpr uint8_t swDIP2 = 1;    ///< GPIOA1.
        constexpr uint8_t swDIP3 = 2;    ///< GPIOA2.
        constexpr uint8_t swGearD3 = 3;  ///< GPIOA3 (shifter D3 contact / red, active-low via black common).
        constexpr uint8_t swGearFwd = 4; ///< GPIOA4 (forward/yellow, active-low; active in D1/D2/D3).
        constexpr uint8_t swGearD1 = 5;  ///< GPIOA5 (shifter D1 contact / green, active-low via black common).
        constexpr uint8_t swGearR = 6;   ///< GPIOA6 (reverse contact / blue, active-low via orange common).
        // Park has no dedicated wire; it is decoded from the contact combination.
        constexpr uint8_t btnAccel = 8; ///< GPIOB0.
        constexpr uint8_t btnHorn = 9;  ///< GPIOB1.
        constexpr uint8_t btnIndL = 10; ///< GPIOB2.
        constexpr uint8_t btnIndR = 11; ///< GPIOB3.

        /// @brief Pins configured as INPUT_PULLUP by MCP23017 setup.
        inline constexpr uint8_t INPUT_PINS[] = {
            swDIP1, swDIP2, swDIP3, swGearD3, swGearFwd, swGearD1,
            swGearR, btnAccel, btnHorn, btnIndL, btnIndR};
    } ///< namespace cfg::mcp23017

    /// @brief Switch-bank wiring and bit masks.
    namespace switches
    {
        constexpr uint32_t DEBOUNCE_MS = 30;

        // The three DIP switches select the operating profile.
        inline constexpr uint8_t DIP_KEYS[] = {
            cfg::mcp23017::swDIP1,
            cfg::mcp23017::swDIP2,
            cfg::mcp23017::swDIP3};

        // Four physical contacts encode the five logical shifter positions.
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
    } ///< namespace cfg::switches

    /// @brief Motor-driver MCPWM pins.
    namespace motor
    {
        constexpr int RPWM_PIN = 37;
        constexpr int LPWM_PIN = 38;
        constexpr int EN_PIN = 39;
    } ///< namespace cfg::motor

    /// @brief RC receiver UART (RCLink/iBUS).
    namespace rc
    {
        constexpr int UART_RX = 18;       ///< iBUS data in.
        constexpr int UART_TX = -1;       ///< Not required for iBUS (disabled).
        constexpr uint32_t BAUD = 115200; ///< iBUS baud rate.
    } ///< namespace cfg::rc
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
 * @brief Map receiver channels to controls named for their purpose.
 */
#define RC_ROLES(X)             \
    X(steering)   /* Ch1: right stick horizontal */ \
    X(direction)  /* Ch2: right stick vertical */   \
    X(speed)      /* Ch3: left stick vertical */    \
    X(indicators) /* Ch4: left stick horizontal */  \
    X(volume)     /* Ch5: knob A */                  \
    X(power)      /* Ch6: knob B */                  \
    X(override)   /* Ch7: switch A */                \
    X(lights)     /* Ch8: switch B */                \
    X(mode)       /* Ch9: three-position switch C */ \
    X(obstacle)   /* Ch10: switch D */

RC_DECLARE_ROLES(RC, RC_ROLES) ///< RCLink enum builder.
