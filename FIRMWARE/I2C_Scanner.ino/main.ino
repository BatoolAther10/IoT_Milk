#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <math.h>

// ===========================================================================
// CONFIGURATION
// ===========================================================================

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* MQTT_BROKER = "broker.emqx.io";
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC = "milk/purity/data";

#define COW_ID "C001"

#define I2C_SDA        21
#define I2C_SCL        22
#define ONE_WIRE_PIN   4
#define MQ135_PIN      34
#define OLED_ADDR      0x3C
#define START_BUTTON   0
#define GREEN_LED      32
#define RED_LED        33
#define BUZZER_PIN     25

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64

#define PH_CHANNEL     0
#define TDS_CHANNEL    1
#define MQ135_CHANNEL  2

#define MQ135_VREF     3.3
#define ADC_RESOLUTION 4095.0
#define MQ135_LOAD_RESISTOR 22.0

// ===========================================================================
// CALIBRATION PARAMETERS
// ===========================================================================

float CAL_VOLTAGE_PH7 = 2.50;
float CAL_VOLTAGE_PH4 = 3.00;
const float TEMP_REFERENCE = 25.0;
float MQ135_R0 = 76.63;

struct Range {
    float lo, hi;
};

Range REF_PH      = {6.60, 6.80};
Range REF_TDS     = {3000, 8000};
Range REF_TEMP    = {2.0, 10.0};
Range REF_GAS     = {10, 100};

// ===========================================================================
// OBJECTS
// ===========================================================================

Adafruit_ADS1115 ads;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature ds18b20(&oneWire);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ===========================================================================
// STATE MACHINE
// ===========================================================================

enum SystemState {
    STATE_IDLE,
    STATE_TDS_STAB,
    STATE_NH3_SLOPE,
    STATE_PROCESSING,
    STATE_RESULT
};

SystemState currentState = STATE_IDLE;

// ===========================================================================
// TIMING VARIABLES
// ===========================================================================

unsigned long lastMillis = 0;
unsigned long stateStartTime = 0;

#define TDS_SAMPLES 15
float tdsValues[TDS_SAMPLES];
int tdsSampleIndex = 0;

#define NH3_SAMPLES 12
float nh3Values[NH3_SAMPLES];
int nh3SampleIndex = 0;
unsigned long nh3LastSampleTime = 0;

#define MEDIAN_SAMPLES 10
float tdsBuffer[MEDIAN_SAMPLES];

// ===========================================================================
// SENSOR DATA STRUCTURE
// ===========================================================================

struct SensorData {
    float ph;
    float tds;
    float tempC;
    float nh3PPM;
    float tdsStableTime;
    float nh3Slope;
    bool isPure;
    String adulterant;
};

SensorData sensorData;

// ===========================================================================
// BUTTON DEBOUNCE
// ===========================================================================

unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 200;
bool lastButtonState = HIGH;

// ===========================================================================
// SENSOR READ FUNCTIONS
// ===========================================================================

float readPH() {
    int16_t raw = ads.readADC_SingleEnded(PH_CHANNEL);
    float voltage = ads.computeVolts(raw);
    float slope = (7.0 - 4.0) / (CAL_VOLTAGE_PH7 - CAL_VOLTAGE_PH4);
    float ph = 7.0 + slope * (voltage - CAL_VOLTAGE_PH7);
    if (ph < 0) ph = 0;
    if (ph > 14) ph = 14;
    return ph;
}

float readTDSVoltage() {
    int16_t raw = ads.readADC_SingleEnded(TDS_CHANNEL);
    return ads.computeVolts(raw);
}

float voltageToTDS(float voltage, float tempC) {
    float compensationCoeff = 1.0 + 0.02 * (tempC - TEMP_REFERENCE);
    float compVoltage = voltage / compensationCoeff;
    float tds = (133.42 * pow(compVoltage, 3) - 255.86 * pow(compVoltage, 2) + 857.39 * compVoltage) * 0.5;
    if (tds < 0) tds = 0;
    if (tds > 50000) tds = 50000;
    return tds;
}

float readTemperature() {
    ds18b20.requestTemperatures();
    float t = ds18b20.getTempCByIndex(0);
    if (t == DEVICE_DISCONNECTED_C) return 25.0;
    return t;
}

float readNH3() {
    int raw = analogRead(MQ135_PIN);
    float voltage = raw * (MQ135_VREF / ADC_RESOLUTION);
    if (voltage <= 0.01) voltage = 0.01;
    float rs = ((MQ135_VREF * MQ135_LOAD_RESISTOR) / voltage) - MQ135_LOAD_RESISTOR;
    float ratio = rs / MQ135_R0;
    float ppm = 116.6020682 * pow(ratio, -2.769034857);
    if (ppm < 0 || isnan(ppm)) ppm = 0;
    if (ppm > 1000) ppm = 1000;
    return ppm;
}

