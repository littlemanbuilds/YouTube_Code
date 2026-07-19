/**
 * MIT License
 *
 * @brief Wires the hardware, shared buses, and RTOS tasks into the running control system.
 *
 * @file main.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-08-01
 * @copyright Copyright (c) 2026 Little Man Builds
 */

#include <AppConfig.h>
#include <AuthorityRouter/AuthorityRouter.h>
#include <ButtonPublisher/ButtonPublisher.h>
#include <ConfigPublisher/ConfigPublisher.h>
#include <ControlCore/ControlCore.h>
#include <McpPublisher/McpPublisher.h>
#include <PowerDriveHandler/PowerDriveHandler.h>
#include <RcPublisher/RcPublisher.h>

#include <AppDisplay.h>
#include <Buses.h>
#include <ConfigDebug.h>
#include <Debugging.h>
#include <I2c.h>
#include <StackWatch.h>
#include <Tasks.h>

namespace
{
  /**
   * @brief Print mode and gear changes to the Serial Monitor.
   * @note ConfigPublisher turns MCP23017 switch readings into ConfigBus snapshots.
   *       This task only displays the result; it does not make control decisions.
   */
  void configDebugTask(void *ctx)
  {
    auto *bus = static_cast<ConfigBus *>(ctx);

    ConfigSnapshot prev{};
    bool has_prev = false;

    for (;;)
    {
      ConfigSnapshot cur{};

      if (bus->peek_into(cur))
      {
        utils::config_debug::logIfChanged(prev, has_prev, cur);
      }

      vTaskDelay(pdMS_TO_TICKS(250));
    }
  }
} ///< namespace

/**
 * @brief Task handles retained for diagnostics and task management.
 */
namespace app::task
{
  inline TaskHandle_t AuthorityRouter_h = nullptr;
  inline TaskHandle_t ConfigDebug_h = nullptr; ///< Configuration diagnostic task.
  inline TaskHandle_t ControlCore_h = nullptr;
  inline TaskHandle_t PowerDriveHandler_h = nullptr;
  inline TaskHandle_t StackWatch_h = nullptr;
}

void setup()
{
  // ---- Start the Serial Monitor ---- //
  Serial.begin(115200);
  delay(200);

  // ---- Bring up shared hardware ---- //
  hal::i2c::begin();

  // ---- Configure the motor output ---- //
  static Motor driveMotor;
  MotorMCPWMConfig hw{};
  hw.rpwm_pin = cfg::motor::RPWM_PIN;
  hw.lpwm_pin = cfg::motor::LPWM_PIN;
  hw.en_pin = cfg::motor::EN_PIN;
  driveMotor.setup(hw);

  // ---- Create the shared buses before tasks start ---- //
  // Keeping C++ static initialization here avoids two tasks trying to create the
  // same bus during concurrent startup.
  buses::init();

  // ---- Start the input publishers ---- //
  static McpPublisher mcp{buses::mcp()};
  mcp.begin(); // Start first: configuration and button input both depend on its MCP23017 samples.

  static ConfigPublisher cp{buses::mcp(), buses::config(), cfg::tick::LOOP_MS};
  static ButtonPublisher bp{buses::mcp(), buses::buttons(), cfg::tick::LOOP_MS};
  static RcPublisher rcp{buses::rc(), cfg::tick::LOOP_MS};

  cp.begin(); // Decode and publish the DIP mode and gear position.
  bp.begin();
  rcp.begin();

  // ---- Configuration diagnostics ---- //
  if constexpr (cfg::display::CONFIG_DEBUG)
  {
    ConfigBus &config_bus = buses::config();
    utils::tasks::start_task(configDebugTask, cfg::task::ConfigDebug,
                             app::task::ConfigDebug_h, &config_bus);
  }

  // ---- Start the decision and actuator tasks ---- //
  static AuthorityRouter auth(buses::rc(), buses::config(), buses::authority());
  static ControlCore cc{buses::buttons(), buses::rc(), buses::authority(), buses::control()};
  static PowerDriveHandler pdh{driveMotor, buses::control()};

  utils::tasks::start_task(AuthorityRouter::task, cfg::task::AuthorityRouter, app::task::AuthorityRouter_h, &auth);
  utils::tasks::start_task(ControlCore::task, cfg::task::ControlCore, app::task::ControlCore_h, &cc);
  utils::tasks::start_task(PowerDriveHandler::task, cfg::task::PowerDriveHandler, app::task::PowerDriveHandler_h, &pdh);

  // ---- Optional task and memory diagnostics ---- //
  if constexpr (cfg::display::STACK_DEBUG)
  {
    static utils::stack_watch::Params sw{};
    sw.period_ms = 10000;
    utils::tasks::start_task(utils::stack_watch::task, cfg::task::StackWatch, app::task::StackWatch_h, &sw);
  }
}

/**
 * @brief Remove the unused Arduino loop task.
 * @note All application work runs in dedicated FreeRTOS tasks, so deleting this
 *       task returns its scheduler resources.
 */
void loop()
{
  vTaskDelete(NULL); // Delete this task.
}
