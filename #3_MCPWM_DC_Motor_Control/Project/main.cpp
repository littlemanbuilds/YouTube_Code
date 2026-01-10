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
#include <Universal_Button.h>
#include <ESP32_MCPWM.h>

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

/**
 * @brief Task context passsed to the lisener RTOS task.
 */
struct ListenerContext
{
  Button *buttons{nullptr};
};

struct HandlerContext
{
  Motor *motor{nullptr};
};

/**
 * @brief Global instance used when creating the listener task.
 */
static ListenerContext listenerCtx{};
static HandlerContext handlerCtx{};

void setup()
{
  // Start serial debugging output.
  Serial.begin(115200); // Default baud rate of the ESP32.

  debugln("===== Startup =====");

  // ---- Button Setup ----
  const ButtonTimingConfig kTiming{cfg::BTN_DEBOUNCE_MS, cfg::BTN_SHORT_MS, cfg::BTN_LONG_MS};
  static Button buttons = makeButtons(kTiming);
  listenerCtx.buttons = &buttons;

  // ---- Motor Setup ----
  static Motor driveMotor;

  MotorMCPWMConfig hw{};
  hw.rpwm_pin = cfg::motor::RPWM_PIN;
  hw.lpwm_pin = cfg::motor::LPWM_PIN;
  hw.en_pin = cfg::motor::EN_PIN;

  driveMotor.setup(hw);
  handlerCtx.motor = &driveMotor;

  // RTOS Task Creation.
  configASSERT(xTaskCreatePinnedToCore(listener,       ///< Task function (must be void listener(void *)).
                                       "listener",     ///< Task name (for debugging).
                                       LISTENER_STACK, ///< Stack size (in bytes).
                                       &listenerCtx,   ///< Parameter passed to the task.
                                       PRI_LISTENER,   ///< Task Priority.
                                       &listener_t,    ///< Pointer to the task handle.
                                       0) == pdPASS);  ///< Core to pin task to (core 0).
  delay(50);
  configASSERT(xTaskCreatePinnedToCore(handler, "handler", HANDLER_STACK, &handlerCtx, PRI_HANDLER, &handler_t, 0) == pdPASS);
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
  auto *ctx = static_cast<ListenerContext *>(parameter);
  Button &btns = *ctx->buttons;

  TickType_t lastWake = xTaskGetTickCount();
  for (;;)
  {
    btns.update();

    /* if (btns.isPressed(ButtonIndex::TestButton1))
    {
      debugln("TestButton1 is currently pressed...");
    }
    else
    {
      debugln("No input detected...");
    } */

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(cfg::LOOP_INTERVAL_TEST_SHORT));
  }
}

/**
 * @brief RTOS task for event handling.
 *
 * @param parameter Pointer to the RTOS queue structure (cast from void*).
 */
void handler(void *parameter)
{
  auto *ctx = static_cast<HandlerContext *>(parameter);
  Motor &motor = *ctx->motor;

  constexpr float STEP_PCT = 2.0f; ///< % per step.
  constexpr int STEP_DELAY = 100;  ///< Time between steps (ms).
  constexpr int END_HOLD = 1000;   ///< Pause between movements (ms).
  Dir dir = Dir::CW;

  TickType_t lastWake = xTaskGetTickCount();
  for (;;)
  {
    // ---- Ramp up: 0% -> 100% ----
    for (float pct = 0.0f; pct <= 100.0f; pct += STEP_PCT)
    {
      motor.setSpeedPercent(pct, dir);
      debug("Speed %: ");
      debugln(pct, 1);
      vTaskDelay(pdMS_TO_TICKS(STEP_DELAY));
    }

    // ---- Hold at max ----
    debugln("Hold at max...");
    vTaskDelay(pdMS_TO_TICKS(END_HOLD));

    // ---- Ramp down: 100% -> 0% ----
    for (float pct = 100.0f; pct >= 0.0f; pct -= STEP_PCT)
    {
      motor.setSpeedPercent(pct, dir);
      debug("Speed %: ");
      debugln(pct, 1);
      vTaskDelay(pdMS_TO_TICKS(STEP_DELAY));
    }

    // ---- Pause at zero (neutral guard) ----
    debugln("Hold at max...");
    vTaskDelay(pdMS_TO_TICKS(END_HOLD));

    // ---- Reverse direction ----
    dir = (dir == Dir::CW) ? Dir::CCW : Dir::CW;

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(cfg::LOOP_INTERVAL_TEST_LONG));
  }
}
