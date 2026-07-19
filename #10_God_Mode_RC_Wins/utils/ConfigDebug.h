/**
 * MIT License
 *
 * @brief Explains the current DIP mode and shifter contacts on the Serial Monitor.
 * @note This observes ConfigBus only; ConfigPublisher remains responsible for decoding.
 *
 * @file ConfigDebug.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-05-19
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AppIO.h>
#include <ConfigBus.h>
#include <Debugging.h>

#include <cstdint>

namespace utils::config_debug
{
    /**
     * @brief Convert a CarMode value into a readable string.
     *
     * @param mode Logical car mode from the configuration snapshot.
     * @return Static string representing the mode.
     */
    [[nodiscard]] inline const char *toString(CarMode mode) noexcept
    {
        switch (mode)
        {
        case CarMode::RC:
            return "RC";
        case CarMode::Joystick:
            return "Joystick";
        case CarMode::OpenSlot:
            return "OpenSlot";
        case CarMode::Max40:
            return "Max40";
        case CarMode::Max55:
            return "Max55";
        case CarMode::Max70:
            return "Max70";
        case CarMode::Max85:
            return "Max85";
        case CarMode::Max100:
            return "Max100";
        default:
            return "?";
        }
    }

    /**
     * @brief Convert a Gear value into a readable string.
     *
     * @param gear Logical gear value decoded by ConfigPublisher.
     * @return Static string representing the gear.
     */
    [[nodiscard]] inline const char *toString(Gear gear) noexcept
    {
        switch (gear)
        {
        case Gear::D3:
            return "D3";
        case Gear::D2:
            return "D2";
        case Gear::D1:
            return "D1";
        case Gear::Park:
            return "Park";
        case Gear::Rev:
            return "Rev";
        case Gear::Unknown:
            return "Unknown";
        default:
            return "?";
        }
    }

    /**
     * @brief Check whether the Reverse physical shifter contact is active.
     *
     * @param gear_raw Physical shifter contact bitfield from ConfigSnapshot.
     * @return True when the Reverse contact is active; otherwise false.
     */
    [[nodiscard]] inline bool contactR(uint8_t gear_raw) noexcept
    {
        return (gear_raw & cfg::switches::GEAR_R_MASK) != 0u;
    }

    /**
     * @brief Check whether the Forward/Drive physical shifter contact is active.
     * @note This is not a dedicated D2 contact. On the mapped 7-wire shifter,
     *       this contact is active in D1, D2, and D3.
     *
     * @param gear_raw Physical shifter contact bitfield from ConfigSnapshot.
     * @return True when the Forward/Drive contact is active; otherwise false.
     */
    [[nodiscard]] inline bool contactFwd(uint8_t gear_raw) noexcept
    {
        return (gear_raw & cfg::switches::GEAR_FWD_MASK) != 0u;
    }

    /**
     * @brief Check whether the D1 modifier contact is active.
     *
     * @param gear_raw Physical shifter contact bitfield from ConfigSnapshot.
     * @return True when the D1 modifier contact is active; otherwise false.
     */
    [[nodiscard]] inline bool contactD1(uint8_t gear_raw) noexcept
    {
        return (gear_raw & cfg::switches::GEAR_D1_MASK) != 0u;
    }

    /**
     * @brief Check whether the D3 modifier contact is active.
     *
     * @param gear_raw Physical shifter contact bitfield from ConfigSnapshot.
     * @return True when the D3 modifier contact is active; otherwise false.
     */
    [[nodiscard]] inline bool contactD3(uint8_t gear_raw) noexcept
    {
        return (gear_raw & cfg::switches::GEAR_D3_MASK) != 0u;
    }

    /**
     * @brief Check whether any visible configuration value has changed.
     * @note stamp_us is deliberately ignored. The timestamp can change even when
     *       the decoded configuration has not changed.
     *
     * @param a First configuration snapshot.
     * @param b Second configuration snapshot.
     *
     * @return True when decoded mode, decoded gear, raw DIP, raw gear contacts,
     *         or DIP lock state differs; otherwise false.
     */
    [[nodiscard]] inline bool changed(const ConfigSnapshot &a, const ConfigSnapshot &b) noexcept
    {
        return (a.mode != b.mode) ||
               (a.gear != b.gear) ||
               (a.dip_raw != b.dip_raw) ||
               (a.gear_raw != b.gear_raw) ||
               (a.dip_locked != b.dip_locked);
    }

    /**
     * @brief Print the decoded mode/gear alongside the underlying shifter contacts.
     *
     * @param s Configuration snapshot to print.
     */
    inline void log(const ConfigSnapshot &s) noexcept
    {
        const bool r = contactR(s.gear_raw);
        const bool fwd = contactFwd(s.gear_raw);
        const bool d1 = contactD1(s.gear_raw);
        const bool d3 = contactD3(s.gear_raw);

        debugfln("CONFIG: mode=%s gear=%s contacts[R=%u FWD=%u D1=%u D3=%u] dip_raw=%u gear_raw=0x%02X locked=%u",
                 toString(s.mode),
                 toString(s.gear),
                 r ? 1u : 0u,
                 fwd ? 1u : 0u,
                 d1 ? 1u : 0u,
                 d3 ? 1u : 0u,
                 static_cast<unsigned>(s.dip_raw),
                 static_cast<unsigned>(s.gear_raw),
                 s.dip_locked ? 1u : 0u);
    }

    /**
     * @brief Print a configuration snapshot only when it has meaningfully changed.
     *
     * @param prev Previously printed configuration snapshot (updated when a new snapshot is printed).
     * @param has_prev Tracks whether prev contains a valid previous snapshot (set to true after the first print).
     * @param cur Current configuration snapshot to compare and potentially print.
     */
    inline void logIfChanged(ConfigSnapshot &prev, bool &has_prev, const ConfigSnapshot &cur) noexcept
    {
        if (!has_prev || changed(prev, cur))
        {
            log(cur);
            prev = cur;
            has_prev = true;
        }
    }
} ///< namespace utils::config_debug
