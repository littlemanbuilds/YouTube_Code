/**
 * MIT License
 *
 * @brief Application policies (timing, units, and safety windows).
 *
 * @file AppPolicy.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-21
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <cstdint>

namespace cfg
{
    /// @brief Common units.
    namespace units
    {
        constexpr float MIN = 0.0f;
        constexpr float MAX = 1.0f;
        constexpr float MAX_PCT = 100.0f;
    } ///< namespace cfg::units

    /// @brief Maximum data ages before an input is treated as stale and unsafe.
    namespace safety::stale
    {
        // The motor task stops if final control commands stop arriving.
        constexpr uint32_t CONTROL_MS = 100; ///< Older control command forces zero motor output.

        // Fast-changing inputs that directly affect movement.
        constexpr uint32_t AUTH_MS = 150; ///< Authority decision must be fresh to trust.
        constexpr uint32_t RC_MS = 150;   ///< RC samples must be fresh.
        constexpr uint32_t MCP_MS = 250;  ///< MCP samples must be fresh.

        // Slower-changing human inputs, kept alive by publisher heartbeats.
        constexpr uint32_t BTN_MS = 750;  ///< Buttons may be quiet; heartbeat keeps the frame recent.
        constexpr uint32_t CFG_MS = 2000; ///< Config may be quiet; heartbeat keeps the frame recent.
    } ///< namespace cfg::safety::stale

    /// @brief Button timings.
    namespace button
    {
        constexpr uint32_t BTN_DEBOUNCE_MS = 50;
        constexpr uint32_t BTN_SHORT_MS = 200;
        constexpr uint32_t BTN_LONG_MS = 1000;
    } ///< namespace cfg::button

    /// @brief Shared loop and demonstration timings.
    namespace tick
    {
        constexpr uint32_t LOOP_MS = 10;                   ///< Standard loop cadence.
        constexpr uint32_t LOOP_INTERVAL_TEST_SHORT = 100; ///< Short demonstration interval.
        constexpr uint32_t LOOP_INTERVAL_TEST_LONG = 1000; ///< Long demonstration interval.
    } ///< namespace cfg::tick
} ///< namespace cfg
