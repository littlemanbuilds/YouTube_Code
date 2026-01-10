/**
 * MIT License
 *
 * @brief Shared configuration and backends for LEDC/MCPWM sweep demo.
 *
 * @file MCPWM_Test_Sweep.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-09-05
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#pragma once

#include <Arduino.h>
#include <cstdint>
#include <cmath>
#include <driver/mcpwm.h>
#include <ESP32_MCPWM.h>

// ---------------- Pins (IBT-2 / BTS7960) ----------------
constexpr int RPWM_PIN = 37; ///< IBT-2 RPWM pin.
constexpr int LPWM_PIN = 38; ///< IBT-2 LPWM pin.
constexpr int EN_PIN = 39;   ///< IBT-2 EN pin.

// ---------------- Common PWM config ----------------
constexpr uint32_t PWM_FREQ_HZ = 20000; ///< PWM frequency (Hz), quiet.
constexpr int LEDC_BITS = 10;           ///< LEDC resolution bits (0..1023).
constexpr int LEDC_CH_R = 0;            ///< LEDC channel used for RPWM only.

// ---------------- Sweep config (unchanged behavior) ----------------
constexpr float SWEEP_START_PCT = 11.0f;   ///< Start duty for sweep (%).
constexpr float SWEEP_STOP_PCT = 22.0f;    ///< Stop duty for sweep (%).
constexpr float SWEEP_STEP_PCT = 1.0f;     ///< Step size for sweep (%).
constexpr float SWEEP_HOLD_PCT = 11.0f;    ///< Low value to hold at end (%).
constexpr uint32_t SWEEP_HOLD_MS = 1500;   ///< Hold time at each step (ms).
constexpr float MIN_EFFECTIVE_LEDC = 0.0f; ///< Minimum LEDC duty to overcome stiction (%).

// ---------------- Tiny helpers ----------------

/**
 * @brief Clamp a duty percentage to [0, 100].
 *
 * @param p Percentage value.
 * @return float Clamped percentage within [0, 100].
 */
static inline float clampPct(float p)
{
    return (p < 0.0f) ? 0.0f : ((p > 100.0f) ? 100.0f : p);
}

/**
 * @brief Convert a percentage (0..100) to LEDC duty for the configured resolution.
 *
 * @param pct Duty percentage.
 * @return uint32_t Duty value in range [0, (1<<LEDC_BITS)-1].
 */
static inline uint32_t pctToDuty10(float pct)
{
    pct = clampPct(pct);
    const uint32_t maxDuty = (1u << LEDC_BITS) - 1u;
    return static_cast<uint32_t>(std::lroundf((pct * static_cast<float>(maxDuty)) / 100.0f));
}

// ===================================================================
//                       Backend interface
// ===================================================================

/**
 * @brief Simple backend interface used by the sweep runner.
 *
 * Each backend binds a begin/drive/end triplet to a label for logging.
 */
struct Backend
{
    const char *label;       ///< Text label for logging (e.g., "LEDC", "MCPWM").
    void (*begin)();         ///< Initialize and enable outputs.
    void (*drivePct)(float); ///< Drive with a percentage duty (0..100).
    void (*end)();           ///< Disable outputs and release pins.
};

// ===================================================================
//                       LEDC backend (RPWM-only)
// ===================================================================
namespace LEDC
{
    /** @brief Initialize LEDC channel and enable the bridge. */
    static void begin()
    {
        if (EN_PIN >= 0)
        {
            pinMode(EN_PIN, OUTPUT);
            digitalWrite(EN_PIN, LOW); ///< Open-circuit initially.
        }
        pinMode(LPWM_PIN, OUTPUT);
        digitalWrite(LPWM_PIN, LOW); ///< Other leg LOW.

        ledcSetup(LEDC_CH_R, PWM_FREQ_HZ, LEDC_BITS);
        ledcAttachPin(RPWM_PIN, LEDC_CH_R);
        ledcWrite(LEDC_CH_R, 0);

        if (EN_PIN >= 0)
            digitalWrite(EN_PIN, HIGH); ///< Enable bridge.
    }

    /**
     * @brief Drive LEDC RPWM with a duty percentage.
     *
     * @param pct Duty percentage (0..100). Clamped internally.
     */
    static void drivePct(float pct)
    {
        pct = clampPct(pct);
        if (pct < MIN_EFFECTIVE_LEDC)
            pct = MIN_EFFECTIVE_LEDC;

        if (EN_PIN >= 0)
            digitalWrite(EN_PIN, HIGH);

        digitalWrite(LPWM_PIN, LOW); ///< Keep other leg solid LOW.
        ledcWrite(LEDC_CH_R, pctToDuty10(pct));
    }

