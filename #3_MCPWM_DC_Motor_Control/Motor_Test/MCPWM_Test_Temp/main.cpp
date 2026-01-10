/**
 * MIT License
 *
 * @brief Entry point for safe MCPWM vs LEDC stress test.
 *
 * @file main.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-09-05
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#include "MCPWM_Test_Temp.h"

static float mcpwmLastPct = 0.0f; ///< Last commanded duty (percent) for MCPWM phase.
static float ledcLastPct = 0.0f;  ///< Last commanded duty (percent) for LEDC phase.

void setup()
{
    Serial.begin(115200);
    delay(5000);
    Serial.println("Safe Stress Test: MCPWM -> LEDC.");

    if (FAULT_PIN >= 0)
    {
        pinMode(FAULT_PIN, INPUT_PULLUP);
        Serial.println("FAULT/E-STOP input ENABLED.");
    }
    else
    {
        Serial.println("FAULT/E-STOP input DISABLED.");
    }

    if (SUPPLY_ADC_PIN >= 0)
    {
#if CONFIG_IDF_TARGET_ESP32S3
        analogReadResolution(12);
        analogSetAttenuation(ADC_11db);
#endif
        Serial.println("Over-voltage guard ENABLED (set divider & limits to taste).");
    }
    else
    {
        Serial.println("Over-voltage guard DISABLED (set SUPPLY_ADC_PIN to enable).");
    }
}

void loop()
{
    // -------- MCPWM PHASE (First) --------
    Serial.println("PHASE: MCPWM");
    mcpwm_begin_phase();
    do_warmup("MCPWM", mcpwm_drive, mcpwm_coast, mcpwmLastPct);
    do_step_load("MCPWM", mcpwm_drive, mcpwm_coast, mcpwmLastPct);
    do_bursts("MCPWM", mcpwm_drive, mcpwm_coast, mcpwmLastPct);
    do_soak("MCPWM", mcpwm_drive, mcpwm_coast, mcpwmLastPct);
    mcpwm_end_phase();

    Serial.println("PHASE: BREAK/COOLDOWN - 5 MINS");
    delay(300000);

    // -------- LEDC PHASE (Second) --------
    Serial.println("PHASE: LEDC");
    ledc_begin_phase();
    do_warmup("LEDC", ledc_drive, ledc_coast, ledcLastPct);
    do_step_load("LEDC", ledc_drive, ledc_coast, ledcLastPct);
    do_bursts("LEDC", ledc_drive, ledc_coast, ledcLastPct);
    do_soak("LEDC", ledc_drive, ledc_coast, ledcLastPct);
    ledc_end_phase();

    Serial.println("PHASE: BREAK/COOLDOWN - 5 MINS");
    delay(300000);
}