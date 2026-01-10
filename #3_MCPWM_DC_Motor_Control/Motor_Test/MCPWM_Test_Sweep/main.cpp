/**
 * MIT License
 *
 * @brief Alternates LEDC and MCPWM sweep tests.
 *
 * @file main.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-09-05
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#include "MCPWM_Test_Sweep.h"

void setup()
{
  Serial.begin(115200);
  delay(250);
  Serial.println("Alternating LEDC <-> MCPWM.");
}

void loop()
{
  // -------- LEDC PHASE (First) --------
  Serial.println("PHASE: LEDC");
  LEDC_BACKEND.begin();
  runSweep(LEDC_BACKEND);
  LEDC_BACKEND.end();
  delay(1000);

  // -------- MCPWM PHASE (Second) --------
  Serial.println("PHASE: MCPWM");
  MCPWM_BACKEND.begin();
  runSweep(MCPWM_BACKEND);
  MCPWM_BACKEND.end();
  delay(1000);
}