    /** @brief Disable LEDC outputs and release pins. */
    static void end()
    {
        ledcWrite(LEDC_CH_R, 0);
        digitalWrite(LPWM_PIN, LOW);
        if (EN_PIN >= 0)
            digitalWrite(EN_PIN, LOW); ///< Hi-Z/open.

        ledcDetachPin(RPWM_PIN);
        pinMode(RPWM_PIN, INPUT);
        pinMode(LPWM_PIN, INPUT);
        delay(50);
    }
} // namespace LEDC

// ===================================================================
//                     MCPWM backend (Library)
// ===================================================================
namespace MCPWM
{
    static Motor motor;         ///< Motor instance provided by ESP32_MCPWM library.
    static bool inited = false; ///< True once the backend is configured.

    /** @brief One-time MCPWM configuration and pin routing. */
    static void initOnce()
    {
        MotorMCPWMConfig hw{
            /* lpwm_pin      */ LPWM_PIN,
            /* rpwm_pin      */ RPWM_PIN,
            /* en_pin        */ EN_PIN,
            /* unit          */ MCPWM_UNIT_0,
            /* timer         */ MCPWM_TIMER_0,
            /* pwm_sig_left  */ MCPWM0A,
            /* pwm_sig_right */ MCPWM0B};
        hw.pwm_freq_hz = PWM_FREQ_HZ;
        hw.input_max = (1 << LEDC_BITS) - 1;
        hw.counter = MCPWM_UP_DOWN_COUNTER; ///< Center-aligned.
        hw.use_deadtime = false;            ///< IBT-2 handles this internally.

        MotorBehaviorConfig beh{
            FreewheelMode::HiZ, // true coast (EN low)
            /* soft_brake_hz     */ 300,
            /* dither_pwm        */ 0,
            /* default_soft      */ 0,
            /* min_phase_us      */ 1000,
            /* dither_coast_hi_z */ true};

        // Route pins (re-assert ownership if LEDC ran first).
        mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, LPWM_PIN);
        mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, RPWM_PIN);

        motor.setup(hw, beh);
        inited = true;
    }

    /** @brief Initialize (once) and start MCPWM outputs. */
    static void begin()
    {
        if (!inited)
            initOnce();

        if (EN_PIN >= 0)
        {
            pinMode(EN_PIN, OUTPUT);
            digitalWrite(EN_PIN, LOW); ///< Disabled initially.
        }

        // Re-bind pins each phase in case LEDC ran first.
        mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, LPWM_PIN);
        mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, RPWM_PIN);

        motor.start();

        if (EN_PIN >= 0)
            digitalWrite(EN_PIN, HIGH); ///< Enable bridge.
    }

    /**
     * @brief Drive MCPWM with a duty percentage (Dir::CCW).
     *
     * @param pct Duty percentage (0..100). Clamped internally.
     */
    static void drivePct(float pct)
    {
        motor.setSpeedPercent(clampPct(pct), Dir::CCW);
    }

    /** @brief Coast outputs and release pins. */
    static void end()
    {
        motor.applyFreewheel(FreewheelMode::HiZ);
        if (EN_PIN >= 0)
            digitalWrite(EN_PIN, LOW);
        pinMode(RPWM_PIN, INPUT);
        pinMode(LPWM_PIN, INPUT);
        delay(50);
    }
} // namespace MCPWM

// ===================================================================
//                           Demo runner
// ===================================================================

/**
 * @brief Perform a low-speed up/down duty sweep and hold a tricky low value.
 *
 * @param b Backend used to issue begin/drive/end calls.
 */
static inline void runSweep(const Backend &b)
{
    Serial.printf("[%s] Low-speed sweep %.1f%% → %.1f%% → %.1f%%\n",
                  b.label, SWEEP_START_PCT, SWEEP_STOP_PCT, SWEEP_START_PCT);

    // Up
    for (float p = SWEEP_START_PCT; p <= SWEEP_STOP_PCT + 0.001f; p += SWEEP_STEP_PCT)
    {
        b.drivePct(p);
        Serial.printf("[%s] Duty = %.1f%%\n", b.label, p);
        delay(SWEEP_HOLD_MS);
    }
    // Down
    for (float p = SWEEP_STOP_PCT; p >= SWEEP_START_PCT - 0.001f; p -= SWEEP_STEP_PCT)
    {
        b.drivePct(p);
        Serial.printf("[%s] Duty = %.1f%%\n", b.label, p);
        delay(SWEEP_HOLD_MS);
    }

    // Hold tricky low value (5s)
    b.drivePct(SWEEP_HOLD_PCT);
    Serial.printf("[%s] Hold = %.1f%% (5s)\n", b.label, SWEEP_HOLD_PCT);
    delay(5000);
}

// ---------------- Expose ready-made backends ----------------

/** @brief Ready-made LEDC backend binding. */
static Backend LEDC_BACKEND{"LEDC", &LEDC::begin, &LEDC::drivePct, &LEDC::end};

/** @brief Ready-made MCPWM backend binding. */
static Backend MCPWM_BACKEND{"MCPWM", &MCPWM::begin, &MCPWM::drivePct, &MCPWM::end};