/**
 * Sketch 02 — Mutex (apples-to-apples)
 *
 * Reader takes a short mutex lock to copy the whole Frame, then does its
 * simulated render outside the lock. Frames are always consistent (EPS = 0),
 * LAT_US reflects just lock+copy, and AGE_MS stays low since the writer
 * isn’t blocked by rendering.
 *
 * Plot series: EPS, LAT_US, AGE_MS, SCORE, INNING, CALL
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// ---- Common knobs (keep identical across all sketches) ---- //
static constexpr uint32_t PUB_HZ = 30;
static constexpr uint32_t UI_HZ = 30;
static constexpr uint32_t PUB_PERIOD_MS = 1000 / PUB_HZ;
static constexpr uint32_t UI_PERIOD_MS = 1000 / UI_HZ;

// Announcer makes the call from wall time (pure function).
static inline int umpire_call(uint32_t t_ms) { return (t_ms / 200) % 10; }

// Integrity check binds fields together so we can detect torn frames.
static inline int checksum(int score, int inning) { return (score * 31 + inning * 7) & 1023; }

// VS Code Serial Plotter line
static inline void plot(uint32_t eps, uint32_t lat_us, uint32_t age_ms,
                        int score, int inning, int call)
{
  // LAT_US = microseconds, AGE_MS = milliseconds, SCORE/CALL = integer runs.
  char b[160];
  int n = snprintf(b, sizeof(b),
                   ">EPS:%u,LAT_US:%u,AGE_MS:%u,SCORE:%d,INNING:%d,CALL:%d\r\n",
                   eps, lat_us, age_ms, score, inning, call);
  Serial.write(reinterpret_cast<const uint8_t *>(b), static_cast<size_t>(n));
}

// ---- Shared state protected by a mutex ---- //
struct Frame
{
  int score;         // What the board will show.
  int inning;        // 1..9 (cyclic).
  int chk;           // Checksum(score, inning).
  uint32_t stamp_ms; // Writer's millis() when published.
};

static Frame g_frame{0, 1, 0, 0};
static SemaphoreHandle_t g_mtx;

// Simulated UI render time (outside the lock to keep it apples-to-apples).
static constexpr uint32_t UI_RENDER_US = 420;

// ---- Writer: updates whole frame under the mutex ---- //
static void writer_task(void *)
{
  for (;;)
  {
    const uint32_t now_ms = millis();
    const int call = umpire_call(now_ms);
    const int inning = ((now_ms / 2000) % 9) + 1;

    xSemaphoreTake(g_mtx, portMAX_DELAY);
    g_frame.score = call; // Board follows the call.
    g_frame.inning = inning;
    g_frame.chk = checksum(g_frame.score, g_frame.inning);
    g_frame.stamp_ms = now_ms;
    xSemaphoreGive(g_mtx);

    vTaskDelay(pdMS_TO_TICKS(PUB_PERIOD_MS));
  }
}

// ---- Reader: lock briefly, copy, unlock, then render ---- //
static void ui_task(void *)
{
  Serial.begin(115200);
  delay(100);

  uint32_t eps_this_sec = 0;
  uint32_t last_sec_ms = millis();

  for (;;)
  {
    Frame f_copy;

    // Measure only the time to lock + copy (apples-to-apples vs SnapshotBus).
    const uint32_t t0_us = micros();
    xSemaphoreTake(g_mtx, portMAX_DELAY);
    f_copy = g_frame; // quick, consistent snapshot
    xSemaphoreGive(g_mtx);
    const uint32_t lat_us = micros() - t0_us;

    // Do the simulated render *after* timing, still outside the lock.
    delayMicroseconds(UI_RENDER_US);

    // EPS: should remain 0 for mutex-protected copy.
    if (f_copy.chk != checksum(f_copy.score, f_copy.inning))
      ++eps_this_sec;

    // AGE_MS: how old the snapshot is when we plotted it.
    const uint32_t now_ms = millis();
    const uint32_t age_ms = (now_ms >= f_copy.stamp_ms) ? (now_ms - f_copy.stamp_ms) : 0U;

    // CALL is derived independently from the live clock, like a real announcer.
    const int call_now = umpire_call(now_ms);

    plot(eps_this_sec, lat_us, age_ms, f_copy.score, f_copy.inning, call_now);

    vTaskDelay(pdMS_TO_TICKS(UI_PERIOD_MS));
  }
}

void setup()
{
  g_mtx = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(writer_task, "writer", 4096, nullptr, 3, nullptr, 0);
  xTaskCreatePinnedToCore(ui_task, "ui", 4096, nullptr, 2, nullptr, 1);
}

void loop() { vTaskDelete(nullptr); }
