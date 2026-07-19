/**
 * MIT License
 *
 * @brief Gives each consumer a local, read-only view of the latest MCP23017 sample.
 * @note A failed bus read marks only that consumer's cache unhealthy, preventing
 *       uncertain electrical data from becoming a button or switch event.
 *
 * @file McpView.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-21
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <McpBus.h>
#include <cstdint>

namespace hal
{
    /**
     * @brief Local cache with helpers for raw and active-low input states.
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

        /// @brief Refresh the cache from one complete McpBus snapshot.
        void refresh() noexcept
        {
            const bool stable = bus_.peek_into(cache_);
            if (!stable)
            {
                // A publish overlapped this bounded read. Reject the uncertain frame
                // so it cannot create a false button or switch edge.
                cache_.ok = false;
            }
        }

        /// @brief Cached combined GPIO word (bits 0..7=GPIOA, 8..15=GPIOB). Bit=1 means HIGH.
        [[nodiscard]] uint16_t gpio() const noexcept { return cache_.gpio; }

        /// @brief Cached GPIOA byte. Bit=1 means HIGH.
        [[nodiscard]] uint8_t gpioa() const noexcept { return cache_.gpioa; }

        /// @brief Cached GPIOB byte. Bit=1 means HIGH.
        [[nodiscard]] uint8_t gpiob() const noexcept { return cache_.gpiob; }

        /// @brief True when both the bus read and underlying MCP23017 sample are valid.
        [[nodiscard]] bool ok() const noexcept { return cache_.ok; }

        /// @brief Cached sample timestamp (ms) captured when the snapshot was produced.
        [[nodiscard]] uint32_t stamp_ms() const noexcept { return cache_.stamp_ms; }

        /**
         * @brief Read raw electrical level for a bit (true == HIGH).
         *
         * @param bit MCP bit index (0..15). Bits 0..7 map to GPIOA; 8..15 map to GPIOB.
         */
        [[nodiscard]] bool level_high(uint8_t bit) const noexcept
        {
            if (bit >= 16u)
            {
                return true; // Out-of-range active-low input defaults to inactive.
            }

            const uint16_t mask = static_cast<uint16_t>(1u) << bit;
            return (cache_.gpio & mask) != 0u;
        }

        /**
         * @brief Read logical assertion state for an active-low input.
         * @note Returns true when the signal is electrically LOW.
         *
         * @param bit MCP bit index (0..15). Bits 0..7 map to GPIOA; 8..15 map to GPIOB.
         */
        [[nodiscard]] bool asserted(uint8_t bit) const noexcept
        {
            if (bit >= 16u)
            {
                return false; // Invalid input must never become a press or switch event.
            }

            const uint16_t mask = static_cast<uint16_t>(1u) << bit;
            return (cache_.gpio & mask) == 0u;
        }

    private:
        McpBus &bus_;      ///< Non-owning reference to the McpBus to read from.
        McpState cache_{}; ///< Cached McpState snapshot.
    };
} ///< namespace hal