// ===========================================================================
// MEDIAN FILTER
// ===========================================================================

float medianFilter(float* buffer, int size) {
    float sorted[MEDIAN_SAMPLES];
    for (int i = 0; i < size; i++) sorted[i] = buffer[i];
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (sorted[j] > sorted[j + 1]) {
                float temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }
        }
    }
    return sorted[size / 2];
}

float readTDSMedian() {
    for (int i = 0; i < MEDIAN_SAMPLES; i++) {
        float voltage = readTDSVoltage();
        tdsBuffer[i] = voltageToTDS(voltage, sensorData.tempC);
        delay(10);
    }
    return medianFilter(tdsBuffer, MEDIAN_SAMPLES);
}

// ===========================================================================
// LINEAR REGRESSION
// ===========================================================================

float calculateSlope(float* values, int count, float timeStep) {
    if (count < 2) return 0;
    float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    for (int i = 0; i < count; i++) {
        float x = i * timeStep;
        float y = values[i];
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }
    float n = count;
    return (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
}

// ===========================================================================
// STATE MACHINE FUNCTIONS
// ===========================================================================

void startTesting() {
    currentState = STATE_TDS_STAB;
    stateStartTime = millis();
    tdsSampleIndex = 0;
    nh3SampleIndex = 0;
    sensorData.tempC = readTemperature();
    Serial.println("=== STARTING TEST CYCLE ===");
    updateOLED();
}

void processTDSStabilization() {
    unsigned long elapsed = millis() - stateStartTime;
    int seconds = elapsed / 1000;
    
    if (elapsed >= (tdsSampleIndex * 1000) && tdsSampleIndex < TDS_SAMPLES) {
        float tds = readTDSMedian();
        tdsValues[tdsSampleIndex] = tds;
        tdsSampleIndex++;
        Serial.print("TDS Sample ");
        Serial.print(tdsSampleIndex);
        Serial.print(": ");
        Serial.print(tds);
        Serial.println(" ppm");
        updateOLED();
    }
    
    if (tdsSampleIndex >= TDS_SAMPLES) {
        float finalValue = 0;
        for (int i = TDS_SAMPLES - 5; i < TDS_SAMPLES; i++) {
            finalValue += tdsValues[i];
        }
        finalValue /= 5;
        float targetValue = finalValue * 0.9;
        sensorData.tdsStableTime = 15;
        for (int i = 0; i < TDS_SAMPLES; i++) {
            if (tdsValues[i] >= targetValue) {
                sensorData.tdsStableTime = i;
                break;
            }
        }
        sensorData.tds = finalValue;
        
        Serial.println("TDS Stabilization Complete:");
        Serial.print("  Final TDS: ");
        Serial.print(sensorData.tds);
        Serial.println(" ppm");
        Serial.print("  Time to 90%: ");
        Serial.print(sensorData.tdsStableTime);
        Serial.println(" seconds");
        
        currentState = STATE_NH3_SLOPE;
        stateStartTime = millis();
        nh3LastSampleTime = millis();
        nh3SampleIndex = 0;
        Serial.println("Phase 2: NH3 Slope Measurement (120 seconds)");
        updateOLED();
    }
}

void processNH3Slope() {
    unsigned long elapsed = millis() - stateStartTime;
    
    if (elapsed - (nh3SampleIndex * 10000) >= 10000 && nh3SampleIndex < NH3_SAMPLES) {
        float nh3 = readNH3();
        nh3Values[nh3SampleIndex] = nh3;
        nh3SampleIndex++;
        Serial.print("NH3 Sample ");
        Serial.print(nh3SampleIndex);
        Serial.print(": ");
        Serial.print(nh3);
        Serial.println(" ppm");
        updateOLED();
    }
    
    if (nh3SampleIndex >= NH3_SAMPLES) {
        sensorData.nh3Slope = calculateSlope(nh3Values, NH3_SAMPLES, 10.0);
        sensorData.nh3PPM = nh3Values[NH3_SAMPLES - 1];
        
        Serial.println("NH3 Slope Measurement Complete:");
        Serial.print("  Final NH3: ");
        Serial.print(sensorData.nh3PPM);
        Serial.println(" ppm");
        Serial.print("  Slope: ");
        Serial.print(sensorData.nh3Slope);
        Serial.println(" ppm/second");
        
        currentState = STATE_PROCESSING;
        calculateResults();
        updateOLED();
        updateIndicators();
        publishMQTT();
        currentState = STATE_RESULT;
    }
}

void calculateResults() {
    sensorData.ph = readPH();
    
    float phScore = 100.0 - abnormality(sensorData.ph, REF_PH, 1.0);
    float tdsScore = 100.0 - abnormality(sensorData.tds, REF_TDS, 5000);
    float tempScore = 100.0 - abnormality(sensorData.tempC, REF_TEMP, 10.0);
    float gasScore = 100.0 - abnormality(sensorData.nh3PPM, REF_GAS, 200);
    
    float purity = (phScore * 0.35) + (tdsScore * 0.35) + (gasScore * 0.20) + (tempScore * 0.10);
    sensorData.isPure = (purity >= 60);
    
    if (!sensorData.isPure) {
        sensorData.adulterant = detectAdulterant(sensorData);
    } else {
        sensorData.adulterant = "PURE";
    }
    
    Serial.println("=== RESULTS ===");
    Serial.print("Purity: ");
    Serial.print(purity);
    Serial.println("%");
    Serial.print("Status: ");
    Serial.println(sensorData.isPure ? "PURE" : "ADULTERATED");
    Serial.print("Adulterant: ");
    Serial.println(sensorData.adulterant);
}

float abnormality(float value, Range r, float tolerance) {
    if (value >= r.lo && value <= r.hi) return 0.0;
    float dist = (value < r.lo) ? (r.lo - value) : (value - r.hi);
    float score = (dist / tolerance) * 100.0;
    if (score > 100) score = 100;
    if (score < 0) score = 0;
    return score;
}

String detectAdulterant(SensorData d) {
    struct AdulterantPattern {
        float phMin, phMax;
        float tdsMin, tdsMax;
        float tempMin, tempMax;
        float gasMin, gasMax;
        String name;
    };
    
    AdulterantPattern patterns[] = {
        {6.50, 6.70, 0, 3000, 2.0, 10.0, 10, 100, "Water"},
        {6.55, 6.75, 8000, 15000, 2.0, 10.0, 10, 100, "Starch"},
        {6.80, 7.20, 10000, 20000, 2.0, 10.0, 10, 100, "Urea"},
        {7.00, 8.50, 12000, 25000, 2.0, 10.0, 100, 300, "Detergent"},
        {6.50, 6.80, 20000, 40000, 2.0, 10.0, 10, 100, "Salt"},
        {5.50, 6.50, 3000, 8000, 2.0, 10.0, 200, 500, "Formalin"}
    };
    
    int maxScore = 0;
    String detected = "Unknown";
    
    for (int i = 0; i < 6; i++) {
        int score = 0;
        if (d.ph >= patterns[i].phMin && d.ph <= patterns[i].phMax) score += 25;
        if (d.tds >= patterns[i].tdsMin && d.tds <= patterns[i].tdsMax) score += 25;
        if (d.tempC >= patterns[i].tempMin && d.tempC <= patterns[i].tempMax) score += 25;
        if (d.nh3PPM >= patterns[i].gasMin && d.nh3PPM <= patterns[i].gasMax) score += 25;
        if (score > maxScore) {
            maxScore = score;
            detected = patterns[i].name;
        }
    }
    return (maxScore > 50) ? detected : "Unknown";
}

// ===========================================================================
// MQTT PUBLISHING
// ===========================================================================

void publishMQTT() {
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    
    StaticJsonDocument<512> doc;
    doc["cow_id"] = COW_ID;
    doc["timestamp"] = millis();
    doc["ph"] = sensorData.ph;
    doc["tds"] = sensorData.tds;
    doc["temperature"] = sensorData.tempC;
    doc["nh3"] = sensorData.nh3PPM;
    doc["tds_stable_time"] = sensorData.tdsStableTime;
    doc["nh3_slope"] = sensorData.nh3Slope;
    doc["is_pure"] = sensorData.isPure;
    doc["adulterant"] = sensorData.adulterant;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    if (mqttClient.publish(MQTT_TOPIC, jsonString.c_str())) {
        Serial.println("✅ MQTT Published successfully");
        Serial.println(jsonString);
    } else {
        Serial.println("❌ MQTT Publish failed");
    }
}

// ===========================================================================
// OLED DISPLAY
// ===========================================================================

void updateOLED() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    
    switch (currentState) {
        case STATE_IDLE:
            display.setCursor(0, 0);
            display.println("MILK ANALYZER");
            display.println("v2.0");
            display.setCursor(0, 30);
            display.println("Press START");
            display.println("to test");
            break;
            
        case STATE_TDS_STAB: {
            display.setCursor(0, 0);
            display.println("TDS Stabilization");
            display.print("Sample: ");
            display.print(tdsSampleIndex);
            display.println("/15");
            display.print("Time: ");
            display.print((millis() - stateStartTime) / 1000);
            display.println("s");
            if (tdsSampleIndex > 0) {
                display.print("TDS: ");
                display.print(tdsValues[tdsSampleIndex - 1], 0);
                display.println(" ppm");
            }
            break;
        }
        
        case STATE_NH3_SLOPE: {
            display.setCursor(0, 0);
            display.println("NH3 Slope Test");
            display.print("Sample: ");
            display.print(nh3SampleIndex);
            display.println("/12");
            display.print("Time: ");
            display.print((millis() - stateStartTime) / 1000);
            display.println("/120s");
            if (nh3SampleIndex > 0) {
                display.print("NH3: ");
                display.print(nh3Values[nh3SampleIndex - 1], 0);
                display.println(" ppm");
            }
            break;
        }
        
        case STATE_PROCESSING:
            display.setCursor(0, 0);
            display.println("Processing...");
            break;
            
        case STATE_RESULT:
            display.setCursor(0, 0);
            display.println("=== RESULT ===");
            display.print("Sample #");
            display.println(COW_ID);
            
            display.print("TDS: ");
            display.print(sensorData.tds, 0);
            display.println(" ppm");
            
            display.print("pH: ");
            display.println(sensorData.ph, 2);
            
            display.print("NH3: ");
            display.print(sensorData.nh3PPM, 0);
            display.println(" ppm");
            
            display.println();
            if (sensorData.isPure) {
                display.println("PURE");
            } else {
                display.print(sensorData.adulterant);
                display.println(" DETECTED");
            }
            break;
    }
    display.display();
}

