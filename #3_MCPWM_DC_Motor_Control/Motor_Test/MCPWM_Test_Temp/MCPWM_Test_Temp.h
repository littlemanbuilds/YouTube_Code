/**
 * MIT License
 *
 * @brief Shared configuration, helpers, and test routines for MCPWM/LEDC stress test.
 *
 * @file MCPWM_Test_Temp.h
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

// -------------------- Pin map --------------------
constexpr int RPWM_PIN = 37; ///< IBT-2 RPWM pin.
constexpr int LPWM_PIN = 38; ///< IBT-2 LPWM pin.
constexpr int EN_PIN = 39;   ///< IBT-2 EN pin.

// Optional FAULT input (active LOW by default). Set to -1 to disable.
constexpr int FAULT_PIN = -1;           ///< Fault/E-STOP input pin; -1 to disable.
constexpr bool FAULT_ACTIVE_LOW = true; ///< Fault is active-low if true.

// -------------------- PWM config --------------------
constexpr uint32_t PWM_FREQ_HZ = 20000; ///< PWM frequency (Hz).
constexpr int LEDC_BITS = 10;           ///< LEDC resolution bits (0..1023).

// -------------------- Timings --------------------
constexpr uint32_t WARMUP_MS = 30000;       ///< Warm-up duration (ms) at 40%.
constexpr uint32_t STEP_BLOCK_MS = 240000;  ///< Total step test duration (ms).
constexpr uint32_t STEP_HOLD_MS = 500;      ///< Dwell per step (ms).
constexpr uint32_t BURST_BLOCK_MS = 240000; ///< Total burst test duration (ms).
constexpr uint32_t BURST_ON_MS = 320;       ///< Burst drive window (ms).
constexpr uint32_t BURST_COAST_MS = 120;    ///< Coast gap between bursts (ms).
constexpr uint32_t SOAK_MS = 600000;        ///< Heat soak duration (ms) at 50%.
constexpr uint32_t GAP_MS = 1200;           ///< Coast gap between phases (ms).

// -------------------- Duty targets --------------------
constexpr float WARMUP_DUTY = 42.0f; ///< Warm-up duty (%).
constexpr float STEP_LOW = 35.0f;    ///< Step-low duty (%).
constexpr float STEP_HIGH = 75.0f;   ///< Step-high duty (%).
constexpr float BURST_DUTY = 88.0f;  ///< Burst duty (%).
constexpr float SOAK_DUTY = 68.0f;   ///< Soak duty (%).

// -------------------- Safer stepping --------------------
constexpr float MAX_SLEW_STEP_PCT = 3.0f;      ///< Max duty change per slew step (%).
constexpr uint32_t SLEW_STEP_MS = 18;          ///< Time between slew steps (ms).
constexpr float COAST_BEFORE_JUMP_PCT = 12.0f; ///< Threshold to coast before large step (%).
constexpr uint32_t COAST_BEFORE_JUMP_MS = 90;  ///< Coast time before large step (ms).
constexpr uint32_t COAST_AFTER_JUMP_MS = 60;   ///< Coast time after large step (ms).

// LEDC lower bound (helps sticky rigs)
constexpr float MIN_EFFECTIVE_LEDC = 0.0f; ///< Minimum effective LEDC duty (%).

// ---------- Optional supply over-voltage guard ----------
constexpr int SUPPLY_ADC_PIN = -1;            ///< ADC pin for VBUS; set >=0 to enable.
constexpr float VDIV_RATIO = 11.0f;           ///< Divider ratio: (Rtop+Rbot)/Rbot.
constexpr float VBUS_OV_LIMIT_VOLTS = 28.0f;  ///< Over-voltage trip threshold (V).
constexpr float VBUS_CLEAR_HYS_VOLTS = 26.5f; ///< Re-enable threshold with hysteresis (V).

// -------------------- helpers --------------------

/**
 * @brief Clamp a percentage to [0, 100].
 *
 * @param p Percentage value.
 * @return float Clamped value in [0, 100].
 */
static inline float clampPct(float p) { return (p < 0.0f) ? 0.0f : ((p > 100.0f) ? 100.0f : p); }

