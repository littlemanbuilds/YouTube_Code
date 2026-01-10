/**
 * MIT License
 *
 * @brief Implementation of StateManager (central manager for input devices).
 *
 * @file StateManager.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-09-12
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#include "StateManager.h"

// Construct with references to the button handler and snapshot bus.
StateManager::StateManager(Button &buttons, InputBus &bus) noexcept
    : buttons_(&buttons), bus_(&bus)
{
    // Seed the bus once, so consumers have a valid first snapshot.
    InputState s{};
    buttons.snapshot(s.buttons); ///< Fill bitset with current debounced levels.
    s.stamp_ms = millis();       ///< Timestamp (ms).
    bus.publish(s);              ///< Initial publish.
}

// Main run loop.
void StateManager::run() noexcept
{
    configASSERT(buttons_ != nullptr && bus_ != nullptr); ///< Sanity check: buttons and bus must be valid.

    TickType_t last_wake = xTaskGetTickCount(); ///< Reference tick for periodic task scheduling.

    for (;;)
    {
        buttons_->update(); ///< Update state.

        InputState s{};                ///< Build a fresh snapshot.
        buttons_->snapshot(s.buttons); ///< Copy debounced levels to bitset.
        s.stamp_ms = millis();         ///< Timestamp (ms).

        bus_->publish(s); ///< Publish to the bus.

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(cfg::LOOP_INTERVAL_TEST_SHORT));
    }
}