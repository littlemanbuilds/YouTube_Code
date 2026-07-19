/**
 * MIT License
 *
 * @brief Defines how operating mode and gear limit the motor output available to the driver.
 *
 * @file AuthorityPolicy.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-19
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <ConfigBus.h>
#include <cstdint>

namespace cfg::authority
{
    // All caps use 0.0 for no output and 1.0 for full available output.
    constexpr float DEFAULT_SPEED_LIMIT = 1.0f;

    // ---- Operating-profile caps selected by the DIP switches ---- //
    constexpr float MODE_LIMIT_RC = 1.0f;
    constexpr float MODE_LIMIT_JOYSTICK = 1.0f;
    constexpr float MODE_LIMIT_OPEN = 1.0f;

    constexpr float MODE_LIMIT_MAX40 = 0.40f;
    constexpr float MODE_LIMIT_MAX55 = 0.55f;
    constexpr float MODE_LIMIT_MAX70 = 0.70f;
    constexpr float MODE_LIMIT_MAX85 = 0.85f;
    constexpr float MODE_LIMIT_MAX100 = 1.00f;

    // ---- Additional caps selected by the physical shifter ---- //

    // These limits apply to logical gear states.
    // Some logical gears, such as Park and D2, are derived by ConfigPublisher.
    constexpr float GEAR_LIMIT_D3 = 1.00f;
    constexpr float GEAR_LIMIT_D2 = 0.70f;
    constexpr float GEAR_LIMIT_D1 = 0.40f;
    constexpr float GEAR_LIMIT_P = 0.00f;
    constexpr float GEAR_LIMIT_R = 0.40f;

    // A gear that cannot be decoded must stop movement, just like Park.
    constexpr float GEAR_LIMIT_UNKNOWN = 0.00f; ///< Treat as Park until valid gear read.

    // ---- Policy helpers ---- //

    /**
     * @brief Return the normalized motor-output cap for the selected operating profile.
     */
    [[nodiscard]] inline float modeSpeedLimit(CarMode mode) noexcept
    {
        float limit = DEFAULT_SPEED_LIMIT;

        switch (mode)
        {
        case CarMode::RC:
            limit = MODE_LIMIT_RC;
            break;
        case CarMode::Joystick:
            limit = MODE_LIMIT_JOYSTICK;
            break;
        case CarMode::OpenSlot:
            limit = MODE_LIMIT_OPEN;
            break;

        case CarMode::Max40:
            limit = MODE_LIMIT_MAX40;
            break;
        case CarMode::Max55:
            limit = MODE_LIMIT_MAX55;
            break;
        case CarMode::Max70:
            limit = MODE_LIMIT_MAX70;
            break;
        case CarMode::Max85:
            limit = MODE_LIMIT_MAX85;
            break;
        case CarMode::Max100:
            limit = MODE_LIMIT_MAX100;
            break;

        default:
            break;
        }

        return limit;
    }

    /**
     * @brief Return the normalized motor-output cap for the selected logical gear.
     */
    [[nodiscard]] inline float gearSpeedLimit(Gear g) noexcept
    {
        float limit = GEAR_LIMIT_UNKNOWN;

        switch (g)
        {
        case Gear::D3:
            limit = GEAR_LIMIT_D3;
            break;
        case Gear::D2:
            limit = GEAR_LIMIT_D2;
            break;
        case Gear::D1:
            limit = GEAR_LIMIT_D1;
            break;
        case Gear::Park:
            limit = GEAR_LIMIT_P;
            break;
        case Gear::Rev:
            limit = GEAR_LIMIT_R;
            break;
        case Gear::Unknown:
        default:
            break;
        }

        return limit;
    }

} ///< namespace cfg::authority
