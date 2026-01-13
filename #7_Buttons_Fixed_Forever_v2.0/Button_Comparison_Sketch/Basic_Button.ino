const uint8_t buttonPin = 2;
const uint8_t ledPin = 13;

bool buttonState = false;
bool lastReading = false;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
}

void loop() {
  bool reading = !digitalRead(buttonPin);

  if (reading != lastReading) {
    lastDebounceTime = millis();
  }

  if (millis() - lastDebounceTime > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      digitalWrite(ledPin, buttonState);
    }
  }

  lastReading = reading;
}