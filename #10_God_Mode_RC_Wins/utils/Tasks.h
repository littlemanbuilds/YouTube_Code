/**
 * MIT License
 *
 * @brief Creates FreeRTOS tasks from the reviewed settings in AppRtos.h.
 * @note One creation path keeps object pointers, stack units, priorities, CPU placement,
 *       and diagnostic registration consistent.
 *
 * @file Tasks.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-19
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <SnapshotRTOS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace utils::tasks
{
    /**
     * @brief Create and register a task using its shared cfg::task::Desc settings.
     *
     * @tparam T Object type passed to the task trampoline.
     * @param fn FreeRTOS task entry function (static trampoline).
     * @param spec Shared task settings: name, stack, priority, and CPU core.
     * @param handle_out Reference to a TaskHandle_t to receive the created task handle.
     * @param obj Non-owning pointer passed as pvParameters; must outlive the task.
     */
    template <typename T>
    inline void start_task(TaskFunction_t fn,
                           const cfg::task::Desc &spec,
                           TaskHandle_t &handle_out,
                           T *obj) noexcept
    {
        const BaseType_t result = xTaskCreatePinnedToCore(fn,
                                                          spec.name,
                                                          spec.stack,
                                                          static_cast<void *>(obj),
                                                          spec.priority,
                                                          &handle_out,
                                                          spec.core);

        configASSERT(result == pdPASS);
        if (result != pdPASS)
        {
            handle_out = nullptr;
            return; // Do not register a task that was never created.
        }

        snapshot::rtos::register_task(handle_out, spec.name, spec.stack, spec.priority, spec.core);
    }
} ///< namespace utils::tasks
