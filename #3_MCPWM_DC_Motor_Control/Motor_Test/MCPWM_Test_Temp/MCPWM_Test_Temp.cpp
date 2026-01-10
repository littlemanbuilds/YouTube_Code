/**
 * MIT License
 *
 * @brief Implementation of MCPWM/LEDC backends and safety helpers for stress test.
 *
 * @file MCPWM_Test_Temp.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-09-05
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#include "MCPWM_Test_Temp.h"

// =================== Local state (MCPWM) ===================
namespace
{
    Motor mcpwmMotor;         ///< MCPWM motor instance (library-provided).
    bool mcpwmInited = false; ///< True if MCPWM backend initialized.
}

// -------------------- ADC / Fault helpers --------------------

// Read VBUS via ADC and divider.
float readVbusVolts()
{
    if (SUPPLY_ADC_PIN < 0)
        return -1.0f;
#if CONFIG_IDF_TARGET_ESP32S3
    const uint32_t mv = analogReadMilliVolts(SUPPLY_ADC_PIN);
    return (static_cast<float>(mv) / 1000.0f) * VDIV_RATIO;
#else
    const int raw = analogRead(SUPPLY_ADC_PIN);
    const float mv = (static_cast<float>(raw) / 4095.0f) * 1100.0f; ///< Approx.
    return (mv / 1000.0f) * VDIV_RATIO;
#endif
}

// Check fault/E-STOP input (optional).
bool faultActive()
{
    if (FAULT_PIN < 0)
        return false;
    const int v = digitalRead(FAULT_PIN);
    return FAULT_ACTIVE_LOW ? (v == LOW) : (v == HIGH);
}

// ======================================================
//                MCPWM backend
// ======================================================

// Begin MCPWM phase: route pins, configure, and enable.
void mcpwm_begin_phase()
{
    if (!mcpwmInited)
    {
        // Route MCPWM to SAME pins (A->LPWM=5, B->RPWM=4).
        mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, LPWM_PIN);
        mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, RPWM_PIN);

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
        hw.use_deadtime = false;            ///< IBT-2 has internal handling.

        MotorBehaviorConfig beh{
            FreewheelMode::HiZ, ///< True coast via EN low.
            /* soft_brake_hz   */ 300,
            /* dither_pwm      */ 0,
            /* default_soft    */ 0,
            /* min_phase_us    */ 1000,
            /* dither_coast_hi_z */ true};

        mcpwmMotor.setup(hw, beh);
        mcpwmInited = true;
    }

    // Re-bind pins (matrix) in case LEDC released them.
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, LPWM_PIN);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, RPWM_PIN);

    if (EN_PIN >= 0)
    {
        pinMode(EN_PIN, OUTPUT);
        digitalWrite(EN_PIN, HIGH);
    }
    mcpwmMotor.start();
}

// Drive MCPWM at percentage duty.
void mcpwm_drive(float pct)
{
    mcpwmMotor.setSpeedPercent(clampPct(pct), Dir::CCW);
}

// Coast MCPWM outputs.
void mcpwm_coast()
{
    mcpwmMotor.applyFreewheel(FreewheelMode::HiZ);
}

// End MCPWM phase and release pins.
void mcpwm_end_phase()
{
    mcpwm_coast();
    if (EN_PIN >= 0)
        digitalWrite(EN_PIN, LOW);
    pinMode(RPWM_PIN, INPUT);
    pinMode(LPWM_PIN, INPUT);
    if (EN_PIN >= 0)
        pinMode(EN_PIN, INPUT);
    delay(50);
}

// ======================================================
//                LEDC backend (RPWM-only)
// ======================================================

namespace
{
    constexpr int LEDC_CH_RPWM = 0;
} ///< LEDC channel for RPWM.

// Begin LEDC phase: configure channel and enable.
void ledc_begin_phase()
{
    // Forward leg: LPWM held LOW
    pinMode(LPWM_PIN, OUTPUT);
    digitalWrite(LPWM_PIN, LOW);

    ledcSetup(LEDC_CH_RPWM, PWM_FREQ_HZ, LEDC_BITS);
    ledcAttachPin(RPWM_PIN, LEDC_CH_RPWM);
    ledcWrite(LEDC_CH_RPWM, 0);

    if (EN_PIN >= 0)
    {
        pinMode(EN_PIN, OUTPUT);
        digitalWrite(EN_PIN, HIGH); ///< Enable bridge.
    }
}

// Drive LEDC at percentage duty.
void ledc_drive(float pct)
{
    pct = clampPct(pct);
    if (pct < MIN_EFFECTIVE_LEDC)
        pct = MIN_EFFECTIVE_LEDC;
    ledcWrite(LEDC_CH_RPWM, pctToDuty10(pct));
    if (EN_PIN >= 0)
        digitalWrite(EN_PIN, HIGH);
    digitalWrite(LPWM_PIN, LOW); ///< Keep other leg LOW.
}

// Coast LEDC outputs.
void ledc_coast()
{
    ledcWrite(LEDC_CH_RPWM, 0);
    if (EN_PIN >= 0)
        digitalWrite(EN_PIN, LOW);
    digitalWrite(LPWM_PIN, LOW);
}

// End LEDC phase and release pins.
void ledc_end_phase()
{
    ledc_coast();
    ledcDetachPin(RPWM_PIN);
    pinMode(RPWM_PIN, INPUT);
    pinMode(LPWM_PIN, INPUT);
    if (EN_PIN >= 0)
        pinMode(EN_PIN, INPUT);
    delay(50);
}