/**
 * MIT License
 *
 * @brief Configuration file for all ESP32 parameters.
 *
 * @file AppConfig.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-08-01
 * @copyright Copyright (c) 2025 Little Man Builds
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

/// @brief SnapshotBus integration macros
// MUST be defined before including SnapshotBus headers anywhere.

// Seqlock retry limit for SnapshotBus readers (controls spin loop work per read).
#ifndef SNAPSHOTBUS_SPIN_LIMIT
#define SNAPSHOTBUS_SPIN_LIMIT 64
#endif

// Optional yield hook used by SnapshotBus during spin loops.
// Safe to call from task context; no-op in ISR context.
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
