/**
 * MIT License
 *
 * @brief RC listener: iBUS → RCLink → SnapshotBus (RCBus).
 *
 * @file RcListener.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-10-08
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#pragma once

#include <app_config.h>
#include <RCLink.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/**
 * @brief Remote control listener task
 */
class RcListener
{
public:
    /**
     * @brief Construct with change-notification settings.
     *
     * @param period_ms FreeRTOS tick interval used to pace the run loop (in milliseconds).
     */
    explicit RcListener(uint32_t period_ms = cfg::tick::LOOP_MS) noexcept;

    /**
     * @brief Configure RCLink (axes, switches, etc.).
     */
    void begin() noexcept;

    /**
     * @brief FreeRTOS task trampoline. Call with `pvParameters = this`.
     */
    static inline void task(void *self) noexcept
    {
        static_cast<RcListener *>(self)->run();
    }

private:
    /// @brief Main run loop.
    void run() noexcept;

    // ---- Aliases ---- //
    using Transport = rc::RcIbusTransport;
    using Link = rc::RcLink<Transport, RC>;

    // ---- State ---- //
    Transport ibus_{};         ///< iBUS transport (must outlive Link).
    Link rclink_{ibus_};       ///< RcLink bound to iBUS.
    TickType_t loop_ticks_{0}; ///< Delay (in ticks) between loop iterations.
};