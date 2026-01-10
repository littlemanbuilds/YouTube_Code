/**
 * MIT License
 *
 * @brief Control core (InputBus → ControlBus).
 *
 * @file ControlCore.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-12-30
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#pragma once

#include <app_config.h>
#include <cmath>
#include <InputBus.h>
#include <ControlBus.h>

/**
 * @brief Applies control policy to raw inputs and emits resolved commands.
 *
 * This layer interprets button state, applies simple rules (latching,
 * toggling), and produces concrete control commands for downstream handlers.
 */
class ControlCore
{
public:
    /**
     * @brief Construct with input bus and output bus.
     *
     * @param in Input bus (non-owning).
     * @param out Control bus (non-owning).
     * @param period_ms Loop period (milliseconds).
     */
    ControlCore(InputBus &in, ControlBus &out, std::uint32_t period_ms = cfg::tick::LOOP_MS) noexcept
        : in_(&in), out_(&out), loop_ticks_(to_ticks_ms(period_ms)) {}

    /**
     * @brief FreeRTOS task trampoline. Call with `pvParameters = this`.
     */
    static inline void task(void *self) noexcept
    {
        static_cast<ControlCore *>(self)->run();
    }

private:
    /// @brief Main run loop.
    void run() noexcept;

    // ---- Button roles (policy-level) ---- //
    static constexpr ButtonIndex kBtnAccel = ButtonIndex::Accelerator;
    static constexpr ButtonIndex kBtnHorn = ButtonIndex::Horn;
    static constexpr ButtonIndex kBtnLeft = ButtonIndex::IndicatorLeft;
    static constexpr ButtonIndex kBtnRight = ButtonIndex::IndicatorRight;

    // ---- Policy knobs ---- //
    static constexpr float kMinPct = 0.0f;   ///< Minimum throttle command (%).
    static constexpr float kMaxPct = 100.0f; ///< Maximum throttle command (%).

    // ---- Internal state ---- //
    InputBus *in_{nullptr};    ///< Non-owning input bus (raw button snapshots).
    ControlBus *out_{nullptr}; ///< Non-owning output bus (resolved control commands).
    TickType_t loop_ticks_{0}; ///< Loop period in FreeRTOS ticks.

    InputState prev_{};    ///< Previous input snapshot (for edge detection + event logging).
    bool has_prev_{false}; ///< True once prev_ is valid.
};