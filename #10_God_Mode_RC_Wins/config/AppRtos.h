/**
 * MIT License
 *
 * @brief Defines where each FreeRTOS task runs and how many resources it receives.
 *
 * @file AppRtos.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-21
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cstdint>

namespace cfg
{
    /// @brief CPU placement, priorities, and reusable stack sizes.
    namespace rtos
    {
        constexpr BaseType_t CORE_IO = 0;   ///< I/O + potentially blocking work.
        constexpr BaseType_t CORE_CTRL = 1; ///< Control pipeline + actuators.

        constexpr UBaseType_t LOW_PRI = 0;   ///< Logging and display work that may wait.
        constexpr UBaseType_t IO_PRI = 1;    ///< Hardware sampling and input publishers.
        constexpr UBaseType_t LOGIC_PRI = 2; ///< Ownership and command decisions.
        constexpr UBaseType_t ACT_PRI = 3;   ///< Motor output; highest application priority.

        // Stack sizes are in words (4 bytes each on ESP32).
        constexpr uint32_t STACK_S = 2048; ///< ~8 KB.
        constexpr uint32_t STACK_M = 4096; ///< ~16 KB.
        constexpr uint32_t STACK_L = 6144; ///< ~24 KB (only if needed).

        /// @brief Convert milliseconds to FreeRTOS ticks.
        [[nodiscard]] inline TickType_t to_ticks_ms(uint32_t ms) noexcept { return pdMS_TO_TICKS(ms); }
    } ///< namespace cfg::rtos

    /// @brief One reviewed place for every application task's name, stack, priority, and core.
    namespace task
    {
        /**
         * @brief Resource and CPU-placement settings used when creating a task.
         * @note Stack sizes are in words (not bytes).
         */
        struct Desc
        {
            const char *name;     ///< Task name (shows in FreeRTOS tooling/debug).
            uint32_t stack;       ///< Stack size in words (ESP32: 4 bytes each).
            UBaseType_t priority; ///< Task priority (higher runs first when ready).
            BaseType_t core;      ///< CPU core to pin the task to.
        };

        // Input publishers.
        inline constexpr Desc RcPublisher{"RcPublisher", rtos::STACK_S, rtos::IO_PRI, rtos::CORE_IO};
        inline constexpr Desc McpPublisher{"McpPublisher", rtos::STACK_M, rtos::IO_PRI, rtos::CORE_IO};
        inline constexpr Desc ButtonPublisher{"ButtonPublisher", rtos::STACK_S, rtos::IO_PRI, rtos::CORE_IO};
        inline constexpr Desc ConfigPublisher{"ConfigPublisher", rtos::STACK_S, rtos::IO_PRI, rtos::CORE_IO};

        // Decision, actuator, and diagnostic tasks.
        inline constexpr Desc AuthorityRouter{"AuthorityRouter", rtos::STACK_M, rtos::LOGIC_PRI, rtos::CORE_CTRL};
        inline constexpr Desc ConfigDebug{"ConfigDebug", rtos::STACK_S, rtos::LOW_PRI, rtos::CORE_IO};
        inline constexpr Desc ControlCore{"ControlCore", rtos::STACK_S, rtos::LOGIC_PRI, rtos::CORE_CTRL};
        inline constexpr Desc PowerDriveHandler{"PowerDriveHandler", rtos::STACK_M, rtos::ACT_PRI, rtos::CORE_CTRL};
        inline constexpr Desc StackWatch{"StackWatch", rtos::STACK_M, rtos::LOW_PRI, rtos::CORE_IO};
    } ///< namespace cfg::task
} ///< namespace cfg
