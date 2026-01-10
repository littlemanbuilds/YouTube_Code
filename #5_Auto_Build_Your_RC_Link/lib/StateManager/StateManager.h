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
#include <InputTypes.h>
#include <freertos/FreeRTOS.h>

/**
 * @brief Manages input scanning and publishes snapshots to an input bus.
 */
class StateManager
{
public:
    /**
     * @brief Construct with references to the button handler and snapshot bus.
     *
     * @param buttons IButtonHandler instance.
     * @param bus Snapshot bus to publish InputState frames to.
     * @param period_ms FreeRTOS tick interval used to pace the run loop (in milliseconds).
     */
    StateManager(IButtonHandler &buttons, InputBus &bus, uint32_t period_ms = cfg::tick::LOOP_MS) noexcept;

    /**
     * @brief FreeRTOS task trampoline. Call with `pvParameters = this`.
     */
    static inline void task(void *self) noexcept
    {
        static_cast<StateManager *>(self)->run();
    }

private:
    /// @brief Main run loop.
    void run() noexcept;

private:
    IButtonHandler *buttons_{nullptr}; ///< Non-owning; provides update() and snapshot().
    InputBus *bus_{nullptr};           ///< Non-owning; receives published InputState frames.
    TickType_t loop_ticks_{0};         ///< Delay (in ticks) between loop iterations.
};