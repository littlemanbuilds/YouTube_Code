/**
 * MIT License
 *
 * @brief Converts internal authority and configuration values into short display labels.
 * @note These labels are for people, not control logic, stored data, or communication
 *       protocols. They may change whenever readability improves.
 *
 * @file Diagnostic.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-19
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AuthorityBus.h>
#include <ConfigBus.h>

namespace utils::diag
{
    /**
     * @brief Compact owner token for HUD / serial output.
     *
     * @param o Authority owner.
     * @return Short diagnostic token: RC, CAR, JOY, or NONE.
     */
    [[nodiscard]] inline const char *owner_tok(Owner o) noexcept
    {
        switch (o)
        {
        case Owner::RC:
            return "RC"; // Remote control.
        case Owner::Direct:
            return "CAR"; // On-car child-operated controls.
        case Owner::Joystick:
            return "JOY"; // Assistive joystick.
        case Owner::None:
        default:
            return "NONE"; // Transitional / fault / no active owner.
        }
    }

    /**
     * @brief Compact car mode token for HUD / serial output.
     *
     * @param m Car mode / DIP profile.
     * @return Short mode token.
     */
    [[nodiscard]] inline const char *carmode_tok(CarMode m) noexcept
    {
        switch (m)
        {
        case CarMode::RC:
            return "RC";
        case CarMode::Joystick:
            return "JOY";
        case CarMode::OpenSlot:
            return "OPEN";
        case CarMode::Max40:
            return "MAX40";
        case CarMode::Max55:
            return "MAX55";
        case CarMode::Max70:
            return "MAX70";
        case CarMode::Max85:
            return "MAX85";
        case CarMode::Max100:
            return "MAX100";
        default:
            return "ERR";
        }
    }

    /**
     * @brief Compact gear token for HUD / serial output.
     *
     * @param g Gear selection.
     * @return Short gear token.
     */
    [[nodiscard]] inline const char *gear_tok(Gear g) noexcept
    {
        switch (g)
        {
        case Gear::Park:
            return "PARK";
        case Gear::Rev:
            return "REV";
        case Gear::D1:
            return "D1";
        case Gear::D2:
            return "D2";
        case Gear::D3:
            return "D3";
        default:
            return "ERR";
        }
    }

    /**
     * @brief Compact delegation token for HUD / serial output.
     *
     * @param d RC delegation selection.
     * @return Short token: FULL, MIX, DELG, or ERR.
     */
    [[nodiscard]] inline const char *rcdel_tok(RcDelegation d) noexcept
    {
        switch (d)
        {
        case RcDelegation::FullRC:
            return "FULL";
        case RcDelegation::Mixed:
            return "MIX";
        case RcDelegation::Delegated:
            return "DELG";
        default:
            return "ERR";
        }
    }

    /**
     * @brief RC delegation token for Authority HUD output.
     *
     * @param mode Current car mode.
     * @param d RC delegation selection.
     * @return FULL, MIX, DELG, or N/A when RC delegation is not relevant.
     */
    [[nodiscard]] inline const char *rcdel_hud_tok(CarMode mode, RcDelegation d) noexcept
    {
        const bool rc_delegation_relevant =
            (mode == CarMode::RC) ||
            (mode == CarMode::Joystick);

        if (!rc_delegation_relevant)
        {
            return "N/A";
        }

        return rcdel_tok(d);
    }
} ///< namespace utils::diag
