/**
 * MIT License
 *
 * @brief Combines trusted inputs and authority policy into final control commands.
 *
 * @file ControlCore.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-12-30
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#include "ControlCore.h"

#include <AppDisplay.h>
#include <ButtonDebug.h>
#include <Clock.h>
#include <Liveness.h>
#include <MathFns.h>
#include <SnapshotTools.h>

#include <cstdint>

/**
 * @brief Small control-policy helpers used only by this translation unit.
 */
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

    /**
     * @brief Check whether the selected gear is Reverse.
     *
     * @param g Current gear selection.
     * @return True when Reverse is selected; otherwise false.
     */
    [[nodiscard]] inline bool gearIsReverse(Gear g) noexcept
    {
        return (g == Gear::Rev);
    }

    /**
     * @brief Check whether a recognized direction owner supplies valid direction intent.
     *
     * @param owner Current direction owner.
     * @param rc_has_direction True when RC supplies an explicit forward/reverse request.
     * @return True only for a recognized owner with valid direction intent.
     */
    [[nodiscard]] constexpr bool directionIntentAllowed(Owner owner, bool rc_has_direction) noexcept
    {
        switch (owner)
        {
        case Owner::RC:
            return rc_has_direction;

        case Owner::Direct:
        case Owner::Joystick:
            return true;

        case Owner::None:
        default:
            return false;
        }
    }

    static_assert(!directionIntentAllowed(Owner::None, true),
                  "Missing direction authority must fail closed.");

    /// @brief Returns RC values only while the receiver data is safe to trust.
    struct TrustedRc
    {
        const RcSnapshot &rc; ///< Current RC snapshot.
        bool trusted;         ///< True if snapshot is fresh and not in failsafe.

        /**
         * @brief Read an RC axis if trusted; otherwise return the fallback.
         *
         * @param ch RC channel to read.
         * @param fallback Value used when RC is unsafe.
         * @return Axis value or fallback.
         */
        [[nodiscard]] float axis(RC ch, float fallback) const noexcept
        {
            return trusted ? rc_get(rc, ch) : fallback;
        }
    };

    /**
     * @brief RC drive direction intent.
     * @note Neutral must not become forward just because throttle rises.
     */
    enum class RcDriveIntent : std::uint8_t
    {
        Neutral = 0,
        Fwd,
        Rev
    };

    /**
     * @brief Derive explicit drive intent from RC direction axis.
     *
     * @param rc Trusted RC wrapper.
     * @return Forward, reverse, or neutral if the stick is near the center.
     */
    [[nodiscard]] inline RcDriveIntent rcDriveIntent(const TrustedRc &rc) noexcept
    {
        static constexpr float kFwdThreshold = 30.0f;  ///< Avoid jitter near center.
        static constexpr float kRevThreshold = -30.0f; ///< Avoid jitter near center.

        const float axis = rc.axis(RC::direction, 0.0f);

        if (axis >= kFwdThreshold)
        {
            return RcDriveIntent::Fwd;
        }

        if (axis <= kRevThreshold)
        {
            return RcDriveIntent::Rev;
        }

        return RcDriveIntent::Neutral;
    }
} ///< namespace

ControlCore::ControlCore(ButtonBus &buttons, RcBus &rc, AuthorityBus &auth,
                         ControlBus &out, uint32_t period_ms) noexcept
    : buttons_(buttons), rc_(rc), auth_(auth), out_(out),
      loop_ticks_(cfg::rtos::to_ticks_ms(period_ms))
{
    configASSERT(loop_ticks_ > 0); // A zero-tick period would turn this task into a busy loop.
    if (loop_ticks_ == 0)
    {
        loop_ticks_ = 1; // Remain schedulable even in builds where assertions are disabled.
    }
}

/**
 * @brief Build a safe authority snapshot when normal authority data is unavailable.
 *
 * @param now_us Current time in microseconds.
 * @return Snapshot with no control owner and a zero drive-output cap.
 */
