/**
 * MIT License
 *
 * @brief Turns trusted button, RC, and authority snapshots into final control commands.
 *
 * @file ControlCore.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-12-30
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <AuthorityBus.h>
#include <ButtonBus.h>
#include <ControlBus.h>
#include <RcBus.h>

/**
 * @brief Selects the authorized inputs, applies safety caps, and publishes one clear command set.
 */
class ControlCore
{
public:
    /**
     * @brief Create the control core and bind its input and output buses.
     *
     * @param buttons Button input bus (from ButtonPublisher).
     * @param rc RC input bus (from RcPublisher).
     * @param auth Authority/mode bus (from AuthorityRouter).
     * @param out Control output bus.
     * @param period_ms Loop period (ms).
     */
    ControlCore(ButtonBus &buttons, RcBus &rc, AuthorityBus &auth, ControlBus &out,
                uint32_t period_ms = cfg::tick::LOOP_MS) noexcept;

    /**
     * @brief FreeRTOS task trampoline. Call with `pvParameters = this`.
     */
    static inline void task(void *self) noexcept
    {
        static_cast<ControlCore *>(self)->run();
    }

private:
    /// @brief Continuously resolve trusted inputs into final control commands.
    void run() noexcept;

    /**
     * @brief Build a fail-closed authority decision when normal authority data is unavailable.
     *
     * @param now_us Timestamp to place on the fallback snapshot.
     * @return Safe no-owner, no-throttle authority state.
     */
    static AuthoritySnapshot safeAuthority(uint64_t now_us) noexcept;

    // ---- Button roles (policy-level) ---- //
    static constexpr ButtonIndex kBtnAccel = ButtonIndex::Accelerator;
    static constexpr ButtonIndex kBtnHorn = ButtonIndex::Horn;
    static constexpr ButtonIndex kBtnLeft = ButtonIndex::IndicatorLeft;
    static constexpr ButtonIndex kBtnRight = ButtonIndex::IndicatorRight;

    // ---- Internal state ---- //
    ButtonBus &buttons_;       ///< Reference to the ButtonBus providing debounced button snapshots.
    RcBus &rc_;                ///< Reference to the RcBus providing RC axes + failsafe state.
    AuthorityBus &auth_;       ///< Reference to the AuthorityBus providing ownership and drive-output caps.
    ControlBus &out_;          ///< Reference to the ControlBus to publish resolved control commands into.
    TickType_t loop_ticks_{0}; ///< Loop period in FreeRTOS ticks.

    // ---- Button edge tracking ---- //
    uint32_t btn_seen_seq_{0}; ///< Sequence marker for try_peek_new().
    ButtonState last_btn_{};   ///< Previous button snapshot used for edge detection.
    bool has_last_btn_{false}; ///< False until first real snapshot is observed (prevents false edges at boot).

    // ---- RC continuity ---- //
    RcSnapshot last_rc_{};    ///< Reused if an RC read overlaps a publish.
    bool has_last_rc_{false}; ///< False until the first complete RC snapshot.

    // ---- Authority continuity ---- //
    AuthoritySnapshot last_auth_{}; ///< Reused if an authority read overlaps a publish.
    bool has_last_auth_{false};     ///< False until the first complete authority snapshot.

    // ---- Direction continuity ---- //
    DriveDir last_dir_cmd_{DriveDir::Fwd}; ///< Remembered direction while no valid movement is requested.
};