/**
 * @brief Convert a percentage (0..100) to a duty value for LEDC resolution.
 *
 * @param pct Duty percentage.
 * @return uint32_t Duty value in range [0, (1<<LEDC_BITS)-1].
 */
static inline uint32_t pctToDuty10(float pct)
{
    pct = clampPct(pct);
    return static_cast<uint32_t>(std::lroundf(pct * (static_cast<float>((1u << LEDC_BITS) - 1)) / 100.0f));
}

/**
 * @brief Read the DC bus (VBUS) voltage via ADC.
 *
 * @return float VBUS voltage in volts, or negative on error/disabled.
 */
float readVbusVolts();

/**
 * @brief Check whether a fault/E-STOP is currently active.
 *
 * @return true If active.
 * @return false Otherwise.
 */
bool faultActive();

// ======================================================
//                MCPWM backend
// ======================================================

/** @brief Begin the MCPWM test phase (configure pins/timers, enable outputs). */
void mcpwm_begin_phase();

/**
 * @brief Drive MCPWM with a percentage duty command.
 *
 * @param pct Duty percentage (0..100).
 */
void mcpwm_drive(float pct);

/** @brief Put MCPWM outputs into freewheel/coast. */
void mcpwm_coast();

/** @brief End the MCPWM phase and release pins. */
void mcpwm_end_phase();

// ======================================================
//                LEDC backend (RPWM-only)
// ======================================================

/** @brief Begin the LEDC test phase (configure pins/channels, enable outputs). */
void ledc_begin_phase();

/**
 * @brief Drive LEDC with a percentage duty command.
 *
 * @param pct Duty percentage (0..100).
 */
void ledc_drive(float pct);

/** @brief Put LEDC outputs into freewheel/coast. */
void ledc_coast();

/** @brief End the LEDC phase and release pins. */
void ledc_end_phase();

// ======================================================
//                  Safety + Ramps (templates)
// ======================================================

/**
 * @brief Over-voltage guard: coasts outputs if VBUS exceeds limit and waits for clear.
 *
 * @tparam CoastFunc Callable of signature void().
 * @param coastFn Function to coast outputs.
 * @param label Text label for logging.
 * @return true If an over-voltage event occurred.
 * @return false Otherwise.
 */
template <typename CoastFunc>
bool guardOverVoltage(CoastFunc coastFn, const char *label)
{
    if (SUPPLY_ADC_PIN < 0)
        return false;
    float vb = readVbusVolts();
    if (vb >= 0.0f && vb > VBUS_OV_LIMIT_VOLTS)
    {
        Serial.printf("[%s] OV trip: Vbus=%.2f V -> COAST...\n", label, vb);
        coastFn();
        const uint32_t t0 = millis();
        while (millis() - t0 < 3000U)
        {
            vb = readVbusVolts();
            if (vb >= 0.0f && vb < VBUS_CLEAR_HYS_VOLTS)
                break;
            delay(10);
        }
        Serial.printf("[%s] OV clear: Vbus=%.2f V\n", label, vb);
        return true;
    }
    return false;
}

/**
 * @brief Safe slew to a target duty with optional coast before/after large changes.
 *
 * Checks fault and over-voltage between steps.
 *
 * @tparam DriveFunc Callable of signature void(float pct).
 * @tparam CoastFunc Callable of signature void().
 * @param label Text label for logging.
 * @param drive Drive function.
 * @param coast Coast function.
 * @param lastPct Reference to last commanded percentage (will be updated).
 * @param targetPct Target duty percentage (0..100).
 */
template <typename DriveFunc, typename CoastFunc>
void safeRampTo(const char *label, DriveFunc drive, CoastFunc coast,
                float &lastPct, float targetPct)
{
    targetPct = clampPct(targetPct);
    const float delta = targetPct - lastPct;

    // E-STOP?
    if (faultActive())
    {
        coast();
        return;
    }
    // Optional OV guard
    if (guardOverVoltage(coast, label))
    {
        lastPct = 0.0f;
        return;
    }

    if (fabsf(delta) >= COAST_BEFORE_JUMP_PCT)
    {
        coast();
        delay(COAST_BEFORE_JUMP_MS);
    }

    int steps = static_cast<int>(ceilf(fabsf(delta) / MAX_SLEW_STEP_PCT));
    if (steps < 1)
        steps = 1;

    const float stepSize = delta / static_cast<float>(steps);
    for (int i = 0; i < steps; ++i)
    {
        lastPct += stepSize;
        if (faultActive())
        {
            coast();
            return;
        }
        if (guardOverVoltage(coast, label))
        {
            lastPct = 0.0f;
            return;
        }
        drive(lastPct);
        delay(SLEW_STEP_MS);
    }

    if (fabsf(delta) >= COAST_BEFORE_JUMP_PCT)
    {
        coast();
        delay(COAST_AFTER_JUMP_MS);
    }
}