// ===========================================================================
// LED AND BUZZER INDICATORS
// ===========================================================================

void updateIndicators() {
    if (currentState == STATE_RESULT) {
        if (sensorData.isPure) {
            digitalWrite(GREEN_LED, HIGH);
            digitalWrite(RED_LED, LOW);
            tone(BUZZER_PIN, 2000, 500);
        } else {
            digitalWrite(GREEN_LED, LOW);
            digitalWrite(RED_LED, HIGH);
            tone(BUZZER_PIN, 1000, 1000);
            delay(1000);
            tone(BUZZER_PIN, 500, 1000);
        }
    } else {
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(RED_LED, LOW);
        noTone(BUZZER_PIN);
    }
}

// ===========================================================================
// BUTTON HANDLING
// ===========================================================================

bool isButtonPressed() {
    bool currentState = digitalRead(START_BUTTON);
    if (currentState == LOW && lastButtonState == HIGH) {
        unsigned long currentTime = millis();
        if (currentTime - lastButtonPress > DEBOUNCE_DELAY) {
            lastButtonPress = currentTime;
            lastButtonState = currentState;
            return true;
        }
    }
    lastButtonState = currentState;
    return false;
}

// ===========================================================================
// WIFI AND MQTT
// ===========================================================================

void connectWiFi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" ✅");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println(" ❌");
    }
}

void connectMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Connecting to MQTT...");
        if (mqttClient.connect("MilkAnalyzer")) {
            Serial.println(" ✅");
        } else {
            Serial.print(" ❌, retrying in 5s...");
            delay(5000);
        }
    }
}

// ===========================================================================
// SETUP
// ===========================================================================

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("═══════════════════════════════════════════════════════");
    Serial.println("  SMART MILK PURITY ANALYZER v2.0");
    Serial.println("═══════════════════════════════════════════════════════");
    
    pinMode(START_BUTTON, INPUT_PULLUP);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
    
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);
    
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("❌ OLED not detected!");
    } else {
        Serial.println("✅ OLED initialized");
    }
    
    if (!ads.begin(0x48)) {
        Serial.println("❌ ADS1115 not detected!");
    } else {
        ads.setGain(GAIN_TWOTHIRDS);
        Serial.println("✅ ADS1115 initialized");
    }
    
    ds18b20.begin();
    if (ds18b20.getDeviceCount() > 0) {
        Serial.print("✅ DS18B20 initialized - ");
        Serial.print(ds18b20.getDeviceCount());
        Serial.println(" device(s)");
    } else {
        Serial.println("⚠️ DS18B20 not detected!");
    }
    
    analogReadResolution(12);
    analogSetPinAttenuation(MQ135_PIN, ADC_11db);
    Serial.println("✅ MQ135 initialized");
    
    connectWiFi();
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    connectMQTT();
    
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(8, 5);
    display.println("MILK");
    display.setCursor(8, 28);
    display.println("TESTER");
    display.setTextSize(1);
    display.setCursor(8, 52);
    display.println(COW_ID);
    display.display();
    
    Serial.println("✅ System Ready - Press START button");
    updateOLED();
}

// ===========================================================================
// MAIN LOOP
// ===========================================================================

void loop() {
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    mqttClient.loop();
    
    if (isButtonPressed() && currentState == STATE_IDLE) {
        startTesting();
    }
    
    switch (currentState) {
        case STATE_IDLE:
            break;
        case STATE_TDS_STAB:
            processTDSStabilization();
            break;
        case STATE_NH3_SLOPE:
            processNH3Slope();
            break;
        case STATE_PROCESSING:
            break;
        case STATE_RESULT:
            break;
    }
    delay(10);
}
