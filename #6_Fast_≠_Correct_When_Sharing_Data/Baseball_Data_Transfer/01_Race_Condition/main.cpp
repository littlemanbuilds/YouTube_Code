/**
 * Sketch 01 — Race condition (torn reads)
 *
 * Writer updates the Frame with no protection; reader copies it directly.
 * Fields are written at different times, so the reader can catch a “half-old,
 * half-new” frame → SCORE and CALL drift apart and EPS increments.
 *
 * Plot series: EPS, LAT_US, AGE_MS, SCORE, INNING, CALL
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>

// ---- Common knobs (identical across all sketches) ---- //
static constexpr uint32_t PUB_HZ = 30;
static constexpr uint32_t UI_HZ = 30;
static constexpr uint32_t PUB_PERIOD_MS = 1000 / PUB_HZ;
static constexpr uint32_t UI_PERIOD_MS = 1000 / UI_HZ;

static inline int umpire_call(uint32_t t_ms) { return (t_ms / 200) % 10; }
static inline int checksum(int score, int inning) { return (score * 31 + inning * 7) & 1023; }

static inline void plot(uint32_t eps, uint32_t lat_us, uint32_t age_ms,
                        int score, int inning, int call)
{
    char b[128];
    const int n = snprintf(b, sizeof(b),
                           ">EPS:%u,LAT_US:%u,AGE_MS:%u,SCORE:%d,INNING:%d,CALL:%d\r\n",
                           eps, lat_us, age_ms, score, inning, call);
    Serial.write((const uint8_t *)b, (size_t)n);
}

// Shared frame (no protection → torn reads possible).
struct Frame
{
    int score;
    int inning;
    int chk;
    uint32_t stamp_us;
};
static Frame g_frame{0, 1, 0, 0};

// Keep SCORE stale for most of the 50 ms period → visible mismatch vs CALL.
static constexpr uint32_t DELAY_SCORE_MS = 42; // try 45–48 for an even longer stale window

static void writer(void *)
{
    bool flip = false;
    for (;;)
    {
        const uint32_t ms = millis();
        const int call = umpire_call(ms);
        const int inning = ((ms / 2000) % 9) + 1;
        const int chk = checksum(call, inning);

        if (!flip)
        {
            g_frame.chk = chk; // “new truth” first.
            g_frame.inning = inning;
            vTaskDelay(pdMS_TO_TICKS(DELAY_SCORE_MS)); // Scoreboard still old.
            g_frame.score = call;                      // Finally flips.
        }
        else
        {
            g_frame.inning = inning;
            vTaskDelay(pdMS_TO_TICKS(DELAY_SCORE_MS));
            g_frame.chk = chk;
            g_frame.score = call;
        }
        g_frame.stamp_us = micros();

        const uint32_t used = DELAY_SCORE_MS;
        const uint32_t rest = (PUB_PERIOD_MS > used) ? (PUB_PERIOD_MS - used) : 1;
        vTaskDelay(pdMS_TO_TICKS(rest));
        flip = !flip;
    }
}

static void ui(void *)
{
    Serial.begin(115200);
    delay(100);
    uint32_t eps = 0, last_ms = millis();

    for (;;)
    {
        const uint32_t t0 = micros();
        const Frame f = g_frame; // Unsafe copy (may tear).
        const uint32_t lat_us = micros() - t0;

        const uint32_t now_us = micros();
        const uint32_t age_ms = (now_us >= f.stamp_us) ? (now_us - f.stamp_us) / 1000U : 0U;

        if (f.chk != checksum(f.score, f.inning))
            ++eps; // Torn frame detected.

        plot(eps, lat_us, age_ms, f.score, f.inning, umpire_call(millis()));

        if (millis() - last_ms >= 1000)
        {
            Serial.printf("[RACE] eps/s=%u\n", eps);
            eps = 0;
            last_ms = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(UI_PERIOD_MS));
    }
}

void setup()
{
    xTaskCreatePinnedToCore(writer, "w", 4096, nullptr, 3, nullptr, 0);
    xTaskCreatePinnedToCore(ui, "ui", 4096, nullptr, 2, nullptr, 1);
}
void loop() { vTaskDelete(nullptr); }
