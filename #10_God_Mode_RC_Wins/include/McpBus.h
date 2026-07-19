/**
 * MIT License
 *
 * @brief Carries one sampled view of all MCP23017 digital inputs and their health.
 *
 * @file McpBus.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-20
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <SnapshotBus.h>

#include <cstdint>

/**
 * @brief Latest complete sample from both MCP23017 GPIO ports.
 */
struct McpState
{
    uint16_t gpio{0xFFFFu}; ///< Combined ports: GPIOA in bits 0..7, GPIOB in bits 8..15.

    uint8_t gpioa{0xFFu}; ///< Port A (GPA0..GPA7).
    uint8_t gpiob{0xFFu}; ///< Port B (GPB0..GPB7).

    bool ok{false};       ///< True if the latest MCP23017 sample is valid.
    uint32_t stamp_ms{0}; ///< Sample timestamp (ms) captured when snapshot was produced.
};

/// @brief Thread-safe bus carrying the latest complete MCP23017 sample.
using McpBus = snapshot::SnapshotBus<McpState>;
