/**
 * MIT License
 *
 * @brief Central manager for button input.
 *
 * @file ButtonPublisher.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-09-12
 * @copyright Copyright (c) 2025 Little Man Builds
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
 * @brief Publishes debounced button snapshots onto ButtonBus.
 */
class ButtonPublisher
{
public:
    /**
     * @brief Construct a ButtonPublisher task.
     *
     * @param mcp Reference to the McpBus providing MCP23017 samples (buttons are MCP-backed).
     * @param bus Reference to the ButtonBus to publish into.
     * @param period_ms Publish interval (milliseconds).
     * @param min_publish_ms Heartbeat minimum interval (milliseconds). 0 = disabled.
     */
    ButtonPublisher(McpBus &mcp,
                    ButtonBus &bus,
                    uint32_t period_ms = cfg::tick::LOOP_MS,
                    uint32_t min_publish_ms = 0) noexcept;

    /// @brief Start the SnapshotRTOS publisher task.
    void begin() noexcept;

private:
    // ---- Universal_Button reader adapter (MCP23017 cached) ---- //

    /**
     * @brief Universal_Button reader callback using cached MCP23017 samples.
     *
     * @param ctx Pointer to the owning ButtonPublisher instance.
     * @param id MCP23017 virtual pin index (0..15).
     * @return true if the input is asserted under active-low wiring (electrical LOW).
     */
    static bool readMcp(void *ctx, uint8_t id) noexcept;

    /**
     * @brief Refresh cached MCP23017 samples and return health status.
     *
     * @return true if MCP samples are valid; false if MCP is unhealthy/offline.
     */
    [[nodiscard]] bool refreshCache_() noexcept;

    /**
     * @brief Change detector for ButtonState (ignores timestamps).
     */
    struct ButtonChanged
    {
        /**
         * @brief Compare two button snapshots.
         *
         * @param prev Previous button snapshot.
         * @param next New button snapshot.
         * @return true if button values changed; false otherwise.
         */
        [[nodiscard]] bool operator()(const ButtonState &prev, const ButtonState &next) const noexcept
        {
            return prev.buttons != next.buttons; ///< Ignore stamp.
        }
    };

    // ---- SnapshotRTOS callback adapter ---- //

    /**
     * @brief SnapshotRTOS update callback (refresh MCP cache + run debounce).
     *
     * @param ctx Pointer to the ButtonPublisher instance.
     */
    static void cb_update(void *ctx) noexcept
    {
        auto &self = *static_cast<ButtonPublisher *>(ctx); ///< Recover ButtonPublisher instance from opaque context pointer.

        if (!self.refreshCache_())
        {
            return; ///< MCP unhealthy: hold current debounced state.
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
        auto &self = *static_cast<ButtonPublisher *>(ctx); ///< Recover ButtonPublisher instance from opaque context pointer.
        self.buttons_.snapshot(out->buttons);
    }

    /**
     * @brief SnapshotRTOS health callback.
     *
     * @param ctx Pointer to the ButtonPublisher instance.
     * @return true if the input source is healthy; false otherwise.
     */
    static bool cb_ok(void *ctx) noexcept
    {
        auto &self = *static_cast<ButtonPublisher *>(ctx); ///< Recover ButtonPublisher instance from opaque context pointer.
        return self.mcp_view_.ok();
    }

private:
    // ---- Internal state ---- //
    McpBus &mcp_;                ///< Reference to the McpBus providing MCP23017 samples (buttons are MCP-backed).
    ButtonBus &bus_;             ///< Reference to the ButtonBus to publish into.
    Button buttons_;             ///< Owns debounce state; updated only by ButtonPublisher.
    uint32_t period_ms_{0};      ///< Publish interval (ms).
    uint32_t min_publish_ms_{0}; ///< Minimum publish interval (ms). 0 = disabled.

    hal::McpView mcp_view_; ///< Cached view of MCP23017 snapshot bus.
};