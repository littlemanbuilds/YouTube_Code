/**
 * MIT License
 *
 * @brief Central manager for configuration switches (DIP + gear selector).
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
 * @brief Reads DIP + gear switches and publishes ConfigSnapshot frames.
 * @note DIP is treated as a boot-time configuration and is latched once per session.
 *       Gear is treated as a continuous input and is updated whenever MCP samples are healthy.
 */
class ConfigPublisher
{
public:
    /**
     * @brief Construct the ConfigPublisher task.
     *
     * @param mcp Reference to the McpBus providing MCP23017 samples (DIP + gear are MCP-backed).
     * @param out Reference to the ConfigBus to publish into.
     * @param period_ms Publish interval (milliseconds).
     * @param min_publish_ms Heartbeat minimum interval (milliseconds). 0 = disabled.
     */
    explicit ConfigPublisher(McpBus &mcp,
                             ConfigBus &out,
                             uint32_t period_ms = cfg::tick::LOOP_MS,
                             uint32_t min_publish_ms = 0) noexcept;

    /// @brief Initialize configuration state and start the ConfigPublisher task.
    void begin() noexcept;

private:
    // -- SwitchBank reader callback (MCP23017 virtual pins) ---- //

    /**
     * @brief SwitchBank reader callback for MCP23017 virtual pins.
     *
     * @param ctx Pointer to the owning ConfigPublisher instance.
     * @param key MCP23017 virtual pin index (0..15).
     * @return true if the MCP pin is electrically HIGH.
     */
    static bool readMcp(void *ctx, uint8_t key) noexcept;

    // ---- Decoders ---- //

    /**
     * @brief Decode 3-bit DIP value into CarMode (direct mapping 0..7).
     *
     * @param dip_value 3-bit DIP value (bits 0..2).
     * @return CarMode Mapped mode value.
     */
    static CarMode decodeMode(uint8_t dip_value) noexcept;

    /**
     * @brief Decode physical shifter contact bits into a logical Gear value.
     *
     * @param gear_bits Physical contact bits: bit0=R, bit1=FWD, bit2=D1, bit3=D3.
     * @return Gear Decoded logical gear, or Unknown for impossible contact combinations.
     */
    static Gear decodeGear(uint8_t gear_bits) noexcept;

    // ---- Internal phases ---- //

    /**
     * @brief Allow debouncers to settle, then latch DIP once for the session.
     * @note DIP is treated as a boot configuration; after latching, the value remains fixed.
     */
    void settleAndLatchDip_() noexcept;

    /**
     * @brief Refresh cached MCP sample and advance debounce engines.
     * @note This is the single place in this class where MCP snapshots are refreshed.
     */
    void updateBanks_() noexcept;

    /**
     * @brief Build a ConfigSnapshot from latched DIP + current debounced shifter contacts.
     *
     * @param s Snapshot frame to populate.
     */
    void fillSnapshot_(ConfigSnapshot &s) noexcept;

    /**
     * @brief Fatal safe loop: publish SAFE config while waiting for MCP recovery, then reboot.
     * @note Reboot is acceptable after hardware recovery.
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
     * @return true if the input source is healthy; false otherwise.
     */
    static bool cb_ok(void *ctx) noexcept
    {
        return static_cast<ConfigPublisher *>(ctx)->mcp_view_.ok();
    }

    // ---- Internal State ---- //
    McpBus &mcp_;                            ///< Reference to the McpBus providing MCP23017 samples.
    ConfigBus &bus_;                         ///< Reference to the ConfigBus to publish into.
    uint32_t period_ms_{cfg::tick::LOOP_MS}; ///< Evaluation period (ms).
    uint32_t min_publish_ms_{0};             ///< Minimum publish interval (ms). 0 = disabled.

    hal::McpView mcp_view_;   ///< Cached view of MCP23017 snapshot bus.
    bool mcp_healthy_{false}; ///< True once MCP samples are valid (guards debouncer updates).

    uint8_t dip_latched_{0}; ///< Latched DIP value (bits 0..2).
    bool dip_locked_{false}; ///< True once DIP has been latched (boot config locked).

    // ---- Debounced switch banks ---- //
    using DipBank = SwitchBank<cfg::switches::DIP_COUNT>;
    DipBank dip_{cfg::switches::DIP_KEYS,       ///< MCP23017 GPIOA pins for the DIP switch.
                 cfg::switches::DEBOUNCE_MS,    ///< Shared debounce (ms).
                 cfg::switches::DIP_VALUE_MASK, ///< Active-low mask.
                 &ConfigPublisher::readMcp,     ///< Reader callback (uses cached MCP samples).
                 this,                          ///< Reader context.
                 &utils::clock::now_ms};        ///< Time source (ms).

    // The gear bank tracks the real shifter contacts only.
    // Park and D2 are not physical contacts (decoded later from gear_raw).
    using GearBank = SwitchBank<cfg::switches::GEAR_COUNT>;
    GearBank gear_{cfg::switches::GEAR_KEYS,
                   cfg::switches::DEBOUNCE_MS,
                   cfg::switches::GEAR_VALUE_MASK,
                   &ConfigPublisher::readMcp,
                   this,
                   &utils::clock::now_ms};
};
