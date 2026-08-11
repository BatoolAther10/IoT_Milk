/**
 * M2 - Day 4: Read MQ135 every 10s for 120s, compute slope.
 */
#include <Adafruit_ADS1X15.h>
Adafruit_ADS1115 ads;

float nh3Readings[13];
int idx = 0;

float readNH3() {
  int16_t adc = ads.readADC_SingleEnded(2);
  float v = adc * 0.0001875;
  return v * 100.0;  // rough ppm
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  if (!ads.begin()) { Serial.println("ADS1115 not found!"); while(1); }
  Serial.println("NH3 Slope Test (120s)");
}

void loop() {
  Serial.println("Reading NH3 every 10s...");
  for (int i = 0; i <= 12; i++) {
    nh3Readings[i] = readNH3();
    Serial.printf("NH3[%d] = %.2f ppm\n", i, nh3Readings[i]);
    if (i < 12) delay(10000);
  }
  float slope = (nh3Readings[12] - nh3Readings[0]) / 120.0;
  Serial.printf("\nSlope = %.4f ppm/s\n", slope);
  while(1);
}
