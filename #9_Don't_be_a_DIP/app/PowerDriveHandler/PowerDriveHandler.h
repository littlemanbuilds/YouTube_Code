/**
 * MIT License
 *
 * @brief Motor power and drive handler.
 *
 * @file PowerDriveHandler.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-09-12
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <ControlBus.h>
#include <ESP32_MCPWM.h>

/**
 * @brief Selects the power level and drives the motor.
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
        configASSERT(loop_ticks_ > 0); ///< Enforce scheduling invariant at construction time.
    }

    /**
     * @brief FreeRTOS task trampoline. Call with `pvParameters = this`.
     */
    static inline void task(void *self) noexcept
    {
        static_cast<PowerDriveHandler *>(self)->run();
    }

private:
    /// @brief Main run loop.
    void run() noexcept;

    // ---- Tuning knobs ---- //
    static constexpr float kRampRatePerSec = 40.0f; ///< %/s: 0→100% in 2.5s (↑ faster, ↓ smoother).
    static constexpr float kDirFlipHoldPct = 1.0f;  ///< %: Require effective zero torque to avoid jitter around 0%.

    // ---- Internal state ---- //
    IMotorDriver &motor_;                              ///< Non-owning motor driver.
    ControlBus &bus_;                                  ///< Non-owning input bus.
    TickType_t loop_ticks_{0};                         ///< Delay (in ticks) between loop iterations.
    DriveDir active_dir_cmd_{DriveDir::Fwd};           ///< Latched direction being executed.
    DriveOwner active_drive_owner_{DriveOwner::None};  ///< Owner currently allowed to drive.
    DriveOwner pending_drive_owner_{DriveOwner::None}; ///< Owner waiting for safe handover.
    bool handover_pending_{false};                     ///< True while waiting for zero before owner change.
    float current_pct_{0.0f};                          ///< Current percent (0..100).
    ControlSnapshot last_ctl_{};                       ///< Last stable control frame.
    bool has_last_ctl_{false};                         ///< True once a stable frame has been observed.
    uint32_t last_debug_ms_{0};                        ///< Last drive debug print time.
};