/**
 * MIT License
 *
 * @brief RC publisher: iBUS → RCLink → SnapshotBus (RCBus).
 *
 * @file RcPublisher.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-10-08
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#pragma once

#include <app_config.h>
#include <cstdint>
#include <cstddef>
#include <RCLink.h>
#include <SnapshotBus.h>
#include <SnapshotRTOS.h>
#include <RcBus.h>

/**
 * @brief Remote control listener task.
 */
class RcPublisher
{
public:
    /**
     * @brief Construct with change-notification settings.
     *
     * @param period_ms FreeRTOS tick interval used to pace the run loop (in milliseconds).
     * @param epsilon Publish if any |delta| > epsilon (0 → always on change).
     * @param min_interval_ms Publish at least every this many ms (0 → disabled).
     */
    explicit RcPublisher(uint32_t period_ms = cfg::tick::LOOP_MS, float epsilon = 0,
                         uint32_t min_interval_ms = 0) noexcept;

    /**
     * @brief Configure RCLink (axes, switches, etc.).
     */
    void begin() noexcept;

private:
    // ---- Aliases ---- //
    using Transport = rc::RcIbusTransport;
    using Link = rc::RcLink<Transport, RC>;

    // ---- Internal state ---- //
    Transport ibus_{};           ///< iBUS transport (must outlive Link).
    Link rclink_{ibus_};         ///< RcLink bound to iBUS.
    TickType_t period_ms_{0};    ///< Delay (in ticks) between loop iterations.
    float eps_{};                ///< Change gate: publish when any |delta| exceeds this (0.0f = always publish).
    uint32_t min_interval_ms_{}; ///< Heartbeat interval (milliseconds): publish at least this often (0 = disabled).

    // ---- Reader that adapts RcLink to SnapshotRTOS ---- //
    struct Reader
    {
        Link *link{nullptr}; ///< RcLink instance that already speaks iBUS and maps channels to RC roles.

        /// @brief Poll the receiver and decode fresh bytes.
        void update()
        {
            link->update(); ///< Pull latest data from UART and refesh RcLink's frame/state.
        }

        /// @brief Copy channels in the publish buffer.
        void read(float *dst, size_t n)
        {
            if (!dst || n == 0)
                return; ///< No destination / nothing to write.

            const auto fr = link->frame();                   ///< Current mapped values.
            const size_t M = static_cast<size_t>(RC::Count); ///< Total channels defined by RC enum.
            const size_t m = (n < M) ? n : M;                ///< Copy only what fits into dst (destination).

            // RcLink stores channels values as int16_t → publish as float (avoids per channel scaling).
            for (size_t i = 0; i < m; ++i)
                dst[i] = static_cast<float>(fr.vals[i]); ///< int16_t → float.
        }

        /// @brief Health check: true → link is OK (not in failsafe).
        bool ok() const
        {
            const auto &st = link->status();                   ///< Current RX/protocol status.
            return !(st.rx_failsafe_sig || st.proto_failsafe); ///< If either asserts failsafe → not OK.
        }
    };
};