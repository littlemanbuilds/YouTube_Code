/**
 * MIT License
 *
 * @brief Implementation of ControlCore.
 *
 * @file ControlCore.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-12-30
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#include "ControlCore.h"

#include <ButtonDebug.h>
#include <Clock.h>
#include <Liveness.h>
#include <MathFns.h>
#include <SnapshotTools.h>

namespace
{
    /**
     * @brief File-lcoal policy helpers.
     */

    // ---- Button policy ---- //

    // Button-derived throttle command.
    inline float buttonThrottle(const ButtonState &btn) noexcept
    {
        // 100% when pressed; otherwise 0%.
        return btn.buttons.test(idx(ButtonIndex::Accelerator)) ? cfg::units::MAX_PCT : cfg::units::MIN;
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

    // Best-effort reads with fallback.
    static constexpr auto kFallback = snapshot::tools::UnstablePolicy::UseLast;

    for (;;)
    {
        vTaskDelayUntil(&last_wake, loop_ticks_); ///< Enforce period is anchored.

        const uint32_t now_ms = utils::clock::now_ms(); ///< Millisecond timebase for control logic.
        const uint64_t now_us = utils::clock::now_us(); ///< Microsecond timebase for snapshot timestamps.

        // ---- Buttons (edge events) ---- //
        ButtonState btn{}; ///< Local copy of the latest button snapshot.
        const bool got_new_btn = snapshot::try_peek_new(
            buttons_,      ///< Source ButtonBus.
            btn_seen_seq_, ///< Last processed sequence number.
            btn);          ///< Output snapshot (new data only).

        if (got_new_btn)
        {
            if (has_last_btn_)
            {
                utils::button_debug::logButtonEvents(last_btn_, btn); ///< Requires a real previous snapshot.
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

        // ---- Authority + RC ---- //

        // Required by peek_with_fallback() API to count unstable reads.
        // Intentionally discarded to keep the control loop free of unused diagnostics.
        uint32_t dummy_auth_unstable = 0;
        uint32_t dummy_rc_unstable = 0;

        AuthoritySnapshot auth{}; ///< Local copy of latest authority snapshot (with fallback support).
        static_cast<void>(snapshot::tools::peek_with_fallback(
            auth_,               ///< Source AuthorityBus.
            auth,                ///< Output snapshot (filled on success or fallback).
            last_auth_,          ///< Last known good snapshot.
            has_last_auth_,      ///< True once a valid snapshot has been observed.
            dummy_auth_unstable, ///< Counter of unstable/failed reads.
            kFallback)           ///< Fallback policy when bus reads are unstable.
        );

        // Provide a timestamp for boot frame.
        if (!has_last_auth_)
        {
            auth.stamp_us = now_ms;
        }

        RcSnapshot rc{}; ///< Local copy of latest RC snapshot (with fallback support).
        static_cast<void>(snapshot::tools::peek_with_fallback(
            rc_,               ///< Source RcBus.
            rc,                ///< Output snapshot (filled on success or fallback).
            last_rc_,          ///< Last known good snapshot.
            has_last_rc_,      ///< True once a valid snapshot has been observed.
            dummy_rc_unstable, ///< Counter of unstable/failed reads.
            kFallback)         ///< Fallback policy when bus reads are unstable.
        );

        // Provide a timestamp for boot frame.
        if (!has_last_rc_)
        {
            rc.stamp_us = now_us;
        }

        // ---- Liveness / trust gating ---- //
        const bool btn_fresh = utils::liveness::fresh_ms(now_ms, btn.stamp_ms, cfg::safety::stale::BTN_MS);

        // TEMPORARY: FOR DEMO PURPOSES ONLY.
        const Owner owner_dir = Owner::Direct;   ///< Normally 'auth.owner_dir'.
        const Owner owner_speed = Owner::Direct; ///< Normally 'auth.owner_speed'.

        // Throttle source selection.
        float throttle_pct = cfg::units::MIN;

        switch (owner_speed)
        {
        case Owner::RC:
            throttle_pct = cfg::units::MIN; ///< Placeholder.
            break;

        case Owner::Direct:
            throttle_pct = btn_fresh ? buttonThrottle(btn) : cfg::units::MIN;
            break;

        case Owner::Joystick:
            throttle_pct = cfg::units::MIN; ///< Placeholder.

        default:
            throttle_pct = cfg::units::MIN;
            break;
        }

        // ---- Build control commands ---- //
        ControlSnapshot out{};
        out.throttle_cmd_pct = throttle_pct; // Temporary fix.

        // Direction is a first-class authority axis.
        // PDH should be a dumb executor: it trusts the ControlBus.
        //
        // Policy:
        // - If RC owns direction, reverse intent comes from RC::direction only while RC is trusted.
        // - If Joystick owns direction, reverse intent comes from joystick Y (not implemented yet).
        //  - Otherwise (Direct / delegated), direction comes from gear.
        //
        // Liveness:
        //  - If RC is stale/failsafe and it owns direction, hold last published direction.
        DriveDir dir_cmd = DriveDir::Fwd;
        switch (owner_dir)
        {
        case Owner::RC:
            // TODO: derive from RC (not implemented - later episode).
            break;

        case Owner::Joystick:
            // TODO: derive from joystick (not implemented - later episode).

        case Owner::Direct:
        default:
            // TODO: derive from direct (reverse currently not implemented - later episode).
            break;
        }

        out.dir_cmd = dir_cmd;
        last_dir_cmd_ = dir_cmd;

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