// ======================================================
//                   STRESS ROUTINES (templates)
// ======================================================

/**
 * @brief Warm-up at a fixed duty for a duration.
 *
 * @tparam DriveFunc Callable void(float).
 * @tparam CoastFunc Callable void().
 * @param label Label for logs.
 * @param drive Drive function.
 * @param coast Coast function.
 * @param lastPct Reference to last commanded duty.
 */
template <typename DriveFunc, typename CoastFunc>
void do_warmup(const char *label, DriveFunc drive, CoastFunc coast, float &lastPct)
{
    Serial.printf("[%s] Warm-up @ %.1f%% for %u ms\n", label, WARMUP_DUTY, static_cast<unsigned>(WARMUP_MS));
    const uint32_t t0 = millis();
    while (millis() - t0 < WARMUP_MS)
    {
        safeRampTo(label, drive, coast, lastPct, WARMUP_DUTY);
        if (faultActive())
        {
            coast();
            break;
        }
        delay(10);
    }
    coast();
    delay(BURST_COAST_MS);
}

/**
 * @brief Alternating step load between STEP_LOW and STEP_HIGH.
 */
template <typename DriveFunc, typename CoastFunc>
void do_step_load(const char *label, DriveFunc drive, CoastFunc coast, float &lastPct)
{
    Serial.printf("[%s] Step load %g <-> %g %% for %u ms\n", label, STEP_LOW, STEP_HIGH, static_cast<unsigned>(STEP_BLOCK_MS));
    const uint32_t t0 = millis();
    bool hi = false;
    while (millis() - t0 < STEP_BLOCK_MS)
    {
        safeRampTo(label, drive, coast, lastPct, hi ? STEP_HIGH : STEP_LOW);
        if (faultActive())
        {
            coast();
            break;
        }
        delay(STEP_HOLD_MS);
        hi = !hi;
    }
    coast();
    delay(BURST_COAST_MS);
}

/**
 * @brief Repeating burst pulses separated by coast gaps.
 */
template <typename DriveFunc, typename CoastFunc>
void do_bursts(const char *label, DriveFunc drive, CoastFunc coast, float &lastPct)
{
    Serial.printf("[%s] Bursts: coast %ums -> %g%% %ums (repeat %u ms)\n",
                  label, static_cast<unsigned>(BURST_COAST_MS), BURST_DUTY,
                  static_cast<unsigned>(BURST_ON_MS), static_cast<unsigned>(BURST_BLOCK_MS));
    const uint32_t t0 = millis();
    while (millis() - t0 < BURST_BLOCK_MS)
    {
        coast();
        delay(BURST_COAST_MS); ///< Let current decay.
        safeRampTo(label, drive, coast, lastPct, BURST_DUTY);
        if (faultActive())
        {
            coast();
            break;
        }
        delay(BURST_ON_MS);
    }
    coast();
    delay(BURST_COAST_MS);
}

/**
 * @brief Heat soak at a fixed duty for a duration.
 */
template <typename DriveFunc, typename CoastFunc>
void do_soak(const char *label, DriveFunc drive, CoastFunc coast, float &lastPct)
{
    Serial.printf("[%s] Heat soak @ %.1f%% for %u ms\n", label, SOAK_DUTY, static_cast<unsigned>(SOAK_MS));
    const uint32_t t0 = millis();
    while (millis() - t0 < SOAK_MS)
    {
        safeRampTo(label, drive, coast, lastPct, SOAK_DUTY);
        if (faultActive())
        {
            coast();
            break;
        }
        delay(20);
    }
    coast();
    delay(GAP_MS);
}