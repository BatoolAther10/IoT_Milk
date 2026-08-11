/**
 * M2 - Day 7: Replace delay() with millis().
 * Blinks LED while measuring sensor without blocking.
 */
#define LED_BUILTIN 2

unsigned long prevBlink = 0;
unsigned long prevSensor = 0;
bool ledState = false;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  // Blink every 500ms (non‑blocking)
  if (millis() - prevBlink >= 500) {
    prevBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
  }

  // Simulate sensor read every 1s
  if (millis() - prevSensor >= 1000) {
    prevSensor = millis();
    Serial.printf("Sensor reading at %lu ms\n", millis());
  }
}