AuthoritySnapshot ControlCore::safeAuthority(uint64_t now_us) noexcept
{
    AuthoritySnapshot auth{};

    auth.mode = CarMode::RC;
    auth.gear = Gear::Unknown;
    auth.rc_delegation = RcDelegation::FullRC;
    auth.rc_failsafe = true;

    auth.owner_steer = Owner::None;
    auth.owner_dir = Owner::None;
    auth.owner_speed = Owner::None;
    auth.owner_power = Owner::None;

    auth.allowed_mask = kAllowNone;

    auth.base_cap = cfg::units::MIN;
    auth.gear_cap = cfg::units::MIN;
    auth.eff_cap = cfg::units::MIN;

    auth.reason_bits = kReasonNone;
    auth.stamp_us = now_us;

    return auth;
}

void ControlCore::run() noexcept
{
    TickType_t last_wake = xTaskGetTickCount(); ///< Reference tick for periodic task scheduling.

    // If a read overlaps a publish, reuse the last complete frame rather than
    // consuming a partially updated input.
    static constexpr auto kFallback = snapshot::tools::UnstablePolicy::UseLast;

    for (;;)
    {
        vTaskDelayUntil(&last_wake, loop_ticks_); // Maintain fixed-period scheduling.

        const uint32_t now_ms = utils::clock::now_ms();
        const uint64_t now_us = utils::clock::now_us();

        // ---- Buttons (edge events) ---- //
        ButtonState btn{};
        const bool got_new_btn = snapshot::try_peek_new(
            buttons_,      // Source ButtonBus.
            btn_seen_seq_, // Last processed sequence number.
            btn);          // Output snapshot (new data only).

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
            // No new complete frame: keep the last button state, or the safe default at boot.
            btn = has_last_btn_ ? last_btn_ : ButtonState{};
        }

        // Timestamp the safe boot frame so freshness checks behave predictably.
        if (!has_last_btn_ && !got_new_btn)
        {
            btn.stamp_ms = now_ms;
        }

        // ---- Authority + RC ---- //

        // The helper counts reads that overlap a publish. Telemetry does not use
        // the counts yet, but the counters are still required by its API.
        uint32_t dummy_auth_unstable = 0;
        uint32_t dummy_rc_unstable = 0;

        AuthoritySnapshot auth{};
        static_cast<void>(snapshot::tools::peek_with_fallback(
            auth_, auth, last_auth_, has_last_auth_, dummy_auth_unstable, kFallback));

        // Never keep driving from an old ownership decision. If AuthorityRouter
        // stops publishing, fail closed to no owner and zero output.
        const bool auth_live = has_last_auth_ &&
                               utils::liveness::fresh_us(now_us, auth.stamp_us, cfg::safety::stale::AUTH_MS);

        if (!auth_live)
        {
            auth = safeAuthority(now_us);
        }

        RcSnapshot rc{};
        static_cast<void>(snapshot::tools::peek_with_fallback(
            rc_, rc, last_rc_, has_last_rc_, dummy_rc_unstable, kFallback));

        // Timestamp the safe boot frame; its default axes still request no movement.
        if (!has_last_rc_)
        {
            rc.stamp_us = now_us;
        }

        // ---- Reject stale inputs before making commands ---- //
        const bool btn_fresh = utils::liveness::fresh_ms(now_ms, btn.stamp_ms, cfg::safety::stale::BTN_MS);

        // RC axes are usable only while the frame is recent and the receiver is not in failsafe.
        const bool rc_live = utils::liveness::fresh_us(now_us, rc.stamp_us, cfg::safety::stale::RC_MS);
        const bool rc_has_intent = rc_live && !rc.failsafe;
        const TrustedRc rc_in{rc, rc_has_intent};

        // Without trusted RC data, neutral is the only safe direction request.
        const RcDriveIntent rc_drive_intent = rc_has_intent ? rcDriveIntent(rc_in)
                                                            : RcDriveIntent::Neutral;

        const Owner owner_dir = auth.owner_dir;
        const Owner owner_speed = auth.owner_speed;
        const float eff_cap_01 = utils::mathfns::clamp_01(auth.eff_cap);

        // Select drive demand from the owner chosen by AuthorityRouter.
        float throttle_pct = cfg::units::MIN;

        switch (owner_speed)
        {
        case Owner::RC:
            // RC drive demand is only valid when a forward or reverse direction is requested.
            throttle_pct = (rc_drive_intent != RcDriveIntent::Neutral)
                               ? rc_in.axis(RC::speed, cfg::units::MIN)
                               : cfg::units::MIN;
            break;

        case Owner::Direct:
            throttle_pct = btn_fresh ? buttonThrottle(btn) : cfg::units::MIN;
            break;

        case Owner::Joystick:
            // Joystick drive demand remains disabled until a JoystickBus is available.
            throttle_pct = cfg::units::MIN;
            break;

        default:
            throttle_pct = cfg::units::MIN;
            break;
        }

        // Apply the final cap once, after selecting the owner. This keeps the cap
        // independent of the input source and prevents accidental double-limiting.
        const float throttle_cmd_pct = utils::mathfns::apply_cap_pct(throttle_pct, eff_cap_01);

        // ---- Build control commands ---- //
        ControlSnapshot out{};
        out.throttle_cmd_pct = throttle_cmd_pct;

        // Direction has its own owner. ControlCore resolves it here so the motor
        // task receives one unambiguous direction command.
        DriveDir dir_cmd = DriveDir::Fwd;

        switch (owner_dir)
        {
        case Owner::RC:
            switch (rc_drive_intent)
            {
            case RcDriveIntent::Fwd:
                dir_cmd = DriveDir::Fwd;
                break;

            case RcDriveIntent::Rev:
                dir_cmd = DriveDir::Rev;
                break;

            case RcDriveIntent::Neutral:
            default:
                // A centered RC stick means stop, not forward. Keep the remembered
                // direction only so the next valid request does not flip polarity needlessly.
                dir_cmd = last_dir_cmd_;
                break;
            }
            break;

        case Owner::Joystick:
            // Joystick drive is still disabled. Keep the local gear as a harmless
            // remembered direction until a JoystickBus supplies real intent.
            dir_cmd = gearIsReverse(auth.gear) ? DriveDir::Rev : DriveDir::Fwd;
            break;

        case Owner::Direct:
        default:
            dir_cmd = gearIsReverse(auth.gear) ? DriveDir::Rev : DriveDir::Fwd;
            break;
        }

        out.dir_cmd = dir_cmd;
        last_dir_cmd_ = dir_cmd;

        // Movement is enabled only when there is both:
        //  - a meaningful throttle request, and
        //  - a valid direction intent for the current direction owner.
        //
        // This prevents a centered RC direction stick from silently becoming "forward".
        static constexpr float kDriveEnablePct = 0.3f;

        const bool direction_intent_ok = directionIntentAllowed(
            owner_dir, rc_drive_intent != RcDriveIntent::Neutral);

        out.drive_enable = direction_intent_ok && (throttle_cmd_pct > kDriveEnablePct);

        switch (owner_speed)
        {
        case Owner::RC:
            out.drive_owner = DriveOwner::RC;
            break;

        case Owner::Direct:
            out.drive_owner = DriveOwner::Direct;
            break;

        case Owner::Joystick:
            out.drive_owner = DriveOwner::Joystick;
            break;

        case Owner::None:
        default:
            out.drive_owner = DriveOwner::None;
            break;
        }

        // Gate button-driven outputs on freshness so a frozen input cannot leave
        // the horn or indicators stuck on.
        out.horn_cmd = btn_fresh && btn.buttons.test(idx(kBtnHorn));

        out.indicator_cmd = ControlSnapshot::Indicator::Off;
        if (btn_fresh && btn.buttons.test(idx(kBtnLeft)))
            out.indicator_cmd = ControlSnapshot::Indicator::Left;
        else if (btn_fresh && btn.buttons.test(idx(kBtnRight)))
            out.indicator_cmd = ControlSnapshot::Indicator::Right;

        out.stamp_ms = now_ms; // Lets the motor task reject this command if updates stop.

        out_.publish(out);
    }
}
