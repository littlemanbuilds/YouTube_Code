#include <Arduino.h>

static constexpr uint8_t kButtonPin = 6;

static bool pressed = false;  // current sampled state (no debounce)

static inline void updateButton()
{
  pressed = (digitalRead(kButtonPin) == LOW);
}

static inline bool isPressed()
{
  return pressed;
}

void setup()
{
  Serial.begin(115200);
  delay(50);
  pinMode(kButtonPin, INPUT_PULLUP);
}

void loop()
{
  updateButton();

  if (isPressed())
  {
    Serial.println("TestButton is pressed!");
  }
  else
  {
    Serial.println("No input detected.");
  }

  delay(100);
}