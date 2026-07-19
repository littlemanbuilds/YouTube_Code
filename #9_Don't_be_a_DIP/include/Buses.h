/**
 * MIT License
 *
 * @brief Central access points for shared SnapshotBus instances.
 *
 * @file Buses.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-19
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <AuthorityBus.h>
#include <ButtonBus.h>
#include <ConfigBus.h>
#include <ControlBus.h>
#include <McpBus.h>
#include <RcBus.h>

/**
 * @brief Namespace providing accessors for shared SnapshotBus instances.
 */
namespace buses
{
    // ---- Forward declarations ---- //
    [[nodiscard]] inline AuthorityBus &authority() noexcept;
    [[nodiscard]] inline ButtonBus &buttons() noexcept;
    [[nodiscard]] inline ConfigBus &config() noexcept;
    [[nodiscard]] inline ControlBus &control() noexcept;
    [[nodiscard]] inline McpBus &mcp() noexcept;
    [[nodiscard]] inline RcBus &rc() noexcept;

    /**
     * @brief Force construction of all shared bus instances.
     * @note Call this once in setup() before any tasks start so all function-local
     *       static buses are constructed in a single-threaded context.
     */

    inline void init() noexcept
    {
        (void)authority();
        (void)buttons();
        (void)config();
        (void)control();
        (void)mcp();
        (void)rc();
    }

    // ---- Snapshot buses ---- //

    /// @brief Return reference to the shared AuthorityBus instance.
    [[nodiscard]] inline AuthorityBus &authority() noexcept
    {
        static AuthorityBus bus{}; ///< Shared AuthorityBus instance (function-local static).
        return bus;
    }

    /// @brief Return reference to the shared ButtonBus instance.
    [[nodiscard]] inline ButtonBus &buttons() noexcept
    {
        static ButtonBus bus{}; ///< Shared ButtonBus instance (function-local static).
        return bus;
    }

    /// @brief Return reference to the shared ConfigBus instance.
    [[nodiscard]] inline ConfigBus &config() noexcept
    {
        static ConfigBus bus{}; ///< Shared ConfigBus instance (function-local static).
        return bus;
    }

    /// @brief Return reference to the shared ControlBus instance.
    [[nodiscard]] inline ControlBus &control() noexcept
    {
        static ControlBus bus{}; ///< Shared ControlBus instance (function-local static).
        return bus;
    }

    /// @brief Return reference to the shared McpBus instance.
    [[nodiscard]] inline McpBus &mcp() noexcept
    {
        static McpBus bus{}; ///< Shared McpBus instance (function-local static).
        return bus;
    }

    /// @brief Return reference to the shared RcBus instance.
    [[nodiscard]] inline RcBus &rc() noexcept
    {
        static RcBus bus{}; ///< Shared RcBus instance (function-local static).
        return bus;
    }
} ///< namespace buses
