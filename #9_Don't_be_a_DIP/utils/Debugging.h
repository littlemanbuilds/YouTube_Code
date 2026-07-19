/**
 * MIT License
 *
 * @brief Serial-based debugging output helpers.
 * @note Provides lightweight wrappers around Arduino Serial printing that
 *       compile to no-ops when debugging is disabled.
 *
 * @file Debugging.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-19
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <Arduino.h>
#include <AppConfig.h>

// These are intentionally macros, so undefine them first in case another
// header or build flag has already defined them.
#ifdef debugf
#undef debugf
#endif

#ifdef debugfln
#undef debugfln
#endif

#if DEBUGGING

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace utils::debug_io
{
    /**
     * @brief Shared recursive mutex for Serial debug output.
     * @note Recursive locking allows a caller to lock a whole debug block while
     *       still using debug(), debugln(), debugf(), and debugfln() inside it.
     *
     * @warning Do not rename this namespace to utils::debug. That can conflict
     *          with unqualified debug(...) calls inside other utils::* namespaces.
     */
    [[nodiscard]] inline SemaphoreHandle_t serialMutex() noexcept
    {
        static SemaphoreHandle_t mutex = xSemaphoreCreateRecursiveMutex();
        return mutex;
    }

    /**
     * @brief Lock the shared Serial debug mutex.
     */
    inline void lockSerial() noexcept
    {
        SemaphoreHandle_t mutex = serialMutex();

        if (mutex)
        {
            xSemaphoreTakeRecursive(mutex, portMAX_DELAY);
        }
    }

    /**
     * @brief Unlock the shared Serial debug mutex.
     */
    inline void unlockSerial() noexcept
    {
        SemaphoreHandle_t mutex = serialMutex();

        if (mutex)
        {
            xSemaphoreGiveRecursive(mutex);
        }
    }

    /**
     * @brief RAII guard for Serial debug output.
     * @note Use this when printing a multi-line debug block that must not be
     *       interleaved with output from another task.
     */
    class SerialLockGuard
    {
    public:
        SerialLockGuard() noexcept { lockSerial(); }
        ~SerialLockGuard() noexcept { unlockSerial(); }

        SerialLockGuard(const SerialLockGuard &) = delete;
        SerialLockGuard &operator=(const SerialLockGuard &) = delete;
    };

} ///< namespace utils::debug_io

/**
 * @brief Print a value to Serial without a newline.
 *
 * @tparam T Printable value type.
 * @param x Value to print.
 */
template <typename T>
inline void debug(const T &x) noexcept
{
    utils::debug_io::SerialLockGuard lock;
    Serial.print(x);
}

/**
 * @brief Print a value to Serial followed by a newline.
 *
 * @tparam T Printable value type.
 * @param x Value to print.
 */
template <typename T>
inline void debugln(const T &x) noexcept
{
    utils::debug_io::SerialLockGuard lock;
    Serial.println(x);
}

/**
 * @brief Print an empty newline.
 */
inline void debugln() noexcept
{
    utils::debug_io::SerialLockGuard lock;
    Serial.println();
}

/**
 * @brief Print a float with explicit precision.
 *
 * @param x Float value to print.
 * @param digits Number of digits after the decimal point.
 */
inline void debug(float x, int digits) noexcept
{
    utils::debug_io::SerialLockGuard lock;
    Serial.print(x, digits);
}

/**
 * @brief Print a float with explicit precision followed by newline.
 *
 * @param x Float value to print.
 * @param digits Number of digits after the decimal point.
 */
inline void debugln(float x, int digits) noexcept
{
    utils::debug_io::SerialLockGuard lock;
    Serial.println(x, digits);
}

/**
 * @brief printf-style formatted debug output without newline.
 * @note This macro locks the shared Serial mutex for the duration of one
 *       formatted print call.
 */
#define debugf(...)                                    \
    do                                                 \
    {                                                  \
        utils::debug_io::SerialLockGuard debug_lock__; \
        Serial.printf(__VA_ARGS__);                    \
    } while (0)

/**
 * @brief printf-style formatted debug output with newline.
 * @note This macro locks the shared Serial mutex for the duration of one
 *       formatted print call.
 */
#define debugfln(fmt, ...)                             \
    do                                                 \
    {                                                  \
        utils::debug_io::SerialLockGuard debug_lock__; \
        Serial.printf(fmt "\n", ##__VA_ARGS__);        \
    } while (0)

#else // DEBUGGING disabled.

namespace utils::debug_io
{
    /**
     * @brief No-op serial lock guard for non-debug builds.
     */
    class SerialLockGuard
    {
    public:
        SerialLockGuard() noexcept = default;
        ~SerialLockGuard() noexcept = default;

        SerialLockGuard(const SerialLockGuard &) = delete;
        SerialLockGuard &operator=(const SerialLockGuard &) = delete;
    };

    /**
     * @brief No-op lock for non-debug builds.
     */
    inline void lockSerial() noexcept {}

    /**
     * @brief No-op unlock for non-debug builds.
     */
    inline void unlockSerial() noexcept {}

} ///< namespace utils::debug_io

/**
 * @brief No-op debug print for non-debug builds.
 *
 * @tparam T Printable value type.
 * @param Unused.
 */
template <typename T>
inline void debug(const T &) noexcept
{
}

/**
 * @brief No-op debug print line for non-debug builds.
 *
 * @tparam T Printable value type.
 * @param Unused.
 */
template <typename T>
inline void debugln(const T &) noexcept
{
}

/**
 * @brief No-op empty debug print line for non-debug builds.
 */
inline void debugln() noexcept
{
}

/**
 * @brief No-op float debug print for non-debug builds.
 */
inline void debug(float, int) noexcept
{
}

/**
 * @brief No-op float debug print line for non-debug builds.
 */
inline void debugln(float, int) noexcept
{
}

#define debugf(...)
#define debugfln(...)

#endif