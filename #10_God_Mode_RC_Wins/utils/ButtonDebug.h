/**
 * MIT License
 *
 * @brief Prints button presses and releases in a form that is easy to follow on camera.
 * @note This observes already-debounced ButtonBus state; it does not alter button behavior.
 *
 * @file ButtonDebug.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-23
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <ButtonBus.h>
#include <Debugging.h>

namespace utils::button_debug
{
    /**
     * @brief Print each press or release that occurred between two button snapshots.
     *
     * @param prev Previous button snapshot.
     * @param cur Current button snapshot.
     */
    inline void
    logButtonEvents(const ButtonState &prev, const ButtonState &cur) noexcept
    {
        for_each_edge<NUM_BUTTONS>(
            prev, cur,
            [](std::size_t i, bool pressed, uint32_t t_ms)
            {
                debug(kButtonNames[i]);                            // Button name.
                debug(pressed ? " pressed @ " : " released @ "); // Edge type.
                debugln(t_ms);                                     // Snapshot timestamp (ms).
            });
    }
} ///< namespace utils::button_debug
