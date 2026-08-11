/**
 * M2 - Day 5: Show progress timers and final classification.
 */
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define LED_GREEN 4
#define LED_RED 5
#define BUZZER 2

void setup() {
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) while(1);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("OLED Test");
  display.display();
}

void loop() {
  // Simulate progress
  for (int i = 1; i <= 15; i++) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.printf("TDS: %d/15s\n", i);
    display.display();
    delay(1000);
  }
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Result: PURE");
  display.println("Conf: 92%");
  display.display();
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);
  tone(BUZZER, 2000, 200);
  delay(3000);
  digitalWrite(LED_GREEN, LOW);
}
