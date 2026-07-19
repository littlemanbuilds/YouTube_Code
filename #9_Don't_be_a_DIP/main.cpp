/**
 * MIT License
 *
 * @brief Main program for ESP32-based project.
 *
 * @file main.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date Created: 2025-08-01
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#include <AppConfig.h>
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
   * @brief Demo/debug task that prints decoded configuration changes.
   * @note ConfigPublisher owns the real work: MCP state -> SwitchBank -> ConfigBus.
   *       This task only observes ConfigBus and prints changes for the Serial Monitor.
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
 * @brief RTOS task handles (for diagnostics/control).
 */
namespace app::task
{
  inline TaskHandle_t ConfigDebug_h = nullptr; ///< Temporary task for debugging.
  inline TaskHandle_t ControlCore_h = nullptr;
  inline TaskHandle_t PowerDriveHandler_h = nullptr;
  inline TaskHandle_t StackWatch_h = nullptr;
}

void setup()
{
  // ---- Start serial monitor ---- //
  Serial.begin(115200);
  delay(200);

  // ---- Initialize devices ---- //
  hal::i2c::begin();

  // ---- Motor ---- //
  static Motor driveMotor;
  MotorMCPWMConfig hw{};
  hw.rpwm_pin = cfg::motor::RPWM_PIN;
  hw.lpwm_pin = cfg::motor::LPWM_PIN;
  hw.en_pin = cfg::motor::EN_PIN;
  driveMotor.setup(hw);

  // ---- Pre-initialize bus ---- //
  // Force creation of all function-local static SnapshotBus instances before any tasks start.
  // This avoids first-call initialization guard paths inside task context.
  buses::init();

  // ---- Publishers (self-owned tasks via SnapshotRTOS) ---- //
  static McpPublisher mcp{buses::mcp()};
  mcp.begin(); ///< Must begin first (config/button depend on MCP samples).

  static ConfigPublisher cp{buses::mcp(), buses::config(), cfg::tick::LOOP_MS, cfg::safety::stale::CFG_MS};
  static ButtonPublisher bp{buses::mcp(), buses::buttons(), cfg::tick::LOOP_MS, cfg::safety::stale::BTN_MS};
  static RcPublisher rcp{buses::rc(), cfg::tick::LOOP_MS};

  cp.begin(); ///< Publishes decoded DIP/gear configuration.
  bp.begin();
  rcp.begin();

  // ---- Demo/config output ---- //
  if constexpr (cfg::display::CONFIG_DEBUG)
  {
    ConfigBus &config_bus = buses::config();
    utils::tasks::start_task(configDebugTask, cfg::task::ConfigDebug,
                             app::task::ConfigDebug_h, &config_bus);
  }

  // ---- Core managers (explicit FreeRTOS tasks) ---- //
  static ControlCore cc{buses::buttons(), buses::rc(), buses::authority(), buses::control()};
  static PowerDriveHandler pdh{driveMotor, buses::control()};

  utils::tasks::start_task(ControlCore::task, cfg::task::ControlCore, app::task::ControlCore_h, &cc);
  utils::tasks::start_task(PowerDriveHandler::task, cfg::task::PowerDriveHandler, app::task::PowerDriveHandler_h, &pdh);

  // ---- Diagnostics (periodic stack high-water marks + task overview) ---- //
  if constexpr (cfg::display::STACK_DEBUG)
  {
    static utils::stack_watch::Params sw{};
    sw.period_ms = 10000;
    utils::tasks::start_task(utils::stack_watch::task, cfg::task::StackWatch, app::task::StackWatch_h, &sw);
  }
}

/**
 * @brief Main Arduino loop.
 * @note Not used because all code is handled via RTOS tasks. This function simply deletes itself.
 */
void loop()
{
  vTaskDelete(NULL); ///< Delete this task.
}