# M2 Firmware – ESP32 Milk Adulteration Tester

## Hardware Setup
1. Connect sensors according to the pinout in `M2_Firmware.ino`.
2. Ensure ADS1115 address is `0x48` and OLED is `0x3C`.
3. Power ESP32 via USB (5V).

## Dependencies (install via Arduino Library Manager)
- Adafruit ADS1X15
- OneWire
- DallasTemperature
- Adafruit SSD1306
- Adafruit GFX
- PubSubClient
- ArduinoJson (version 6)

## Configuration
Edit these lines in `M2_Firmware.ino`:
```cpp
const char* WIFI_SSID     = "YourWiFiSSID";
const char* WIFI_PASSWORD = "YourWiFiPassword";
const char* MQTT_BROKER   = "test.mosquitto.org";  // or your broker IP
