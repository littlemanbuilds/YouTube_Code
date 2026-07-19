/**
 * MIT License
 *
 * @brief Read-only, cached, time-aware projection of McpBus state.
 *
 * @note Provides a single, shared representation of MCP23017 sampled state so
 *       publishers don't duplicate cache fields (gpioa/gpiob/gpio/ok).
 *
 * @file McpView.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2026-01-21
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <McpBus.h>
#include <cstdint>

namespace hal
{
    /**
     * @brief Cached view over the latest McpState frame.
     */
    class McpView
    {
    public:
        /**
         * @brief Construct a view bound to an MCP snapshot bus.
         *
         * @param bus Reference to the McpBus to read from.
         */
        explicit McpView(McpBus &bus) noexcept : bus_(bus) {}

        /// @brief Refresh cached snapshot (sinle best-effort peek).
        void refresh() noexcept
        {
            const bool stable = bus_.peek_into(cache_);
            if (!stable)
            {
                // Best-effort frame (write in-flight when bounded). Treat as unhealthy to prevent false edges.
                cache_.ok = false;
            }
        }

        /// @brief Cached combined GPIO word (bits 0..7=GPIOA, 8..15=GPIOB). Bit=1 means HIGH.
        [[nodiscard]] uint16_t gpio() const noexcept { return cache_.gpio; }

        /// @brief Cached GPIOA byte. Bit=1 means HIGH.
        [[nodiscard]] uint8_t gpioa() const noexcept { return cache_.gpioa; }

        /// @brief Cached GPIOB byte. Bit=1 means HIGH.
        [[nodiscard]] uint8_t gpiob() const noexcept { return cache_.gpiob; }

        /// @brief True if the cached sample is valide (e.g., MCP read succeeded).
        [[nodiscard]] bool ok() const noexcept { return cache_.ok; }

        /// @brief Cached sample timestamp (ms) captured when the snapshot was produced.
        [[nodiscard]] uint32_t stamp_ms() const noexcept { return cache_.stamp_ms; }

        /**
         * @brief Read logical assertion state for an active-low input.
         * @note Returns true when the signal is electrically LOW.
         *
         * @param bit MCP bit index (0..15). Bits 0..7 map to GPIOA; 8..15 map to GPIOB.
         */
        [[nodiscard]] bool asserted(uint8_t bit) const noexcept
        {
            const uint16_t mask = static_cast<uint16_t>(1u) << bit;
            return (cache_.gpio & mask) == 0u;
        }

    private:
        McpBus &bus_;      ///< Non-owning reference to the McpBus to read from.
        McpState cache_{}; ///< Cached McpState snapshot.
    };
} ///< namespace hal