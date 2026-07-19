/**
 * MIT License
 *
 * @brief Snapshot defining MCP23017 GPIO state (cached digital inputs).
 *
 * @file McpBus.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2026-01-20
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <SnapshotBus.h>

#include <cstdint>

/**
 * @brief Latest MCP23017 sampled GPIO state.
 */
struct McpState
{
    uint16_t gpio{0xFFFFu}; ///< Combined ports.

    uint8_t gpioa{0xFFu}; ///< Port A (GPA0..GPA7).
    uint8_t gpiob{0xFFu}; ///< Port B (GPB0..GPB7).

    bool ok{false};       ///< True if the latest MCP23017 sample is valid.
    uint32_t stamp_ms{0}; ///< Sample timestamp (ms) captured when snapshot was produced.
};

/// @brief Snapshot bus carrying MCP23017 samples.
using McpBus = snapshot::SnapshotBus<McpState>;