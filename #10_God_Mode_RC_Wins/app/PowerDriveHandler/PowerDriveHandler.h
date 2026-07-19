/**
 * MIT License
 *
 * @brief Turns final drive commands into safe physical motor output.
 *
 * @file PowerDriveHandler.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-09-12
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <ControlBus.h>
#include <ESP32_MCPWM.h>

#include "DriveHandover.h"

/**
 * @brief Ramps motor output and enforces stop-before-handover and stop-before-reverse rules.
 */
class PowerDriveHandler
{
public:
    /**
     * @brief Construct with motor driver and input bus.
     *
     * @param motor Motor driver (non-owning).
     * @param bus Control snapshot bus (non-owning).
     * @param period_ms FreeRTOS tick interval used to pace the run loop (in milliseconds).
     */
    PowerDriveHandler(IMotorDriver &motor, ControlBus &bus, uint32_t period_ms = cfg::tick::LOOP_MS) noexcept
        : motor_(motor), bus_(bus), loop_ticks_(cfg::rtos::to_ticks_ms(period_ms))
    {
        configASSERT(loop_ticks_ > 0); // A zero-tick period would turn this task into a busy loop.
        if (loop_ticks_ == 0)
        {
            loop_ticks_ = 1; // Remain schedulable even in builds where assertions are disabled.
        }
    }

    /**
     * @brief FreeRTOS task trampoline. Call with `pvParameters = this`.
     */
    static inline void task(void *self) noexcept
    {
        static_cast<PowerDriveHandler *>(self)->run();
    }

private:
    /// @brief Continuously validate commands and update physical motor output.
    void run() noexcept;

    // ---- Tuning knobs ---- //
    static constexpr float kRampRatePerSec = 40.0f; ///< 40 percentage points/s; a full ramp takes 2.5 s.
    static constexpr float kDirFlipHoldPct = 1.0f;  ///< Treat 1% or less as stopped before handover/reverse.

    // ---- Internal state ---- //
    IMotorDriver &motor_;                             ///< Non-owning motor driver.
    ControlBus &bus_;                                 ///< Non-owning input bus.
    TickType_t loop_ticks_{0};                        ///< Delay (in ticks) between loop iterations.
    DriveDir active_dir_cmd_{DriveDir::Fwd};          ///< Physical direction currently sent to the motor.
    app::drive::OwnerHandoverState owner_handover_{}; ///< Safe drive-owner handover state.
    float current_pct_{0.0f};                         ///< Current ramped motor command (0..100%).
    ControlSnapshot last_ctl_{};                      ///< Last complete control command.
    bool has_last_ctl_{false};                        ///< True after the first complete command frame.
    uint32_t last_debug_ms_{0};                       ///< Last drive debug print time.
};
