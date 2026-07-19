/**
 * MIT License
 *
 * @brief MCP publisher: MCP23017 → SnapshotBus (McpBus).
 *
 * @file McpPublisher.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2026-01-20
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#pragma once

#include <Adafruit_MCP23X17.h>
#include <AppConfig.h>
#include <Clock.h>
#include <McpBus.h>

#include <cstdint>

/**
 * @brief Publishes cached MCP23017 GPIO samples onto McpBus.
 */
class McpPublisher
{
public:
    /**
     * @brief Construct a McpPublisher task.
     *
     * @param bus Reference to the McpBus to publish into.
     * @param period_ms Publish interval (milliseconds).
     */
    explicit McpPublisher(McpBus &bus, uint32_t period_ms = cfg::tick::LOOP_MS) noexcept;

    /// @brief Initialize MCP23017 and start SnapshotRTOS publisher task.
    void begin() noexcept;

private:
    // ---- MCP init + sampling ---- //

    /// @brief Initialize MCP23017 and configure required pins as INPUT_PULLUP.
    void initMcp_() noexcept;

    /// @brief Sample GPIOA and GPIOB into cached members (one I2C read per port).
    void sample_() noexcept;

    /**
     * @brief Lightweight I2C presence probe (ACK check).
     * @note This is a cheap way to detect brownouts/disconnects.
     *
     * @return true if the MCP address ACKs on the bus.
     */
    [[nodiscard]] bool probeAck_() noexcept;

    // ---- SnapshotRTOS callback adapter ---- //

    /**
     * @brief SnapshotRTOS update callback (advance one sample step).
     *
     * @param ctx Pointer to the McpPublisher instance.
     */
    static void cb_update(void *ctx) noexcept
    {
        static_cast<McpPublisher *>(ctx)->sample_();
    }

    /**
     * @brief SnapshotRTOS health callback.
     *
     * @param ctx Pointer to the McpPublisher instance.
     * @return true if the input source is healthy; false otherwise.
     */
    static void cb_read(void *ctx, McpState *s) noexcept
    {
        auto &self = *static_cast<McpPublisher *>(ctx); ///< Recover McpPublisher instance from opaque context pointer.

        s->gpio = static_cast<uint16_t>((static_cast<uint16_t>(self.gpiob_cached_) << 8u) | self.gpioa_cached_);
        s->gpioa = self.gpioa_cached_;
        s->gpiob = self.gpiob_cached_;
        s->ok = self.mcp_healthy_;
    }

    /**
     * @brief SnapshotRTOS health callback.
     *
     * @param ctx Pointer to the McpPublisher instance.
     * @return true if the MCP is healthy.
     *
     * @return true if MCP sampling is currently healthy.
     */
    static bool cb_ok(void *ctx) noexcept
    {
        return static_cast<McpPublisher *>(ctx)->mcp_healthy_;
    }

    // ---- Internal state ---- //
    McpBus &bus_;           ///< Reference to the McpBus to publish into.
    uint32_t period_ms_{0}; ///< Publish interval (ms).

    Adafruit_MCP23X17 mcp_{}; ///< MCP23017 driver instance.
    bool mcp_healthy_{false}; ///< Current MCP availability state.
    uint32_t last_try_ms_{0}; ///< Last MCP init retry time (ms).

    static constexpr uint32_t kProbeIntervalMs = 500u; ///< ACK probe interval (ms).
    static constexpr uint8_t kMaxProbeFails = 3u;      ///< Failed ACK probes before marking unhealthy.
    uint32_t last_probe_ms_{0};                        ///< Last ACK probe time (ms).
    uint8_t probe_fail_{0};                            ///< Consecutive failed ACK probe count.

    uint8_t gpioa_cached_{0xFFu}; ///< Cached GPIOA sample (HIGH safe default).
    uint8_t gpiob_cached_{0xFFu}; ///< Cached GPIOB sample (HIGH safe default).
};