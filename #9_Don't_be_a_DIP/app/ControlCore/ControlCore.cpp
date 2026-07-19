/**
 * MIT License
 *
 * @brief Implementation of ControlCore.
 *
 * @file ControlCore.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-12-30
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#include "ControlCore.h"

#include <AppDisplay.h>
#include <ButtonDebug.h>
#include <Clock.h>
#include <Liveness.h>
#include <MathFns.h>
#include <SnapshotTools.h>

namespace
{
    /**
     * @brief Button-derived throttle command.
     *
     * @param btn Latest button state.
     * @return 100% when the accelerator button is pressed; otherwise 0%.
     */
    [[nodiscard]] inline float buttonThrottle(const ButtonState &btn) noexcept
    {
        return btn.buttons.test(idx(ButtonIndex::Accelerator))
                   ? cfg::units::MAX_PCT
                   : cfg::units::MIN;
    }
} ///< namespace

// Construct the ControlCore task.
ControlCore::ControlCore(ButtonBus &buttons, RcBus &rc, AuthorityBus &auth,
                         ControlBus &out, uint32_t period_ms) noexcept
    : buttons_(buttons), rc_(rc), auth_(auth), out_(out),
      loop_ticks_(cfg::rtos::to_ticks_ms(period_ms))
{
    configASSERT(loop_ticks_ > 0); ///< Enforce scheduling invariant at construction time.
}

// Main run loop.
void ControlCore::run() noexcept
{
    TickType_t last_wake = xTaskGetTickCount(); ///< Reference tick for periodic task scheduling.

    for (;;)
    {
        vTaskDelayUntil(&last_wake, loop_ticks_); ///< Enforce period is anchored.

        const uint32_t now_ms = utils::clock::now_ms();

        // ---- Buttons (edge events) ---- //
        ButtonState btn{};
        const bool got_new_btn = snapshot::try_peek_new(
            buttons_,      ///< Source ButtonBus.
            btn_seen_seq_, ///< Last processed sequence number.
            btn);          ///< Output snapshot (new data only).

        if (got_new_btn)
        {
            if constexpr (cfg::display::BUTTON_DEBUG)
            {
                if (has_last_btn_)
                {
                    utils::button_debug::logButtonEvents(last_btn_, btn);
                }
            }
            last_btn_ = btn;
            has_last_btn_ = true;
        }
        else
        {
            // No new stable frame; use last known stable state (or default).
            btn = has_last_btn_ ? last_btn_ : ButtonState{};
        }

        // Provide a timestamp for boot frame.
        if (!has_last_btn_ && !got_new_btn)
        {
            btn.stamp_ms = now_ms;
        }

        // ---- Liveness / trust gating ---- //
        const bool btn_fresh = utils::liveness::fresh_ms(now_ms, btn.stamp_ms, cfg::safety::stale::BTN_MS);

        // TEMPORARY => keeping the drivetrain intentionally direct:
        // - ConfigPublisher proves DIP + shifter decoding.
        // - The accelerator button remains the temporary direct drive request.
        const float throttle_cmd_pct = btn_fresh ? buttonThrottle(btn) : cfg::units::MIN;

        // ---- Build control commands ---- //
        ControlSnapshot out{};
        out.throttle_cmd_pct = throttle_cmd_pct;
        out.dir_cmd = DriveDir::Fwd; ///< TEMPORARY => forward-only.

        // Movement is enabled only when there is a meaningful direct throttle request.
        // This keeps PowerDriveHandler's explicit drive_enable contract active before
        // "the next episode's" authority layer starts selecting owners dynamically.
        static constexpr float kDriveEnablePct = 0.3f;
        out.drive_enable = throttle_cmd_pct > kDriveEnablePct;
        out.drive_owner = out.drive_enable ? DriveOwner::Direct : DriveOwner::None;

        // Only allow button actions if the input is recent.
        // Prevents outputs (e.g. horn) from getting stuck if input freezes.
        out.horn_cmd = btn_fresh && btn.buttons.test(idx(kBtnHorn));

        out.indicator_cmd = ControlSnapshot::Indicator::Off;
        if (btn_fresh && btn.buttons.test(idx(kBtnLeft)))
            out.indicator_cmd = ControlSnapshot::Indicator::Left;
        else if (btn_fresh && btn.buttons.test(idx(kBtnRight)))
            out.indicator_cmd = ControlSnapshot::Indicator::Right;

        out.stamp_ms = now_ms; ///< Critical: control output liveness.

        out_.publish(out);
    }
}