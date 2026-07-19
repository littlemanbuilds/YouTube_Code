/**
 * MIT License
 *
 * @brief Reads MCP23017-backed buttons, debounces them, and publishes their state.
 *
 * @file ButtonPublisher.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-09-12
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <ButtonBus.h>
#include <Clock.h>
#include <McpBus.h>
#include <McpView.h>
#include <Universal_Button.h>

#include <cstdint>

/**
 * @brief Converts raw active-low button inputs into stable ButtonBus snapshots.
 */
class ButtonPublisher
{
public:
    /**
     * @brief Create a publisher bound to the MCP input and button output buses.
     *
     * @param mcp Reference to the McpBus providing MCP23017 samples (buttons are MCP-backed).
     * @param bus Reference to the ButtonBus to publish into.
     * @param period_ms Publish interval (milliseconds).
     */
    ButtonPublisher(McpBus &mcp,
                    ButtonBus &bus,
                    uint32_t period_ms = cfg::tick::LOOP_MS) noexcept;

    /// @brief Start the SnapshotRTOS publisher task.
    void begin() noexcept;

private:
    // ---- Adapter between Universal_Button and cached MCP23017 input ---- //

    /**
     * @brief Read one active-low button from the cached MCP23017 sample.
     *
     * @param ctx Pointer to the owning ButtonPublisher instance.
     * @param id MCP23017 virtual pin index (0..15).
     * @return True when the active-low input is asserted; otherwise false.
     */
    static bool readMcp(void *ctx, uint8_t id) noexcept;

    /**
     * @brief Refresh the MCP23017 cache and reject invalid input data.
     *
     * @return True when MCP samples are valid; otherwise false.
     */
    [[nodiscard]] bool refreshCache_() noexcept;

    // ---- SnapshotRTOS callback adapter ---- //

    /**
     * @brief SnapshotRTOS update callback (refresh MCP cache + run debounce).
     *
     * @param ctx Pointer to the ButtonPublisher instance.
     */
    static void cb_update(void *ctx) noexcept
    {
        // Recover the publisher object supplied when the callback task was started.
        auto &self = *static_cast<ButtonPublisher *>(ctx);

        if (!self.refreshCache_())
        {
            return; // Invalid hardware data must not create a false button edge.
        }

        self.buttons_.update();
    }

    /**
     * @brief SnapshotRTOS read callback (copy debounced button state into snapshot).
     *
     * @param ctx Pointer to the ButtonPublisher instance.
     * @param s Snapshot output structure filled with the latest button state.
     */
    static void cb_read(void *ctx, ButtonState *out) noexcept
    {
        // Recover the publisher object supplied when the callback task was started.
        auto &self = *static_cast<ButtonPublisher *>(ctx);
        self.buttons_.snapshot(out->buttons);
    }

    /**
     * @brief SnapshotRTOS health callback.
     *
     * @param ctx Pointer to the ButtonPublisher instance.
     * @return True when the input source is healthy; otherwise false.
     */
    static bool cb_ok(void *ctx) noexcept
    {
        // Recover the publisher object supplied when the callback task was started.
        auto &self = *static_cast<ButtonPublisher *>(ctx);
        return self.mcp_view_.ok();
    }

private:
    // ---- Internal state ---- //
    ButtonBus &bus_;        ///< Reference to the ButtonBus to publish into.
    Button buttons_;        ///< Owns debounce state; updated only by ButtonPublisher.
    uint32_t period_ms_{0}; ///< Publish interval (ms).

    hal::McpView mcp_view_; ///< Cached view of MCP23017 snapshot bus.
};
