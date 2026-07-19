/**
 * MIT License
 *
 * @brief Starts SnapshotRTOS input tasks with consistent timing and task settings.
 * @note Centralizing this adapter keeps timestamps, heartbeat policy, stack units,
 *       and CPU placement consistent across every publisher.
 *
 * @file Publisher.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-19
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <SnapshotRTOS.h>
#include <SnapshotTools.h>
#include <Clock.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cstdint>

namespace utils::publisher
{
    namespace srt = snapshot::rtos; ///< Short alias for SnapshotRTOS helpers.

    /**
     * @brief Convert a minimum publish interval from milliseconds to a SnapshotRTOS policy.
     *
     * @param min_publish_ms Heartbeat minimum interval (milliseconds). 0 = disabled.
     * @return Policy with min_interval_us set, or zero when disabled.
     */
    [[nodiscard]] inline srt::PublishPolicy policy_from_ms(uint32_t min_publish_ms) noexcept
    {
        srt::PublishPolicy pol{}; ///< Zero-init.

        if (min_publish_ms == 0u)
        {
            return pol; // Disabled.
        }

        // Saturate the millisecond-to-microsecond conversion instead of wrapping
        // a very large interval into an unexpectedly short one.
        pol.min_interval_us = snapshot::time::ms_to_us_sat(min_publish_ms);
        return pol;
    }

    /**
     * @brief Start a SnapshotRTOS frame publisher with project-standard defaults.
     *
     * @tparam FrameT Snapshot frame type.
     * @tparam ReaderT Reader adapter type.
     * @tparam PublishT Publish decision type (AlwaysPublish, PublishOnChange, etc.).
     * @param bus Reference to the SnapshotBus to publish frames into.
     * @param reader Reader adapter used to read the next frame.
     * @param publish Publish decision policy (AlwaysPublish, PublishOnChange, etc.).
     * @param min_interval_ms Heartbeat minimum interval (milliseconds). 0 = disabled.
     * @param task_name FreeRTOS task name.
     * @param stack_words Task stack size in words (ESP32: 4 bytes each).
     * @param priority FreeRTOS task priority.
     * @param period_ms Polling period in milliseconds.
     * @param core_id Core affinity (tskNO_AFFINITY for unpinned).
     */
    template <typename FrameT, typename ReaderT, typename PublishT>
    inline void start(snapshot::SnapshotBus<FrameT> &bus,
                      ReaderT reader,
                      PublishT publish,
                      uint32_t min_interval_ms,
                      const char *task_name,
                      uint32_t stack_words,
                      UBaseType_t priority,
                      uint32_t period_ms,
                      BaseType_t core_id = tskNO_AFFINITY) noexcept
    {
        srt::start_frame_publisher<FrameT>(
            bus,
            reader,
            utils::clock::now_us,
            publish,
            policy_from_ms(min_interval_ms),
            task_name,
            stack_words,
            priority,
            period_ms,
            core_id);
    }

    /**
     * @brief Start a SnapshotRTOS frame publisher without a heartbeat interval.
     * @note This overload disables the heartbeat interval for AlwaysPublish-style publishers.
     *
     * @tparam FrameT Snapshot frame type.
     * @tparam ReaderT Reader adapter type.
     * @tparam PublishT Publish decision type (AlwaysPublish, PublishOnChange, etc.).
     * @param bus Reference to the SnapshotBus to publish frames into.
     * @param reader Reader adapter used to read the next frame.
     * @param publish Publish decision policy (AlwaysPublish, PublishOnChange, etc.).
     * @param task_name FreeRTOS task name.
     * @param stack_words Task stack size in words (ESP32: 4 bytes each).
     * @param priority FreeRTOS task priority.
     * @param period_ms Polling period in milliseconds.
     * @param core_id Core affinity (tskNO_AFFINITY for unpinned).
     */
    template <typename FrameT, typename ReaderT, typename PublishT>
    inline void start(snapshot::SnapshotBus<FrameT> &bus,
                      ReaderT reader,
                      PublishT publish,
                      const char *task_name,
                      uint32_t stack_words,
                      UBaseType_t priority,
                      uint32_t period_ms,
                      BaseType_t core_id = tskNO_AFFINITY) noexcept
    {
        start<FrameT>(bus, reader, publish, 0u, task_name, stack_words, priority, period_ms, core_id);
    }

    /**
     * @brief Start a SnapshotRTOS frame publisher with project-standard defaults (callback-based).
     * @note This overload uses C-style callbacks instead of a dedicated Reader type.
     *
     * @tparam FrameT Snapshot frame type.
     * @tparam PublishT Publish decision type (AlwaysPublish, PublishOnChange, etc.).
     * @param bus Reference to the SnapshotBus to publish frames into.
     * @param ctx Caller-owned context pointer passed unchanged to each callback.
     * @param update_fn Optional periodic update callback (may be nullptr).
     * @param read_fn Frame sampling callback (required).
     * @param ok_fn Optional health check callback (may be nullptr).
     * @param publish Publish decision policy (AlwaysPublish, PublishOnChange, etc.).
     * @param min_interval_ms Heartbeat minimum interval (milliseconds). 0 = disabled.
     * @param task_name FreeRTOS task name.
     * @param stack_words Task stack size in words (ESP32: 4 bytes each).
     * @param priority FreeRTOS task priority.
     * @param period_ms Polling period in milliseconds.
     * @param core_id Core affinity (tskNO_AFFINITY for unpinned).
     */
    template <typename FrameT, typename PublishT>
    inline void start_cb(snapshot::SnapshotBus<FrameT> &bus,
                         void *ctx,
                         void (*update_fn)(void *),
                         void (*read_fn)(void *, FrameT *),
                         bool (*ok_fn)(void *),
                         PublishT publish,
                         uint32_t min_interval_ms,
                         const char *task_name,
                         uint32_t stack_words,
                         UBaseType_t priority,
                         uint32_t period_ms,
                         BaseType_t core_id = tskNO_AFFINITY) noexcept
    {
        srt::start_frame_publisher_cb<FrameT>(
            bus,
            ctx,
            update_fn,
            read_fn,
            ok_fn,
            utils::clock::now_us,
            publish,
            policy_from_ms(min_interval_ms),
            task_name,
            static_cast<uint16_t>(stack_words),
            priority,
            period_ms,
            core_id);
    }

    /**
     * @brief Start a SnapshotRTOS frame publisher without a heartbeat interval (callback-based).
     * @note This overload disables the heartbeat interval for AlwaysPublish-style publishers.
     *
     * @tparam FrameT Snapshot frame type.
     * @tparam PublishT Publish decision type (AlwaysPublish, PublishOnChange, etc.).
     *
     * @param bus Reference to the SnapshotBus to publish frames into.
     * @param ctx Caller-owned context pointer passed unchanged to each callback.
     * @param update_fn Optional periodic update callback (may be nullptr).
     * @param read_fn Frame sampling callback (required).
     * @param ok_fn Optional health check callback (may be nullptr).
     * @param publish Publish decision policy (AlwaysPublish, PublishOnChange, etc.).
     * @param task_name FreeRTOS task name.
     * @param stack_words Task stack size in words (ESP32: 4 bytes each).
     * @param priority FreeRTOS task priority.
     * @param period_ms Polling period in milliseconds.
     * @param core_id Core affinity (tskNO_AFFINITY for unpinned).
     */
    template <typename FrameT, typename PublishT>
    inline void start_cb(snapshot::SnapshotBus<FrameT> &bus,
                         void *ctx,
                         void (*update_fn)(void *),
                         void (*read_fn)(void *, FrameT *),
                         bool (*ok_fn)(void *),
                         PublishT publish,
                         const char *task_name,
                         uint32_t stack_words,
                         UBaseType_t priority,
                         uint32_t period_ms,
                         BaseType_t core_id = tskNO_AFFINITY) noexcept
    {
        start_cb<FrameT>(bus,
                         ctx,
                         update_fn,
                         read_fn,
                         ok_fn,
                         publish,
                         0u,
                         task_name,
                         stack_words,
                         priority,
                         period_ms,
                         core_id);
    }

} ///< namespace utils::publisher
