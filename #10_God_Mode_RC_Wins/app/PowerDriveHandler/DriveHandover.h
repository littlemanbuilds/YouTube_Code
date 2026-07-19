/**
 * MIT License
 *
 * @brief Stops the drivetrain before accepting commands from a new drive owner.
 *
 * @file DriveHandover.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-06-20
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <ControlBus.h>

namespace app::drive
{
    /// @brief Tracks the current owner and any new owner waiting for the car to stop.
    struct OwnerHandoverState
    {
        DriveOwner active{DriveOwner::None};  ///< Owner currently allowed to drive.
        DriveOwner pending{DriveOwner::None}; ///< Latest owner waiting for a safe stop.
        bool in_progress{false};              ///< True while output is being ramped to zero.
    };

    /**
     * @brief Start, continue, or complete a safe drive-owner handover.
     *
     * @param state Handover state to update.
     * @param drive_enabled True when the current control frame requests movement.
     * @param requested Latest requested drive owner.
     * @param current_pct Current motor output percentage.
     * @param stopped_pct Maximum percentage considered effectively stopped.
     * @return True only when the active owner changes during this update.
     */
    [[nodiscard]] constexpr bool updateOwnerHandover(OwnerHandoverState &state,
                                                     bool drive_enabled,
                                                     DriveOwner requested,
                                                     float current_pct,
                                                     float stopped_pct) noexcept
    {
        const bool valid_request = drive_enabled && (requested != DriveOwner::None);

        if (state.in_progress)
        {
            // While slowing down, follow the newest request. This prevents an old
            // owner request from being accepted after the driver changes their mind.
            state.pending = (valid_request && requested != state.active)
                                ? requested
                                : DriveOwner::None;

            if (current_pct <= stopped_pct)
            {
                const bool owner_changed = (state.pending != DriveOwner::None);

                if (state.pending != DriveOwner::None)
                {
                    state.active = state.pending;
                }

                state.pending = DriveOwner::None;
                state.in_progress = false;
                return owner_changed;
            }

            return false;
        }

        if (!valid_request || requested == state.active)
        {
            return false;
        }

        if (current_pct <= stopped_pct)
        {
            // No handover delay is needed when the drivetrain is already stopped.
            state.active = requested;
            state.pending = DriveOwner::None;
            return true;
        }

        state.pending = requested;
        state.in_progress = true;
        return false;
    }

    /**
     * @brief Check whether the requested owner's command may reach the motor.
     *
     * @param state Current owner handover state.
     * @param drive_enabled True when the current control frame requests movement.
     * @param requested Latest requested drive owner.
     * @return True when the requested owner's command may be sent to the motor.
     */
    [[nodiscard]] constexpr bool ownerCommandAllowed(const OwnerHandoverState &state,
                                                     bool drive_enabled,
                                                     DriveOwner requested) noexcept
    {
        return drive_enabled && !state.in_progress &&
               (requested != DriveOwner::None) && (requested == state.active);
    }
} ///< namespace app::drive
