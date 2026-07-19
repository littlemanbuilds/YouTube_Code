/**
 * MIT License
 *
 * @brief Compile-time debug view selection for Serial Monitor output.
 *
 * @file AppDisplay.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-05-19
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <cstdint>

namespace cfg::display
{
    /**
     * @brief Main Serial Monitor view used while filming/debugging.
     * @note Keep this to one major view at a time so the Serial Monitor remains readable.
     *
     * @details
     * StackWatch runs separately so it can report stack usage while another
     * debug view is active.
     */
    enum class View : uint8_t
    {
        None,   ///< No high-level debug stream.
        Config, ///< Show decoded DIP/shifter configuration.
        Button, ///< Show button edge/latch events.
        Drive,  ///< Show PowerDriveHandler request/actual/throttle output.
        Stack   ///< Show only RTOS stack high-water marks.
    };

    /**
     * @brief Select the active Serial Monitor view.
     */
    inline constexpr View ACTIVE_VIEW = View::Config;

    /**
     * @brief Enable StackWatch alongside the active view.
     * @note Enable this while sizing task stacks so StackWatch runs in the same boot session as the main debug view.
     */
    inline constexpr bool STACK_WATCH_BACKGROUND = false;

    /**
     * @brief True when ConfigDebug should be active.
     */
    inline constexpr bool CONFIG_DEBUG = (ACTIVE_VIEW == View::Config);

    /**
     * @brief True when ButtonDebug should be active.
     */
    inline constexpr bool BUTTON_DEBUG = (ACTIVE_VIEW == View::Button);

    /**
     * @brief True when PowerDriveHandler drive debug should be active.
     */
    inline constexpr bool DRIVE_DEBUG = (ACTIVE_VIEW == View::Drive);

    /**
     * @brief True when StackWatch should be active.
     * @note This is intentionally decoupled from ACTIVE_VIEW so StackWatch can
     *       run while another debug view is producing stack-heavy output.
     */
    inline constexpr bool STACK_DEBUG =
        (ACTIVE_VIEW == View::Stack) || STACK_WATCH_BACKGROUND;

} ///< namespace cfg::display
