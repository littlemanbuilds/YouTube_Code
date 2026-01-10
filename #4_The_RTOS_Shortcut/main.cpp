/**
 * MIT License
 *
 * @brief Main program for ESP32-based project. Sets up all hardware, RTOS tasks, queues, and system initialization.
 *
 * @file main.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-08-01
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#include <app_config.h>
#include <Universal_Button.h>
#include <ESP32_MCPWM.h>
#include <StateManager/StateManager.h>
#include <PowerDriveHandler/PowerDriveHandler.h>

// Optional: Let other tasks run if we catch the writer mid-update.
#define SNAPSHOTBUS_YIELD() taskYIELD()
#include <SnapshotBus.h>
#include <SnapshotModel.h>
#include <InputTypes.h>

/**
 * @brief Constants and type definitions.
 * @note On ESP32/FreeRTOS, stack size is in words (4 bytes each), not bytes.
 */
constexpr int SM_STACK = 2048;  ///< Memory allocated to state manager (~8 KB).
constexpr int PDH_STACK = 4096; ///< Memory allocated to power drive handler (~16 KB).

constexpr UBaseType_t SM_PRI = 1;  ///< Task Priority 1.
constexpr UBaseType_t PDH_PRI = 2; ///< Task Priority 2.

/**
 * @brief Global RTOS handles and queues.
 */
TaskHandle_t sm_t = nullptr;  ///< State manager logic task handle.
TaskHandle_t pdh_t = nullptr; ///< Power drive handler logic task handle.

void setup()
{
  // ---- Start serial monitor ----
  Serial.begin(115200);
  delay(200);

  debugln("===== Startup =====");

  // ---- Shared InputBus ----
  static InputBus inputBus{};

  // ---- Button setup ----
  const ButtonTimingConfig kTiming{cfg::button::BTN_DEBOUNCE_MS, cfg::button::BTN_SHORT_MS,
                                   cfg::button::BTN_LONG_MS};
  static Button btnHandler = makeButtons(kTiming);

  // ---- Motor setup ----
  static Motor driveMotor;

  MotorMCPWMConfig hw{};
  hw.rpwm_pin = cfg::motor::RPWM_PIN;
  hw.lpwm_pin = cfg::motor::LPWM_PIN;
  hw.en_pin = cfg::motor::EN_PIN;

  driveMotor.setup(hw);

  // ---- Managers ----
  static StateManager sm(btnHandler, inputBus);
  static PowerDriveHandler pdh(driveMotor, inputBus);

  // ---- FreeRTOS tasks ----
  configASSERT(xTaskCreatePinnedToCore(StateManager::task, "StateManager", SM_STACK, &sm, SM_PRI, &sm_t, /*Core=*/0) == pdPASS);
  delay(50);
  configASSERT(xTaskCreatePinnedToCore(PowerDriveHandler::task, "PDHandler", PDH_STACK, &pdh, PDH_PRI, &pdh_t, /*Core=*/1) == pdPASS);
  delay(50);

  debugln("All RTOS tasks started!");
}

/**
 * @brief Main Arduino loop.
 * @note Not used because all code is handled via RTOS tasks. This function simply deletes itself.
 */
void loop()
{
  vTaskDelete(NULL); ///< Delete this task.
}