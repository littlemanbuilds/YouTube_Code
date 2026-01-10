/**
 * MIT License
 *
 * @brief Main program for ESP32-based project. Sets up all hardware, RTOS tasks, queues, and system initialization.
 *
 * @file main.cpp
 * @author Little Man Builds
 * @date 2025-08-01
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#include <app_config.h>

/**
 * @brief Constants and type definitions.
 * @note On ESP32/FreeRTOS, stack size is in words (4 bytes each), not bytes.
 */
constexpr int LISTENER_STACK = 2048; ///< Memory allocated to the listener stack (~8 KB).
constexpr int HANDLER_STACK = 4096;  ///< Memory allocated to the handler stack (~16 KB).

constexpr UBaseType_t PRI_LISTENER = 1; ///< Task Priority 1.
constexpr UBaseType_t PRI_HANDLER = 2;  ///< Task Priority 2.

/**
 * @brief Global RTOS handles and queues.
 */
TaskHandle_t listener_t = nullptr; ///< Listener logic task handle.
TaskHandle_t handler_t = nullptr;  ///< Handler logic task handle.

/**
 * @brief Function declarations.
 */
void listener(void *parameter);
void handler(void *parameter);

void setup()
{
  // Start serial debugging output.
  Serial.begin(115200); // Default baud rate of the ESP32.

  debugln("===== Startup =====");

  // RTOS Task Creation.
  configASSERT(xTaskCreatePinnedToCore(listener,       ///< Task function (must be void listener(void *)).
                                       "listener",     ///< Task name (for debugging).
                                       LISTENER_STACK, ///< Stack size (in bytes).
                                       0,              ///< Parameter passed to the task.
                                       PRI_LISTENER,   ///< Task Priority.
                                       &listener_t,    ///< Pointer to the task handle.
                                       0) == pdPASS);  ///< Core to pin task to (core 0).
  delay(50);
  configASSERT(xTaskCreatePinnedToCore(handler, "handler", HANDLER_STACK, 0, PRI_HANDLER, &handler_t, 0) == pdPASS);
  delay(50);

  debugln("All RTOS tasks started!");
}

/**
 * @brief Main Arduino loop.
 *
 * Not used because all code is handled via RTOS tasks. This function simply deletes itself.
 */
void loop()
{
  vTaskDelete(NULL); ///< Delete this task.
}

/**
 * @brief RTOS task for event listening.
 *
 * @param parameter Pointer to the RTOS queue structure (cast from void*).
 */
void listener(void *parameter)
{
  TickType_t lastWake = xTaskGetTickCount();
  for (;;)
  {
    debugln("Hello Listener Task...");
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(cfg::LOOP_INTERVAL_TESTING));
  }
}

/**
 * @brief RTOS task for event handling.
 *
 * @param parameter Pointer to the RTOS queue structure (cast from void*).
 */
void handler(void *parameter)
{
  TickType_t lastWake = xTaskGetTickCount();
  for (;;)
  {
    debugln("Hello Handler Task...");
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(cfg::LOOP_INTERVAL_TESTING));
  }
}
