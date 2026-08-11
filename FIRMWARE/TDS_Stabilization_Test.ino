/**
 * M2 - Day 3: Test TDS reading every 1s for 15s, median filter, 90% time.
 */
#include <Adafruit_ADS1X15.h>
Adafruit_ADS1115 ads;

float tdsReadings[15];
int idx = 0;

float medianFilter(float* arr, int n) {
  float sorted[n];
  for (int i = 0; i < n; i++) sorted[i] = arr[i];
  for (int i = 0; i < n-1; i++)
    for (int j = 0; j < n-i-1; j++)
      if (sorted[j] > sorted[j+1]) {
        float t = sorted[j]; sorted[j] = sorted[j+1]; sorted[j+1] = t;
      }
  return sorted[n/2];
}

float readTDS() {
  int16_t adc = ads.readADC_SingleEnded(1);
  float v = adc * 0.0001875;
  return (133.42*v*v*v - 255.86*v*v + 857.39*v) * 0.5;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  if (!ads.begin()) { Serial.println("ADS1115 not found!"); while(1); }
  Serial.println("TDS Stabilisation Test");
}

void loop() {
  Serial.println("Reading TDS for 15s...");
  for (int i = 0; i < 15; i++) {
    float buf[10];
    for (int j = 0; j < 10; j++) buf[j] = readTDS();
    tdsReadings[i] = medianFilter(buf, 10);
    Serial.printf("TDS[%d] = %.1f\n", i+1, tdsReadings[i]);
    delay(1000);
  }
  float finalTDS = 0;
  for (int i = 10; i < 15; i++) finalTDS += tdsReadings[i];
  finalTDS /= 5.0;
  float target90 = finalTDS * 0.90;
  int stableTime = 15;
  for (int i = 0; i < 15; i++) {
    if (tdsReadings[i] >= target90) { stableTime = i; break; }
  }
  Serial.printf("\nFinal TDS = %.1f ppm\n", finalTDS);
  Serial.printf("Time to reach 90%% (% .1f) = %d seconds\n", target90, stableTime);
  while(1);
}
