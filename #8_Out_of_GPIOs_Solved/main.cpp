/**
 * MIT License
 *
 * @brief Main program for ESP32-based project.
 *
 * @file main.cpp
 * @author Little Man Builds (Darren Osborne)
 * @date 2025-08-01
 * @copyright Copyright (c) 2025 Little Man Builds
 */

#include <AppConfig.h>
#include <ButtonPublisher/ButtonPublisher.h>
#include <ControlCore/ControlCore.h>
#include <McpPublisher/McpPublisher.h>
#include <PowerDriveHandler/PowerDriveHandler.h>
#include <RcPublisher/RcPublisher.h>

#include <Buses.h>
#include <Debugging.h>
#include <I2c.h>
#include <Tasks.h>

/**
 * @brief RTOS task handles (for diagnostics/control).
 */
namespace app::task
{
  inline TaskHandle_t AuthorityRouter_h = nullptr;
  inline TaskHandle_t ControlCore_h = nullptr;
  inline TaskHandle_t PowerDriveHandler_h = nullptr;
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
  buses::init();

  // ---- Publishers (self-owned tasks via SnapshotRTOS) ---- //
  static McpPublisher mcp{buses::mcp()};
  mcp.begin(); ///< Must begin first (config/button depend on MCP samples).

  static ButtonPublisher bp{buses::mcp(), buses::buttons(), cfg::tick::LOOP_MS, cfg::safety::stale::BTN_MS};
  static RcPublisher rcp{buses::rc(), cfg::tick::LOOP_MS};

  bp.begin();
  rcp.begin();

  // ---- Core managers (explicit FreeRTOS tasks) ---- //
  static ControlCore cc{buses::buttons(), buses::rc(), buses::authority(), buses::control()};
  static PowerDriveHandler pdh{driveMotor, buses::control()};

  utils::tasks::start_task(ControlCore::task, cfg::task::ControlCore, app::task::ControlCore_h, &cc);
  utils::tasks::start_task(PowerDriveHandler::task, cfg::task::PowerDriveHandler, app::task::PowerDriveHandler_h, &pdh);
}

/**
 * @brief Main Arduino loop.
 * @note Not used because all code is handled via RTOS tasks. This function simply deletes itself.
 */
void loop()
{
  vTaskDelete(NULL); ///< Delete this task.
}