/**
 * MIT License
 *
 * @brief Decodes the iBUS receiver into named controls and publishes their state.
 *
 * @file RcPublisher.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-10-08
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <RcBus.h>
#include <RCLink.h>

#include <cstddef>
#include <cstdint>

/**
 * @brief Owns receiver decoding, channel mapping, and RC failsafe behavior.
 */
class RcPublisher
{
public:
    /**
     * @brief Create a publisher bound to the shared RC output bus.
     *
     * @param bus Reference to the RcBus to publish into.
     * @param period_ms FreeRTOS poll period in milliseconds.
     */
    explicit RcPublisher(RcBus &bus, uint32_t period_ms = cfg::tick::LOOP_MS) noexcept;

    /// @brief Configure the receiver channels and start publishing RC snapshots.
    void begin() noexcept;

private:
    // ---- SnapshotRTOS callback adapter ---- //

    /**
     * @brief SnapshotRTOS update callback (advance one decode step).
     *
     * @param ctx Pointer to the RcLink instance.
     */
    static void cb_update(void *ctx) noexcept
    {
        static_cast<Link *>(ctx)->update();
    }

    /**
     * @brief SnapshotRTOS read callback (copy decoded RC channels into snapshot).
     *
     * @param ctx Pointer to the RcLink instance.
     * @param out Snapshot output structure filled with the latest RC state.
     */
    static void cb_read(void *ctx, RcSnapshot *out) noexcept
    {
        // RCLink supplies the link object registered as the callback context.
        auto &link = *static_cast<Link *>(ctx);
        const auto fr = link.frame();

        for (std::size_t i = 0; i < static_cast<std::size_t>(RC::Count); ++i)
        {
            out->out[i] = fr.vals[i]; // RCLink and RcSnapshot share the same role order.
        }
    }

    /**
     * @brief SnapshotRTOS health callback.
     *
     * @param ctx Pointer to the RcLink instance.
     * @return True when the input source is healthy; otherwise false.
     */
    static bool cb_ok(void *ctx) noexcept
    {
        // Either receiver-level or protocol-level failsafe makes this input unhealthy.
        const auto &st = static_cast<Link *>(ctx)->status();
        return !(st.rx_failsafe_sig || st.proto_failsafe);
    }

    // ---- Aliases ---- //
    using Transport = rc::RcIbusTransport;
    using Link = rc::RcLink<Transport, RC>;

    // ---- Internal state ---- //
    Transport ibus_{};     ///< iBUS transport instance (must outlive Link).
    Link rclink_{ibus_};   ///< RcLink bound to iBUS transport.
    RcBus &bus_;           ///< Reference to the RcBus to publish into.
    uint32_t period_ms_{}; ///< Poll period (ms) for SnapshotRTOS publisher.
};
