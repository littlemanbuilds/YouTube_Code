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
#include <SnapshotBus.h>
#include <SnapshotModel.h>
#include <InputTypes.h>

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
     */
    PowerDriveHandler(IMotorDriver &motor, InputBus &bus) noexcept
        : motor_(&motor), bus_(&bus) {}

    /**
     * @brief FreeRTOS task trampoline.
     * Call with `pvParameters = this`. Invokes the private run loop.
     *
     * @param self Opaque pointer to a PowerDriveHandler instance.
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

    // ---- Tuning knobs ----
    static constexpr float kRampStepPct = 2.0f; ///< % change per tick (↑ faster, ↓ smoother).
    static constexpr float kMinPct = 0.0f;      ///< Lower clamp for percent.
    static constexpr float kMaxPct = 100.0f;    ///< Upper clamp for percent.
    static constexpr Dir kDir = Dir::CW;        ///< Direction parameter.

    // ---- Buttons ----
    static constexpr auto btnAccel = idx(ButtonIndex::Accelerator); ///< Index of the Accelerator button.

    // ---- State ----
    IMotorDriver *motor_{nullptr}; ///< Non-owning motor driver.
    InputBus *bus_{nullptr};       ///< Non-owning input bus.
    float current_pct_{0.0f};      ///< Current percent (0..100).
};