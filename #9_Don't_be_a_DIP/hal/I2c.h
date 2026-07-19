/**
 * MIT License
 *
 * @brief I2C initialization helper (composition root owns Wire.begin()).
 *
 * @file I2c.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-20
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <Wire.h>

/**
 * @brief Hardware abstraction helpers.
 */
namespace hal::i2c
{
    /**
     * @brief Initialize the I2C (Wire) bus with configured SDA/SCL pins.
     * @note Must be called in setup() before any I2C peripherals are initialized or polled.
     */
    inline void begin() noexcept
    {
        Wire.begin(cfg::pins::I2C_SDA, cfg::pins::I2C_SCL);
    }
} ///< namespace hal::i2c