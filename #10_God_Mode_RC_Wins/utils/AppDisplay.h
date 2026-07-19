/**
 * MIT License
 *
 * @brief Selects one clear Serial Monitor story for filming or engineering diagnostics.
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
     * @brief Main Serial Monitor view used while filming or debugging.
     * @note Keep this to one major view at a time so the Serial Monitor remains readable.
     *
     * @details
     * StackWatch can run in the background because memory sizing may need to be
     * observed while the audience-facing view is active.
     */
    enum class View : uint8_t
    {
        None,      ///< No high-level debug stream.
        Authority, ///< Show who is in control and the current maximum output.
        Config,    ///< Show decoded DIP/shifter configuration.
        Button,    ///< Show button edge/latch events.
        Drive,     ///< Show requested and ramped drivetrain commands.
        Stack      ///< Show remaining stack margin for each application task.
    };

    /**
     * @brief Select the active Serial Monitor view.
     */
    inline constexpr View ACTIVE_VIEW = View::Drive;

    /**
     * @brief Enable StackWatch alongside the active view.
     * @note Enable this while sizing task stacks so StackWatch runs in the same boot session as the main debug view.
     */
    inline constexpr bool STACK_WATCH_BACKGROUND = false;

    /**
     * @brief True when AuthorityDebug should be active.
     */
    inline constexpr bool AUTHORITY_DEBUG = (ACTIVE_VIEW == View::Authority);

    /**
     * @brief True when ButtonDebug should be active.
     */
    inline constexpr bool BUTTON_DEBUG = (ACTIVE_VIEW == View::Button);

    /**
     * @brief True when ConfigDebug should be active.
     */
    inline constexpr bool CONFIG_DEBUG = (ACTIVE_VIEW == View::Config);

    /**
     * @brief True when PowerDriveHandler drive debug should be active.
     */
    inline constexpr bool DRIVE_DEBUG = (ACTIVE_VIEW == View::Drive);

    /**
     * @brief True when StackWatch should be active.
     * @note This is separate from ACTIVE_VIEW so stack usage can be measured
     *       under the real load of another debug view.
     */
    inline constexpr bool STACK_DEBUG =
        (ACTIVE_VIEW == View::Stack) || STACK_WATCH_BACKGROUND;

} ///< namespace cfg::display
