/**
 * MIT License
 *
 * @brief Turns the physical DIP switches and shifter contacts into configuration snapshots.
 *
 * @file ConfigPublisher.h
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2026-01-02
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <AppConfig.h>
#include <Clock.h>
#include <ConfigBus.h>
#include <McpBus.h>
#include <McpView.h>
#include <SwitchBank.h>

#include <cstdint>

/**
 * @brief Publishes the selected operating mode and current logical gear.
 * @note The DIP mode is locked at boot so a loose switch cannot change control mode
 *       while driving. Gear remains live and updates whenever MCP23017 data is healthy.
 */
class ConfigPublisher
{
public:
    /**
     * @brief Create a publisher bound to the MCP input and configuration output buses.
     *
     * @param mcp Reference to the McpBus providing MCP-backed DIP and gear samples.
     * @param out Reference to the ConfigBus to publish into.
     * @param period_ms Publish interval (milliseconds).
     */
    explicit ConfigPublisher(McpBus &mcp,
                             ConfigBus &out,
                             uint32_t period_ms = cfg::tick::LOOP_MS) noexcept;

    /// @brief Initialize configuration state and start the ConfigPublisher task.
    void begin() noexcept;

private:
    // ---- Adapter between SwitchBank and cached MCP23017 input ---- //

    /**
     * @brief Read one electrical pin level from the cached MCP23017 sample.
     *
     * @param ctx Pointer to the owning ConfigPublisher instance.
     * @param key MCP23017 virtual pin index (0..15).
     * @return True when the MCP pin is electrically HIGH; otherwise false.
     */
    static bool readMcp(void *ctx, uint8_t key) noexcept;

    // ---- Decoders ---- //

    /**
     * @brief Decode 3-bit DIP value into CarMode (direct mapping 0..7).
     *
     * @param dip_value 3-bit DIP value (bits 0..2).
     * @return Mapped mode value.
     */
    static CarMode decodeMode(uint8_t dip_value) noexcept;

    /**
     * @brief Decode physical shifter contact bits into a logical Gear value.
     *
     * @param gear_bits Physical contact bits: bit0=R, bit1=FWD, bit2=D1, bit3=D3.
     * @return Decoded logical gear, or Unknown for impossible contact combinations.
     */
    static Gear decodeGear(uint8_t gear_bits) noexcept;

    // ---- Internal phases ---- //

    /**
     * @brief Let the switches settle, then lock the selected mode for this boot.
     */
    void settleAndLatchDip_() noexcept;

    /**
     * @brief Refresh the MCP23017 sample and update both switch debouncers.
     * @note Keeping the refresh here gives the DIP and shifter the same hardware sample.
     */
    void updateBanks_() noexcept;

    /**
     * @brief Combine the locked mode and live shifter contacts into one snapshot.
     *
     * @param s Snapshot frame to populate.
     */
    void fillSnapshot_(ConfigSnapshot &s) noexcept;

    /**
     * @brief Publish a safe configuration until MCP23017 communication recovers, then reboot.
     * @note Rebooting reruns the normal switch-settling and mode-lock sequence.
     */
    [[noreturn]] void fatalWaitForMcpAndReboot_() noexcept;

    // ---- SnapshotRTOS callback adapter ---- //

    /**
     * @brief SnapshotRTOS update callback (advance one bank update step).
     *
     * @param ctx Pointer to the ConfigPublisher instance.
     */
    static void cb_update(void *ctx) noexcept
    {
        static_cast<ConfigPublisher *>(ctx)->updateBanks_();
    }

    /**
     * @brief SnapshotRTOS read callback (fill ConfigSnapshot from latest bank state).
     *
     * @param ctx Pointer to the ConfigPublisher instance.
     * @param s Snapshot output structure filled with the latest bank state.
     */
    static void cb_read(void *ctx, ConfigSnapshot *s) noexcept
    {
        static_cast<ConfigPublisher *>(ctx)->fillSnapshot_(*s);
    }

    /**
     * @brief SnapshotRTOS health callback.
     *
     * @param ctx Pointer to the ConfigPublisher instance.
     * @return True when the input source is healthy; otherwise false.
     */
    static bool cb_ok(void *ctx) noexcept
    {
        return static_cast<ConfigPublisher *>(ctx)->mcp_view_.ok();
    }

    // ---- Internal state ---- //
    ConfigBus &bus_;                         ///< Reference to the ConfigBus to publish into.
    uint32_t period_ms_{cfg::tick::LOOP_MS}; ///< Evaluation period (ms).

    hal::McpView mcp_view_;   ///< Cached view of MCP23017 snapshot bus.
    bool mcp_healthy_{false}; ///< True while current MCP23017 input is valid.

    uint8_t dip_latched_{0}; ///< Latched DIP value (bits 0..2).
    bool dip_locked_{false}; ///< True once DIP has been latched (boot config locked).

    // ---- Debounced switch banks ---- //
    using DipBank = SwitchBank<cfg::switches::DIP_COUNT>;
    DipBank dip_{cfg::switches::DIP_KEYS,       // MCP23017 GPIOA pins for the DIP switch.
                 cfg::switches::DEBOUNCE_MS,    // Shared debounce (ms).
                 cfg::switches::DIP_VALUE_MASK, // Active-low mask.
                 &ConfigPublisher::readMcp,     // Reader callback (uses cached MCP samples).
                 this,                          // Reader context.
                 &utils::clock::now_ms};        // Time source (ms).

    // The shifter has no separate Park or D2 wires; those logical positions are
    // decoded later from combinations of the four physical contacts.
    using GearBank = SwitchBank<cfg::switches::GEAR_COUNT>;
    GearBank gear_{cfg::switches::GEAR_KEYS,
                   cfg::switches::DEBOUNCE_MS,
                   cfg::switches::GEAR_VALUE_MASK,
                   &ConfigPublisher::readMcp,
                   this,
                   &utils::clock::now_ms};
};
