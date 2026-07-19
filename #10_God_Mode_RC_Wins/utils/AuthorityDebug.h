/**
 * MIT License
 *
 * @brief Presents the authority decision as one compact, audience-readable HUD line.
 * @note This file only explains the decision; AuthorityRouter still owns the policy.
 *
 * @file AuthorityDebug.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-05-19
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AuthorityBus.h>
#include <Diagnostic.h>
#include <Debugging.h>

#include <cmath>
#include <cstdint>

namespace utils::authority_debug
{
    /**
     * @brief Choose when the authority heads-up display prints.
     *
     * @note
     * - Off: no output.
     * - ChangesOnly: prints only when key authority fields change.
     * - Heartbeat: prints on change or periodically.
     */
    enum class HudMode : std::uint8_t
    {
        Off = 0,         ///< No HUD output.
        ChangesOnly = 1, ///< Print only when authority state changes.
        Heartbeat = 2    ///< Print on change, plus periodic heartbeat lines.
    };

    /**
     * @brief Shows who controls the car and what output limit currently applies.
     */
    class Hud
    {
    public:
        /**
         * @brief Construct an authority HUD helper.
         *
         * @param mode Output mode.
         * @param heartbeat_ms Heartbeat interval in milliseconds for Heartbeat mode.
         * @param eps Floating-point change threshold for normalized cap fields.
         */
        explicit Hud(HudMode mode = HudMode::ChangesOnly,
                     std::uint32_t heartbeat_ms = 1000u,
                     float eps = 0.005f) noexcept
            : mode_(mode), heartbeat_ms_(heartbeat_ms), eps_(eps)
        {
        }

        /**
         * @brief Set the HUD output mode.
         *
         * @param m New output mode.
         */
        void setMode(HudMode m) noexcept { mode_ = m; }

        /**
         * @brief Get the current HUD output mode.
         *
         * @return Current output mode.
         */
        [[nodiscard]] HudMode mode() const noexcept { return mode_; }

        /**
         * @brief Set the heartbeat interval used by Heartbeat mode.
         *
         * @param ms Heartbeat interval in milliseconds.
         */
        void setHeartbeatMs(std::uint32_t ms) noexcept { heartbeat_ms_ = ms; }

        /**
         * @brief Call once per loop with the latest authority snapshot.
         *
         * @param a Latest authority snapshot.
         * @param now_ms Current monotonic time in milliseconds.
         */
        void display(const AuthoritySnapshot &a, std::uint32_t now_ms) noexcept
        {
            if (mode_ == HudMode::Off)
            {
                // Keep tracking state while hidden so re-enabling the HUD does not
                // report an old transition as if it just happened.
                last_ = a;
                has_last_ = true;
                return;
            }

            bool should_print = false;

            if (!has_last_)
            {
                should_print = true; // First visible line.
            }
            else
            {
                const bool changed = differs_(a, last_);

                if (mode_ == HudMode::ChangesOnly)
                {
                    should_print = changed;
                }
                else
                {
                    const std::uint32_t dt = now_ms - last_print_ms_;
                    should_print = changed || (dt >= heartbeat_ms_);
                }
            }

            if (!should_print)
            {
                last_ = a;
                has_last_ = true;
                return;
            }

            last_print_ms_ = now_ms;
            last_ = a;
            has_last_ = true;

            print_(a);
        }

    private:
        /**
         * @brief Determine whether two authority snapshots differ meaningfully.
         *
         * @param a New authority snapshot.
         * @param b Previous authority snapshot.
         * @return True when the snapshots differ enough to print; otherwise false.
         */
        [[nodiscard]] bool differs_(const AuthoritySnapshot &a, const AuthoritySnapshot &b) const noexcept
        {
            // Ignore tiny floating-point noise that would otherwise spam the Serial Monitor.
            const auto fdiff = [this](float x, float y) noexcept -> bool
            {
                return std::fabs(x - y) > eps_;
            };

            return (a.mode != b.mode) ||
                   (a.gear != b.gear) ||
                   (a.rc_delegation != b.rc_delegation) ||
                   (a.owner_steer != b.owner_steer) ||
                   (a.owner_dir != b.owner_dir) ||
                   (a.owner_speed != b.owner_speed) ||
                   (a.owner_power != b.owner_power) ||
                   (a.rc_failsafe != b.rc_failsafe) ||
                   ((a.reason_bits & kReasonConfigStale) != (b.reason_bits & kReasonConfigStale)) ||
                   fdiff(a.eff_cap, b.eff_cap) ||
                   fdiff(a.base_cap, b.base_cap) ||
                   fdiff(a.gear_cap, b.gear_cap);
        }

