/**
 * MIT License
 *
 * @brief Central manager for input devices.
 *
 * @file StateManager.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-09-12
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#pragma once

#include <app_config.h>
#include <Universal_Button.h>
#include <SnapshotBus.h>
#include <SnapshotModel.h>
#include <InputTypes.h>

/**
 * @brief Manages input scanning and publishes snapshots to an input bus.
 */
class StateManager
{
public:
    /**
     * @brief Construct with references to the button handler and snapshot bus.
     *
     * @param buttons Button handler instance.
     * @param bus Snapshot bus to publish InputState frames to.
     */
    StateManager(Button &buttons, InputBus &bus) noexcept;

    /**
     * @brief FreeRTOS task trampoline.
     * Call with `pvParameters = this`. Invokes the private run loop.
     *
     * @param self Opaque pointer to a StateManager instance.
     */
    static inline void task(void *self) noexcept
    {
        static_cast<StateManager *>(self)->run();
    }

private:
    /**
     * @brief Main run loop.
     */
    void run() noexcept;

private:
    Button *buttons_{nullptr}; ///< Non-owning; provides update() and snapshot().
    InputBus *bus_{nullptr};   ///< Non-owning; receives published InputState frames.
};