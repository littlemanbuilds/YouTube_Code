/**
 * MIT License
 *
 * @brief Carries the decision about who controls the car and how much output is allowed.
 *
 * @file AuthorityBus.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-19
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <ConfigBus.h>
#include <SnapshotBus.h>

#include <cstdint>

/**
 * @brief Possible owners of steering, direction, drive demand, or the output cap.
 * @note Ownership is decided separately for each function, allowing RC to keep
 *       overall supervision while handing selected controls to the car or joystick.
 */
enum class Owner : uint8_t
{
    None,    ///< Transitional or fault state (safe output only).
    RC,      ///< Remote control (may delegate per-axis).
    Direct,  ///< Direct child-operated analog control (accelerator, steering wheel).
    Joystick ///< Assistive joystick control (delegated).
};

/**
 * @brief Three ways RC can retain or share control with local/assistive inputs.
 */
enum class RcDelegation : uint8_t
{
    FullRC = 0,    ///< RC owns steering, direction, drive demand, and the drive-output cap.
    Mixed = 1,     ///< Direct/joystick owns steering; RC owns direction, drive demand, and the drive-output cap.
    Delegated = 2, ///< Direct/joystick owns steering, direction, and drive demand; RC owns the drive-output cap.
};

/**
 * @brief Bitmask listing the control sources permitted by the selected mode.
 * @note Permission and health are separate: a permitted source must still provide
 *       fresh, valid data before its commands are trusted. Values can be combined
 *       with bitwise OR (for example, kAllowRC | kAllowDirect):
 *
 *       kAllowRC       = 0b001  (Remote control permitted).
 *       kAllowDirect   = 0b010  (Direct / local control permitted).
 *       kAllowJoystick = 0b100  (Joystick control permitted).
 */
enum OwnerMask : uint8_t
{
    kAllowNone = 0,           ///< No owners permitted (safe / fault state).
    kAllowRC = 1u << 0,       ///< RC permitted.
    kAllowDirect = 1u << 1,   ///< Direct child-operated analog control permitted.
    kAllowJoystick = 1u << 2, ///< Assistive joystick control permitted.
};

/**
 * @brief Machine-readable reasons why authority was shared, limited, or stopped.
 * @note Multiple reasons can be combined in AuthoritySnapshot::reason_bits for
 *       diagnostics without changing the control decision itself.
 */
enum AuthorityReasonBits : uint32_t
{
    kReasonNone = 0u,                 ///< No diagnostic authority reason is active.
    kReasonRouteDelegate = 1u << 0,  ///< RC/Joystick mode delegates at least one control axis.
    kReasonGearCap = 1u << 1,        ///< Applied gear policy reduces the effective cap.
    kReasonBaseCap = 1u << 2,        ///< Mode/RC cap policy reduces the base cap.
    kReasonFailsafe = 1u << 3,       ///< RC link is in failsafe.
    kReasonGearUnknown = 1u << 4,    ///< Required local gear state is unknown.
    kReasonGearPark = 1u << 5,       ///< Applied local gear state is Park.
    kReasonConfigStale = 1u << 6,    ///< Configuration updates stopped; authority failed closed.
};

/**
 * @brief One complete authority decision consumed by the rest of the control pipeline.
 */
struct AuthoritySnapshot
{
    // ---- Inputs ---- //
    CarMode mode{CarMode::RC};                        ///< DIP-selected primary mode.
    RcDelegation rc_delegation{RcDelegation::FullRC}; ///< Delegation policy while RC participates.
    Gear gear{Gear::Unknown};                         ///< Current gear selection.
    bool rc_failsafe{false};                          ///< True if RC link is in failsafe.

    // ---- Decisions ---- //
    Owner owner_steer{Owner::None}; ///< Authority for steering.
    Owner owner_dir{Owner::None};   ///< Authority for direction (Fwd/Rev).
    Owner owner_speed{Owner::None}; ///< Authority for drive demand (throttle request).
    Owner owner_power{Owner::None}; ///< Authority for the drive-output cap.

    OwnerMask allowed_mask{kAllowNone}; ///< Allowed owners (inferred from mode / DIP policy).

    // ---- Caps (normalized 0.0 .. 1.0) ---- //
    float base_cap{1.0f}; ///< Base cap from mode/profile policy and the RC cap input when applicable.
    float gear_cap{1.0f}; ///< Gear-derived cap.
    float eff_cap{1.0f};  ///< Effective cap (base_cap * gear_cap).

    // ---- Reasons retained for engineering diagnostics and telemetry ---- //
    uint32_t reason_bits{kReasonNone}; ///< Bitmask of decision reasons (see AuthorityReasonBits).
    uint64_t stamp_us{0};              ///< Timestamp (us) captured when snapshot was produced.
};

// Thread-safe bus carrying the latest complete authority decision.
using AuthorityBus = snapshot::SnapshotBus<AuthoritySnapshot>;