        /**
         * @brief Emit one formatted authority HUD line.
         *
         * @param a Authority snapshot to print.
         *
         * @details
         * The HUD answers the audience-facing questions in a compact order:
         *
         * - Active mode and RC delegation, when relevant.
         * - Owners of steering, drive (direction/demand), and the drive-output cap.
         * - Effective drive-output cap, including the gear calculation when applied.
         * - Plain fault markers only when active (for example FS! or GEAR?).
         *
         * Internal permission and reason masks remain available in AuthoritySnapshot,
         * but are intentionally omitted from this presentation-oriented view.
         */
        void print_(const AuthoritySnapshot &a) noexcept
        {
            const bool rc_delegation_relevant =
                (a.mode == CarMode::RC) ||
                (a.mode == CarMode::Joystick);

            const char *delegation_label = rc_delegation_relevant ? " rcm=" : "";
            const char *delegation_tok = rc_delegation_relevant
                                             ? utils::diag::rcdel_tok(a.rc_delegation)
                                             : "";

            // Direction and drive demand currently move together. Preserve visibility if a
            // future policy splits them by showing both owners as drive=DIR/DEMAND.
            const bool drive_owner_split = (a.owner_dir != a.owner_speed);
            const char *drive_sep = drive_owner_split ? "/" : "";
            const char *speed_owner_tok = drive_owner_split
                                              ? utils::diag::owner_tok(a.owner_speed)
                                              : "";

            // Match AuthorityRouter's policy: FullRC/Mixed ignore the local shifter;
            // delegated and direct modes include its cap in the effective limit.
            const bool gear_cap_applied =
                !rc_delegation_relevant || (a.owner_speed != Owner::RC);

            const bool config_stale = (a.reason_bits & kReasonConfigStale) != 0u;
            const char *fault_tok = a.rc_failsafe && config_stale ? " FS! CFG!"
                                    : a.rc_failsafe                 ? " FS!"
                                    : config_stale                 ? " CFG!"
                                                                   : "";

            static constexpr float kToPct = 100.0f;
            const double base_pct = static_cast<double>(a.base_cap * kToPct);
            const double gear_pct = static_cast<double>(a.gear_cap * kToPct);
            const double effective_pct = static_cast<double>(a.eff_cap * kToPct);

            if (gear_cap_applied)
            {
                const char *gear_tok = (a.gear == Gear::Unknown)
                                           ? "GEAR?"
                                           : utils::diag::gear_tok(a.gear);

                debugfln(
                    "AUTH: mode=%s%s%s steer=%s drive=%s%s%s limit=%s "
                    "cap=%.0f%%x%.0f%%=%.0f%% gear=%s%s",
                    utils::diag::carmode_tok(a.mode),
                    delegation_label,
                    delegation_tok,
                    utils::diag::owner_tok(a.owner_steer),
                    utils::diag::owner_tok(a.owner_dir),
                    drive_sep,
                    speed_owner_tok,
                    utils::diag::owner_tok(a.owner_power),
                    base_pct,
                    gear_pct,
                    effective_pct,
                    gear_tok,
                    fault_tok);
            }
            else
            {
                debugfln(
                    "AUTH: mode=%s%s%s steer=%s drive=%s%s%s limit=%s cap=%.0f%%%s",
                    utils::diag::carmode_tok(a.mode),
                    delegation_label,
                    delegation_tok,
                    utils::diag::owner_tok(a.owner_steer),
                    utils::diag::owner_tok(a.owner_dir),
                    drive_sep,
                    speed_owner_tok,
                    utils::diag::owner_tok(a.owner_power),
                    effective_pct,
                    fault_tok);
            }
        }

    private:
        HudMode mode_{HudMode::ChangesOnly}; ///< Current output mode.
        std::uint32_t heartbeat_ms_{1000u};  ///< Heartbeat interval in milliseconds.
        float eps_{0.005f};                  ///< Float change threshold for cap fields.

        AuthoritySnapshot last_{};        ///< Last snapshot used to detect changes.
        bool has_last_{false};            ///< True once last_ is initialized.
        std::uint32_t last_print_ms_{0u}; ///< Timestamp in milliseconds of last HUD line.
    };

} ///< namespace utils::authority_debug
