/**
 * MIT License
 *
 * @file AuthorityRouter.cpp
 *
 * @brief Turns mode, gear, and RC state into one system-wide authority decision.
 *
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-02
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#include "AuthorityRouter.h"

#include <AppDisplay.h>
#include <AuthorityPolicy.h>
#include <Clock.h>
#include <Debugging.h>
#include <Liveness.h>
#include <MathFns.h>
#include <SnapshotTools.h>

#include <cmath>
#include <cstdint>

namespace
{
    /// @brief Read the three-position RC switch that selects Full, Mixed, or Delegated control.
    [[nodiscard]] inline RcDelegation resolveDelegation(const RcSnapshot &r) noexcept
    {
        // RCLink reports the switch as 0, 1, or 2. Round and clamp the float so
        // small input noise cannot create an invalid enum value.
        const long pos = lroundf(rc_get(r, RC::mode));
        const uint8_t p = static_cast<uint8_t>(utils::mathfns::clamp<long>(pos, 0L, 2L));
        return static_cast<RcDelegation>(p);
    }
} ///< namespace

AuthorityRouter::AuthorityRouter(RcBus &rc, ConfigBus &cfg, AuthorityBus &out, uint32_t period_ms) noexcept
    : rc_(rc),
      cfg_(cfg),
      out_(out),
      loop_ticks_(cfg::rtos::to_ticks_ms(period_ms))
{
    configASSERT(loop_ticks_ > 0); // A zero-tick period would turn this task into a busy loop.
    if (loop_ticks_ == 0)
    {
        loop_ticks_ = 1; // Remain schedulable even in builds where assertions are disabled.
    }
}

void AuthorityRouter::run() noexcept
{
    TickType_t last = xTaskGetTickCount();

    // If a read overlaps a publish, reuse the last complete frame rather than
    // consuming a partially updated decision input.
    static constexpr auto kFallback = snapshot::tools::UnstablePolicy::UseLast;

    // The helper counts overlapping reads. Telemetry does not use the counts yet,
    // but the counters are still required by its API.
    uint32_t dummy_rc_unstable = 0;
    uint32_t dummy_cfg_unstable = 0;

    for (;;)
    {
        vTaskDelayUntil(&last, loop_ticks_); // Maintain fixed-period scheduling.

        // Read the latest complete RC and configuration snapshots.
        RcSnapshot r{};
        ConfigSnapshot c{};

        // Each successful read refreshes its saved frame. If a publisher is writing
        // at the same instant, the helper returns that saved frame instead.
        static_cast<void>(snapshot::tools::peek_with_fallback(
            rc_, r, last_rc_, has_last_rc_, dummy_rc_unstable, kFallback));

        static_cast<void>(snapshot::tools::peek_with_fallback(
            cfg_, c, last_cfg_, has_last_cfg_, dummy_cfg_unstable, kFallback));

        // A complete frame can still be too old. Check both data consistency above
        // and producer freshness here before trusting either input.
        const uint64_t now_us = utils::clock::now_us();
        const bool rc_fresh = has_last_rc_ &&
                              utils::liveness::fresh_us(now_us, r.stamp_us, cfg::safety::stale::RC_MS);
        const bool cfg_fresh = has_last_cfg_ &&
                               utils::liveness::fresh_us(now_us, c.stamp_us, cfg::safety::stale::CFG_MS);

        if (!rc_fresh)
        {
            // No recent RC update: discard the old controls and switch to failsafe.
            // In RC or Joystick mode, this produces a zero drive-output cap.
            r = RcSnapshot{};
            r.failsafe = true;
            r.stamp_us = now_us;
        }

        if (!cfg_fresh)
        {
            // Preserve a well-formed input for compute(); the decision is forced fully
            // safe below because neither the selected mode nor gear can be trusted.
            c = ConfigSnapshot{};
            c.gear = Gear::Unknown;
            c.stamp_us = now_us;
        }

        // Publish one decision that says who controls each function and how much
        // motor output the current policy allows.
        AuthoritySnapshot a = compute(r, c);

        if (!cfg_fresh)
        {
            // Losing configuration means losing the proof of who should control the
            // car. Stop all sources instead of silently falling back to another mode.
            a.owner_steer = Owner::None;
            a.owner_dir = Owner::None;
            a.owner_speed = Owner::None;
            a.owner_power = Owner::None;
            a.allowed_mask = kAllowNone;
            a.base_cap = cfg::units::MIN;
            a.gear_cap = cfg::units::MIN;
            a.eff_cap = cfg::units::MIN;
            a.reason_bits = kReasonConfigStale;

            if (r.failsafe)
            {
                a.reason_bits |= kReasonFailsafe;
            }
        }

        out_.publish(a);

        const uint32_t now_ms = utils::clock::now_ms();
        if constexpr (cfg::display::AUTHORITY_DEBUG)
        {
            hud_.display(a, now_ms);
        }
    }
}

// Infer available control sources from the DIP mode until a future service layer
// can also report whether each source is healthy.
OwnerMask AuthorityRouter::inferAllowedMask(CarMode mode) noexcept
{
    switch (mode)
    {
    case CarMode::RC:
        // RC is primary, but it may hand selected controls to the car.
        return static_cast<OwnerMask>(kAllowRC | kAllowDirect);
    case CarMode::Joystick:
        // RC and the assistive joystick can share control.
        return static_cast<OwnerMask>(kAllowRC | kAllowJoystick);
    case CarMode::OpenSlot:
        // The reserved expansion profile currently permits only on-car controls.
        return kAllowDirect;
    default:
        return kAllowDirect;
    }
}

// Resolve ownership for steering, direction, drive demand, and the output cap.
void AuthorityRouter::decideOwners(CarMode mode, RcDelegation d, Owner &steer,
                                   Owner &dir, Owner &speed, Owner &power) noexcept
{
    // The DIP switch chooses the main control profile. The RC three-position
    // switch then decides how much of RC/joystick control is delegated.
    // DIP 000: RC primary.
    // DIP 001: Joystick primary.
    // DIP 010: Reserved expansion profile; currently direct control.
    // DIP 011..111: Direct only.

    if (mode == CarMode::RC)
    {
        switch (d)
        {
        case RcDelegation::FullRC:
            steer = Owner::RC;
            dir = Owner::RC;
            speed = Owner::RC;
            power = Owner::RC;
            return;

        case RcDelegation::Mixed:
            // Direct owns steering; RC owns direction, drive demand, and the drive-output cap.
            steer = Owner::Direct;
            dir = Owner::RC;
            speed = Owner::RC;
            power = Owner::RC;
            return;

        case RcDelegation::Delegated:
            // Direct owns steering, direction, and drive demand; RC owns the drive-output cap.
            steer = Owner::Direct;
            dir = Owner::Direct;
            speed = Owner::Direct;
            power = Owner::RC;
            return;
        }
    }

    if (mode == CarMode::Joystick)
    {
        switch (d)
        {
        case RcDelegation::FullRC:
            steer = Owner::RC;
            dir = Owner::RC;
            speed = Owner::RC;
            power = Owner::RC;
            return;

        case RcDelegation::Mixed:
            // Joystick owns steering; RC owns direction, drive demand, and the drive-output cap.
            steer = Owner::Joystick;
            dir = Owner::RC;
            speed = Owner::RC;
            power = Owner::RC;
            return;

        case RcDelegation::Delegated:
        default:
            // Joystick owns steering, direction, and drive demand; RC owns the drive-output cap.
            steer = Owner::Joystick;
            dir = Owner::Joystick;
            speed = Owner::Joystick;
            power = Owner::RC;
            return;
        }
    }

    steer = Owner::Direct;
    dir = Owner::Direct;
    speed = Owner::Direct;
    power = Owner::Direct;
}

AuthoritySnapshot AuthorityRouter::compute(const RcSnapshot &r, const ConfigSnapshot &c) noexcept
{
    AuthoritySnapshot a{};

    // ---- Inputs ---- //
    a.mode = c.mode;
    a.gear = c.gear;
    a.rc_failsafe = r.failsafe;
    a.rc_delegation = a.rc_failsafe ? RcDelegation::FullRC : resolveDelegation(r);

    a.allowed_mask = inferAllowedMask(a.mode);
    decideOwners(a.mode, a.rc_delegation, a.owner_steer, a.owner_dir, a.owner_speed, a.owner_power);

    // ---- Drive-output caps (policy) ---- //

    // First calculate the cap set by the active control profile:
    //  - RC / Joystick: the RC power-cap knob drives the cap.
    //  - Direct: fixed mode policy limit.
    const bool is_rc_mode = (a.mode == CarMode::RC || a.mode == CarMode::Joystick);

    if (is_rc_mode)
    {
        // In RC/joystick profiles, the transmitter's power knob is the global cap.
        a.base_cap = a.rc_failsafe
                         ? cfg::units::MIN
                         : utils::mathfns::clamp_01(utils::mathfns::pct_to_01(rc_get(r, RC::power)));
    }
    else
    {
        // Direct profiles use the fixed policy cap selected by the DIP mode.
        a.base_cap = utils::mathfns::clamp_01(cfg::authority::modeSpeedLimit(a.mode));
    }

    // Record the base cap only when it actually reduces the available output.
    if (a.base_cap < 0.999f)
    {
        a.reason_bits |= kReasonBaseCap;
    }

    // Then decide whether the physical shifter also limits motor output:
    //  - In RC/Joystick FullRC/Mixed, RC owns drive demand and the local shifter is ignored.
    //  - In Delegated RC/Joystick control, the local shifter becomes a safety cap.
    //  - In Direct modes, the local shifter always applies.
    //
    // Result:
    //  - Local/Delegated control + Park/Unknown => no movement.
    //  - Full RC ignores the local shifter; the receiver supplies direction instead.
    const bool apply_gear_limit = (!is_rc_mode) || (a.owner_speed != Owner::RC);

    // A multiplier of 1.0 leaves the profile cap unchanged when RC owns drive demand.
    a.gear_cap = apply_gear_limit ? cfg::authority::gearSpeedLimit(a.gear) : 1.0f;

    // Unknown gear is a limiting reason only when the local shifter applies.
    if (apply_gear_limit && a.gear == Gear::Unknown)
    {
        a.reason_bits |= kReasonGearUnknown;
    }

    if (apply_gear_limit && a.gear == Gear::Park)
    {
        a.reason_bits |= kReasonGearPark;
    }

    // Multiplying the two caps produces the single limit applied by ControlCore.
    a.eff_cap = utils::mathfns::clamp_01(a.base_cap * a.gear_cap);

    // ---- Reasons ---- //

    // Record why the decision was limited or shared for diagnostics and telemetry.
    if (is_rc_mode && a.rc_delegation != RcDelegation::FullRC)
        a.reason_bits |= kReasonRouteDelegate;

    // Only report a gear cap when it is active and actually reduces output.
    if (apply_gear_limit && (a.gear != Gear::Unknown) && (a.gear_cap < 0.999f))
        a.reason_bits |= kReasonGearCap;
    // The tolerance prevents harmless floating-point noise from creating a reason bit.

    // Preserve the receiver failsafe as an explicit diagnostic reason.
    if (a.rc_failsafe)
        a.reason_bits |= kReasonFailsafe;

    // Consumers use this timestamp to reject an authority decision that becomes stale.
    a.stamp_us = utils::clock::now_us();
    return a;
}
