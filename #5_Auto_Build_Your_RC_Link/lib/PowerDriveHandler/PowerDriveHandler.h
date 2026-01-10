/**
 * MIT License
 *
 * @brief Motor power and drive handler.
 *
 * @file PowerDriveHandler.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-09-12
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#pragma once

#include <app_config.h>
#include <ESP32_MCPWM.h>
#include <InputTypes.h>
#include <freertos/FreeRTOS.h>

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
     * @param bus Input snapshot bus (non-owning).
     * @param period_ms FreeRTOS tick interval used to pace the run loop (in milliseconds).
     */
    PowerDriveHandler(IMotorDriver &motor, InputBus &bus, uint32_t period_ms = cfg::tick::LOOP_MS) noexcept
        : motor_(&motor), bus_(&bus), loop_ticks_(to_ticks_ms(period_ms)) {}

    /**
     * @brief FreeRTOS task trampoline. Call with `pvParameters = this`.
     */
    static inline void task(void *self) noexcept
    {
        static_cast<PowerDriveHandler *>(self)->run();
    }

private:
    /**
     * @brief Main run loop.
     */
    void run() noexcept;

    // ---- Tuning knobs ---- //
    static constexpr float kRampStepPct = 2.0f; ///< % change per tick (↑ faster, ↓ smoother).
    static constexpr float kMinPct = 0.0f;      ///< Lower clamp for percent.
    static constexpr float kMaxPct = 100.0f;    ///< Upper clamp for percent.
    static constexpr Dir kDir = Dir::CW;        ///< Direction parameter.

    // ---- Buttons ---- //
    static constexpr auto btnAccel = idx(ButtonIndex::Accelerator); ///< Index of the Accelerator button.

    // ---- State ---- //
    IMotorDriver *motor_{nullptr}; ///< Non-owning motor driver.
    InputBus *bus_{nullptr};       ///< Non-owning input bus.
    TickType_t loop_ticks_{0};     ///< Delay (in ticks) between loop iterations.
    float current_pct_{0.0f};      ///< Current percent (0..100).
};