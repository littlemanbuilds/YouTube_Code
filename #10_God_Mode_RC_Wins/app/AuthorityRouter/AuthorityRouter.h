/**
 * MIT License
 *
 * @brief Decides who controls each function and how much motor output is allowed.
 *
 * @file AuthorityRouter.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-02
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <AuthorityBus.h>
#include <AuthorityDebug.h>
#include <ConfigBus.h>
#include <RcBus.h>

/**
 * @brief Combines operating mode, gear, and RC state into a fail-safe authority snapshot.
 */
class AuthorityRouter
{
public:
    /**
     * @brief Create the router and bind its input and output buses.
     *
     * @param rc Reference to the RcBus providing remote control input snapshots.
     * @param cfg Reference to the ConfigBus providing DIP mode and gear snapshots.
     * @param out Reference to the AuthorityBus to publish authority decisions into.
     * @param period_ms Execution period in milliseconds; defaults to the project loop cadence.
     */
    AuthorityRouter(RcBus &rc,
                    ConfigBus &cfg,
                    AuthorityBus &out,
                    uint32_t period_ms = cfg::tick::LOOP_MS) noexcept;

    /**
     * @brief FreeRTOS task trampoline. Call with `pvParameters = this`.
     */
    static inline void task(void *self) noexcept
    {
        static_cast<AuthorityRouter *>(self)->run();
    }

private:
    /// @brief Continuously refresh and publish the system authority decision.
    void run() noexcept;

    // ---- Decisions ---- //

    /**
     * @brief Infer available control sources from DIP mode until provider health is available.
     *
     * @param mode DIP-selected primary mode.
     * @return Bitmask of permitted owners/providers.
     */
    [[nodiscard]] static OwnerMask inferAllowedMask(CarMode mode) noexcept;

    /**
     * @brief Resolve per-axis ownership based on DIP mode + delegation policy.
     *
     * @param mode DIP-selected primary mode.
     * @param rc_delegation Delegation policy (FullRC/Mixed/Delegated).
     * @param steer Owner for steering axis (output).
     * @param dir Owner for direction axis (output).
     * @param speed Receives the owner of drive demand (throttle request).
     * @param power Receives the owner of the drive-output cap.
     */
    static void decideOwners(CarMode mode,
                             RcDelegation rc_delegation,
                             Owner &steer,
                             Owner &dir,
                             Owner &speed,
                             Owner &power) noexcept;

    /**
     * @brief Compute one AuthoritySnapshot from current RC and configuration snapshots.
     *
     * @param r Current RC state, or the most recent complete fallback frame.
     * @param c Current mode and gear state, or the most recent complete fallback frame.
     * @return Fully computed authority decision containing owners, caps, and timestamp.
     */
    [[nodiscard]] static AuthoritySnapshot compute(const RcSnapshot &r, const ConfigSnapshot &c) noexcept;

    // ---- Dependency buses ---- //
    RcBus &rc_;         ///< Reference to the RcBus providing RC input snapshots.
    ConfigBus &cfg_;    ///< Reference to the ConfigBus providing DIP/gear snapshots.
    AuthorityBus &out_; ///< Reference to the AuthorityBus receiving published authority decisions.

    // ---- Scheduling ---- //
    TickType_t loop_ticks_{0}; ///< Fixed control-loop period (ticks).

    // ---- Last complete snapshots ---- //
    RcSnapshot last_rc_{};      ///< Reused if an RC read overlaps a publish.
    ConfigSnapshot last_cfg_{}; ///< Reused if a configuration read overlaps a publish.

    // ---- Snapshot validity flags ---- //
    bool has_last_rc_{false};  ///< False until the first complete RC snapshot.
    bool has_last_cfg_{false}; ///< False until the first complete configuration snapshot.

    // ---- HUD / telemetry ---- //
    utils::authority_debug::Hud hud_{utils::authority_debug::HudMode::Heartbeat};
};
