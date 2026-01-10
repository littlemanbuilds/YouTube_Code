/**
 * MIT License
 *
 * @brief Alternates LEDC and MCPWM rotation holds (20% then 75%).
 *
 * @file main.cpp
 * @author Little Man Builds
 * @date 2025-09-08
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#include "MCPWM_Test_Rotation.h"

void setup()
{
  Serial.begin(115200);
  delay(250);
  Serial.println("Rotation Test: LEDC(20%->75%) then MCPWM(20%->75%).");
}

void loop()
{
  // -------- LEDC PHASE (First) --------
  Serial.println("PHASE: LEDC");
  LEDC_BACKEND.begin();
  runRotation(LEDC_BACKEND);
  LEDC_BACKEND.end();
  delay(1000);

  // -------- MCPWM PHASE (Second) --------
  Serial.println("PHASE: MCPWM");
  MCPWM_BACKEND.begin();
  runRotation(MCPWM_BACKEND);
  MCPWM_BACKEND.end();
  delay(1000);
}