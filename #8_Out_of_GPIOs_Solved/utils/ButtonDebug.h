/**
 * MIT License
 *
 * @brief Debug helpers for button snapshots (human-readable edge logging).
 *
 * @file ButtonDebug.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2026-01-23
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <ButtonBus.h>
#include <Debugging.h>

namespace utils::button_debug
{
    /**
     * @brief Print human-readable button edges (press/release) between two button snapshots.
     *
     * @param prev Previous button snapshot.
     * @param cur Current button snapshot.
     */
    inline void
    logButtonEvents(const ButtonState &prev, const ButtonState &cur) noexcept
    {
        for_each_edge<NUM_BUTTONS>(
            prev, cur,
            [](std::size_t i, bool pressed, std::uint32_t t_ms)
            {
                debug(kButtonNames[i]);                          ///< Button name.
                debug(pressed ? " pressed @ " : " released @ "); ///< Edge type.
                debugln(t_ms);                                   ///< Snapshot timestamp (ms).
            });
    }
} ///< namespace button_debug
