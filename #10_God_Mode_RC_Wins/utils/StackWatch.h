/**
 * MIT License
 *
 * @brief Shows how much heap and task stack the application is actually using.
 * @note The stack high-water mark is the smallest amount of unused stack observed.
 *       Only registered application tasks are shown; ESP32 system tasks are excluded.
 *
 * @file StackWatch.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-31
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <Arduino.h>
#include <AppConfig.h>
#include <Debugging.h>
#include <SnapshotRTOS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cstddef>
#include <cstdint>

namespace utils::stack_watch
{
    /**
     * @brief Configuration parameters for the StackWatch task.
     */
    struct Params
    {
        uint32_t period_ms{2000}; ///< Print cadence (milliseconds).
    };

    namespace detail
    {
        // Warn when measured stack use approaches the configured allocation.
        constexpr uint32_t kWarnPct = 80;
        constexpr uint32_t kCritPct = 90;

        // Shared widths keep the Serial Monitor table readable.
        constexpr int kColName = 21;
        constexpr int kColPri = 3;
        constexpr int kColCore = 4;
        constexpr int kColHwm = 6;

        /**
         * @brief Convert used and allocated bytes into a rounded usage percentage.
         */
        [[nodiscard]] inline uint32_t pct_used(uint32_t used_b, uint32_t alloc_b) noexcept
        {
            if (alloc_b == 0u)
            {
                return 0u;
            }

            // Round to nearest integer percent.
            const uint32_t num = (used_b * 100u) + (alloc_b / 2u);
            return num / alloc_b;
        }

        /**
         * @brief Print the registered-task table header.
         */
        inline void print_header() noexcept
        {
            debugfln("%-*s %*s  %*s  %*s   %s",
                     kColName, "name",
                     kColPri, "pri",
                     kColCore, "core",
                     kColHwm, "hwm(B)",
                     "used/alloc(B)");
        }

        /**
         * @brief Print one registered task and flag high stack use.
         */
        inline void print_row(const char *name,
                              unsigned pri,
                              int core_id,
                              unsigned hwm_b,
                              unsigned used_b,
                              unsigned alloc_b,
                              uint32_t pct) noexcept
        {
            const char *warn = "";

            if (pct >= kCritPct)
            {
                warn = "  !!! CRIT";
            }
            else if (pct >= kWarnPct)
            {
                warn = "  !! WARN";
            }

            debugfln("%-*s %*u  %*d  %*u   %*u/%*u%s",
                     kColName, name,
                     kColPri, pri,
                     kColCore, core_id,
                     kColHwm, hwm_b,
                     kColHwm, used_b,
                     kColHwm, alloc_b,
                     warn);
        }

        /**
         * @brief Print stack use for every registered application task.
         * @note Only tasks registered via snapshot::rtos::register_task() will appear.
         */
        inline void print_registered_tasks() noexcept
        {
            print_header();

            const uint32_t count = snapshot::rtos::task_meta_count();

            for (uint32_t i = 0; i < count; ++i)
            {
                const snapshot::rtos::TaskMeta *meta = snapshot::rtos::task_meta_at(i);
                if (!meta || !meta->handle || !meta->name)
                {
                    continue;
                }

                const UBaseType_t hwm_w = snapshot::rtos::stack_high_water_mark_words(meta->handle);

                const uint32_t hwm_b =
                    snapshot::rtos::stack_words_to_bytes(static_cast<uint32_t>(hwm_w));

                const uint32_t alloc_b =
                    snapshot::rtos::stack_words_to_bytes(meta->stack_words);

                const uint32_t used_w =
                    snapshot::rtos::stack_used_words(meta->stack_words,
                                                     static_cast<uint32_t>(hwm_w));

                const uint32_t used_b =
                    snapshot::rtos::stack_words_to_bytes(used_w);

                const uint32_t pct =
                    pct_used(used_b, alloc_b);

                const unsigned prio =
                    static_cast<unsigned>(uxTaskPriorityGet(meta->handle));

                const int core_id =
                    static_cast<int>(meta->core);

                print_row(meta->name,
                          prio,
                          core_id,
                          static_cast<unsigned>(hwm_b),
                          static_cast<unsigned>(used_b),
                          static_cast<unsigned>(alloc_b),
                          pct);
            }

            if (count == 0)
            {
                debugln("(StackWatch: no registered tasks to report.)");
            }
        }

    } ///< namespace utils::stack_watch::detail

    /**
     * @brief FreeRTOS task entrypoint for periodic stack/heap diagnostics.
     *
     * @param pv Pointer to Params (optional). Must outlive the task if provided.
     */
    inline void task(void *pv) noexcept
    {
        const auto *p = static_cast<const Params *>(pv);
        const uint32_t period_ms = p ? p->period_ms : 2000u;

        TickType_t last = xTaskGetTickCount();

        for (;;)
        {
            const uint32_t heap_free =
                static_cast<uint32_t>(ESP.getFreeHeap());

            const uint32_t heap_min =
                static_cast<uint32_t>(ESP.getMinFreeHeap());

            {
                // Lock the table output so debug lines stay aligned.
                utils::debug_io::SerialLockGuard lock;

                debugln("");
                debugln("--- StackWatch ---");

                if (heap_free)
                {
                    debugfln("Heap: free=%u B  min=%u B",
                             static_cast<unsigned>(heap_free),
                             static_cast<unsigned>(heap_min));
                }

                detail::print_registered_tasks();

                debugln("--- /StackWatch ---");
                debugln("");
            }

            vTaskDelayUntil(&last, pdMS_TO_TICKS(period_ms));
        }
    }

} ///< namespace utils::stack_watch
