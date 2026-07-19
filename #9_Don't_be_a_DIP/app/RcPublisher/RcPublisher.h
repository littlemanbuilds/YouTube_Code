/**
 * MIT License
 *
 * @brief RC publisher: iBUS → RCLink → SnapshotBus (RCBus).
 *
 * @file RcPublisher.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-10-08
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <RcBus.h>
#include <RCLink.h>

#include <cstddef>
#include <cstdint>

/**
 * @brief Remote control listener task.
 */
class RcPublisher
{
public:
    /**
     * @brief Construct the RcPublisher task.
     *
     * @param bus Reference to the RcBus to publish into.
     * @param period_ms FreeRTOS poll period in milliseconds.
     */
    explicit RcPublisher(RcBus &bus, uint32_t period_ms = cfg::tick::LOOP_MS) noexcept;

    /// @brief Configure RCLink (axes, switches, etc.).
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
        auto &link = *static_cast<Link *>(ctx); ///< Recover RcLink instance from opaque context pointer.
        const auto fr = link.frame();           ///< Snapshot the most recent decoded RC frame.

        for (std::size_t i = 0; i < static_cast<std::size_t>(RC::Count); ++i)
        {
            out->out[i] = fr.vals[i]; ///< Copy RC channel values (assumes frame matches RC::Count).
        }
    }

    /**
     * @brief SnapshotRTOS health callback.
     *
     * @param ctx Pointer to the RcLink instance.
     * @return true if the input source is healthy; false otherwise.
     */
    static bool cb_ok(void *ctx) noexcept
    {
        const auto &st = static_cast<Link *>(ctx)->status(); ///< Read current RC link status flags from context.
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