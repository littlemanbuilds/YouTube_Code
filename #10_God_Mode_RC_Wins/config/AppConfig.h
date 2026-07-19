/**
 * MIT License
 *
 * @brief Top-level project configuration and SnapshotBus integration settings.
 *
 * @file AppConfig.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-08-01
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <Arduino.h>
#include <AppIO.h>
#include <AppPolicy.h>
#include <AppRtos.h>

// Enable serial debugging (used by utils/Debugging.h).
// Override in build flags (e.g. -DDEBUGGING=0) if needed.
#ifndef DEBUGGING
#define DEBUGGING true
#endif

// SnapshotBus reads can briefly collide with a publish. These integration macros
// must be defined before any SnapshotBus header so every bus uses the same limits.

// Bound the work spent retrying a read while another task is publishing.
#ifndef SNAPSHOTBUS_SPIN_LIMIT
#define SNAPSHOTBUS_SPIN_LIMIT 64
#endif

// Let another task run between retries, but never yield from interrupt context.
#ifndef SNAPSHOTBUS_YIELD
static inline void snapshotbus_maybe_yield()
{
    if (!xPortInIsrContext())
    {
        taskYIELD();
    }
}
#define SNAPSHOTBUS_YIELD() snapshotbus_maybe_yield()
#endif
