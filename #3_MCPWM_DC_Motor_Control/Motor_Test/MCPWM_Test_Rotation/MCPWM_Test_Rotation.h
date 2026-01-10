/**
 * MIT License
 *
 * @brief Shared config and backends for LEDC/MCPWM rotation demo:
 *        hold 20% duty, then 75% duty for a clear visual/thermal compare.
 *
 * @file MCPWM_Test_Rotation.h
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-09-08
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
constexpr int EN_PIN = 39;   ///< IBT-2 EN pin;

// ---------------- Common PWM config ----------------
constexpr uint32_t PWM_FREQ_HZ = 20000; ///< Quiet 20 kHz.
constexpr int LEDC_BITS = 10;           ///< LEDC resolution (0..1023).
constexpr int LEDC_CH_R = 0;            ///< LEDC channel for RPWM.

// ---------------- Rotation test config ----------------
constexpr float LOW_DUTY_PCT = 20.0f;      ///< First hold (%).
constexpr float HIGH_DUTY_PCT = 75.0f;     ///< Second hold (%).
constexpr uint32_t HOLD_LOW_MS = 30000;    ///< Hold time at LOW (%).
constexpr uint32_t HOLD_HIGH_MS = 30000;   ///< Hold time at HIGH (%).
constexpr uint32_t COAST_GAP_MS = 2000;    ///< Small settling coast between holds.
constexpr float MIN_EFFECTIVE_LEDC = 0.0f; ///< Keep if your rig needs a floor.

// ---------------- Tiny helpers ----------------
static inline float clampPct(float p)
{
    return (p < 0.0f) ? 0.0f : ((p > 100.0f) ? 100.0f : p);
}
static inline uint32_t pctToDuty10(float pct)
{
    pct = clampPct(pct);
    const uint32_t maxDuty = (1u << LEDC_BITS) - 1u;
    return static_cast<uint32_t>(std::lroundf((pct * static_cast<float>(maxDuty)) / 100.0f));
}

// ===================================================================
//                       Backend interface
// ===================================================================
struct Backend
{
    const char *label;           ///< "LEDC" or "MCPWM"
    void (*begin)();             ///< Initialize/enable outputs
    void (*drivePct)(float pct); ///< Drive with duty % (0..100)
    void (*end)();               ///< Disable/release outputs
};

// ===================================================================
//                       LEDC backend (RPWM-only)
// ===================================================================
namespace LEDC
{
    static void begin()
    {
        if (EN_PIN >= 0)
        {
            pinMode(EN_PIN, OUTPUT);
            digitalWrite(EN_PIN, LOW); ///< Open initially.
        }
        pinMode(LPWM_PIN, OUTPUT);
        digitalWrite(LPWM_PIN, LOW); ///< Other leg LOW.

        ledcSetup(LEDC_CH_R, PWM_FREQ_HZ, LEDC_BITS);
        ledcAttachPin(RPWM_PIN, LEDC_CH_R);
        ledcWrite(LEDC_CH_R, 0);

        if (EN_PIN >= 0)
            digitalWrite(EN_PIN, HIGH); ///< Enable bridge.
    }

    static void drivePct(float pct)
    {
        pct = clampPct(pct);
        if (pct < MIN_EFFECTIVE_LEDC)
            pct = MIN_EFFECTIVE_LEDC;
        if (EN_PIN >= 0)
            digitalWrite(EN_PIN, HIGH);
        digitalWrite(LPWM_PIN, LOW);
        ledcWrite(LEDC_CH_R, pctToDuty10(pct));
    }

    static void end()
    {
        ledcWrite(LEDC_CH_R, 0);
        digitalWrite(LPWM_PIN, LOW);
        if (EN_PIN >= 0)
            digitalWrite(EN_PIN, LOW);
        ledcDetachPin(RPWM_PIN);
        pinMode(RPWM_PIN, INPUT);
        pinMode(LPWM_PIN, INPUT);
        delay(50);
    }
} // namespace LEDC

// ===================================================================
//                       MCPWM backend (library)
// ===================================================================
namespace MCPWM
{
    static Motor motor; ///< ESP32_MCPWM library motor instance.
    static bool inited = false;

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
        hw.use_deadtime = false;            ///< IBT-2 handles internally.

        MotorBehaviorConfig beh{
            FreewheelMode::HiZ, /* soft_brake_hz     */ 300,
            /* dither_pwm        */ 0,
            /* default_soft      */ 0,
            /* min_phase_us      */ 1000,
            /* dither_coast_hi_z */ true};

        // Ensure pins are routed (esp. if LEDC ran first).
        mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, LPWM_PIN);
        mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, RPWM_PIN);

        motor.setup(hw, beh);
        inited = true;
    }

    static void begin()
    {
        if (!inited)
            initOnce();

        if (EN_PIN >= 0)
        {
            pinMode(EN_PIN, OUTPUT);
            digitalWrite(EN_PIN, LOW); ///< Disabled initially.
        }

        // Re-bind pins each phase.
        mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, LPWM_PIN);
        mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, RPWM_PIN);

        motor.start();
        if (EN_PIN >= 0)
            digitalWrite(EN_PIN, HIGH);
    }

    static void drivePct(float pct)
    {
        motor.setSpeedPercent(clampPct(pct), Dir::CCW);
    }

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
//             Demo runner: 20% hold -> coast -> 75% hold
// ===================================================================
static inline void runRotation(const Backend &b)
{
    Serial.printf("[%s] Hold %.1f%% for %u ms\n",
                  b.label, LOW_DUTY_PCT, (unsigned)HOLD_LOW_MS);
    b.drivePct(LOW_DUTY_PCT);
    delay(HOLD_LOW_MS);

    // Brief coast/settle to make the step visually obvious on camera/scope.
    b.drivePct(0.0f);
    delay(COAST_GAP_MS);

    Serial.printf("[%s] Hold %.1f%% for %u ms\n",
                  b.label, HIGH_DUTY_PCT, (unsigned)HOLD_HIGH_MS);
    b.drivePct(HIGH_DUTY_PCT);
    delay(HOLD_HIGH_MS);

    // finish coasting
    b.drivePct(0.0f);
    delay(COAST_GAP_MS);
}

// Expose ready-made backends
static Backend LEDC_BACKEND{"LEDC", &LEDC::begin, &LEDC::drivePct, &LEDC::end};
static Backend MCPWM_BACKEND{"MCPWM", &MCPWM::begin, &MCPWM::drivePct, &MCPWM::end};