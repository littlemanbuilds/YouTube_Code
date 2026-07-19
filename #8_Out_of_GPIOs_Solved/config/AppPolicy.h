/**
 * MIT License
 *
 * @brief Application policies (timing, units, and safety windows).
 *
 * @file AppPolicy.h
 * @author Little Man Builds
 * @date 2026-01-21
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <cstdint>

namespace cfg
{
    // ---- Common units ---- //
    namespace units
    {
        constexpr float MIN = 0.0f;
        constexpr float MAX = 1.0f;
        constexpr float MAX_PCT = 100.0f;
    } ///< namespace units.

    // ---- Safety policy ---- //
    namespace safety::stale
    {
        // Control output watchdog.
        constexpr uint32_t CONTROL_MS = 100; ///< ControlSnapshot older than this => throttle=0.

        // Safety-critical, fast changing.
        constexpr uint32_t RC_MS = 150;  ///< RC samples must be fresh.
        constexpr uint32_t MCP_MS = 250; ///< MCP samples must be fresh.

        // Human-speed / discrete.
        constexpr uint32_t BTN_MS = 750; ///< Buttons may be quiet; heartbeat maintains liveness.
        // constexpr uint32_t CFG_MS = 2000; ///< Config can be quiet; heartbeat maintains liveness.
    } ///< namespace safety::stale

    // ---- Button timings ---- //
    namespace button
    {
        constexpr uint32_t BTN_DEBOUNCE_MS = 50;
        constexpr uint32_t BTN_SHORT_MS = 200;
        constexpr uint32_t BTN_LONG_MS = 1000;
    } ///< namespace button.

    // ---- Canonical cadences ---- //
    namespace tick
    {
        constexpr uint32_t LOOP_MS = 10;                   ///< Standard loop cadence.
        constexpr uint32_t LOOP_INTERVAL_TEST_SHORT = 100; ///< Short test ms.
        constexpr uint32_t LOOP_INTERVAL_TEST_LONG = 1000; ///< Long test ms.
    } ///< namespace tick
} ///< namespace cfg