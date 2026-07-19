/**
 * MIT License
 *
 * @brief Task utilities for creating pinned FreeRTOS tasks from cfg::task::Desc descriptors.
 * @note Centralizes xTaskCreatePinnedToCore() boilerplate so task creation is consistent
 *       and less error-prone (e.g., avoids passing the wrong object pointer to a task).
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
     * @brief Create a pinned FreeRTOS task using a canonical cfg::task::Desc.
     *
     * @tparam T Object type passed to the task trampoline.
     * @param fn FreeRTOS task entry function (static trampoline).
     * @param spec Canonical task descriptor (name/stack/priority/core).
     * @param handle_out Reference to a TaskHandle_t to receive the created task handle.
     * @param obj Non-owning pointer passed as pvParameters; must outlive the task.
     */
    template <typename T>
    inline void start_task(TaskFunction_t fn,
                           const cfg::task::Desc &spec,
                           TaskHandle_t &handle_out,
                           T *obj) noexcept
    {
        configASSERT(xTaskCreatePinnedToCore(fn,
                                             spec.name,
                                             spec.stack,
                                             static_cast<void *>(obj),
                                             spec.priority,
                                             &handle_out,
                                             spec.core) == pdPASS);

        snapshot::rtos::register_task(handle_out, spec.name, spec.stack, spec.priority, spec.core);
    }
} ///< namespace utils::tasks
