/**
 * MIT License
 *
 * @brief RTOS parameters and canonical task descriptors.
 *
 * @file AppRtos.h
 * @author Little Man Builds
 * @date 2026-01-21
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cstdint>

namespace cfg
{
    // ---- RTOS parameters ---- //
    namespace rtos
    {
        constexpr BaseType_t CORE_IO = 0;   ///< I/O + potentially blocking work.
        constexpr BaseType_t CORE_CTRL = 1; ///< Control pipeline + actuators.

        constexpr UBaseType_t LOW_PRI = 0;   ///< Logging, stats, UI, serial printing.
        constexpr UBaseType_t IO_PRI = 1;    ///< Input publishers + I/O work.
        constexpr UBaseType_t LOGIC_PRI = 2; ///< Arbitration + decisions.
        constexpr UBaseType_t ACT_PRI = 3;   ///< Actuator control.

        // In FreeRTOs, higher numbers have higher priority.

        // Stack sizes are in words (4 bytes each on ESP32).
        constexpr uint32_t STACK_S = 2048; ///< ~8 KB.
        constexpr uint32_t STACK_M = 4096; ///< ~16 KB.
        constexpr uint32_t STACK_L = 6144; ///< ~24 KB (only if needed).

        /// @brief Convert milliseconds to FreeRTOS ticks.
        [[nodiscard]] inline TickType_t to_ticks_ms(uint32_t ms) noexcept { return pdMS_TO_TICKS(ms); }

    } ///< namespace rtos

    // ---- Task descriptors ---- //
    namespace task
    {
        /**
         * @brief Canonical task descriptor used when creating pinned FreeRTOS tasks.
         *
         * @note Stack sizes are in words (not bytes).
         */
        struct Desc
        {
            const char *name;     ///< Task name (shows in FreeRTOS tooling/debug).
            uint32_t stack;       ///< Stack size in words (ESP32: 4 bytes each).
            UBaseType_t priority; ///< Task priority (higher runs first when ready).
            BaseType_t core;      ///< CPU core to pin the task to.
        };

        // SnapshotRTOS.
        inline constexpr Desc RcPublisher{"RcPublisher", rtos::STACK_S, rtos::IO_PRI, rtos::CORE_IO};
        inline constexpr Desc McpPublisher{"McpPublisher", rtos::STACK_M, rtos::IO_PRI, rtos::CORE_IO};
        inline constexpr Desc ButtonPublisher{"ButtonPublisher", rtos::STACK_S, rtos::IO_PRI, rtos::CORE_IO};

        // App.
        inline constexpr Desc ControlCore{"ControlCore", rtos::STACK_S, rtos::LOGIC_PRI, rtos::CORE_CTRL};
        inline constexpr Desc PowerDriveHandler{"PDHandler", rtos::STACK_M, rtos::ACT_PRI, rtos::CORE_CTRL};

    } ///< namespace task
} ///< namespace cfg