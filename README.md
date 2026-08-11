# 🥛 MilkGuard – IoT Milk Adulteration Detection System

**An ESP32-powered, AI-driven device for rapid detection of 6 milk contaminants (Water, Urea, Salt, Sugar, Starch, and Spoilage) using pH, TDS, and NH₃ sensors.**

[![Platform](https://img.shields.io/badge/platform-ESP32-blue)](https://www.espressif.com/)
[![Framework](https://img.shields.io/badge/framework-Arduino-red)](https://www.arduino.cc/)
[![Python](https://img.shields.io/badge/python-3.8+-green)](https://www.python.org/)
[![License](https://img.shields.io/badge/license-MIT-lightgrey)](LICENSE)

---

## 📖 Table of Contents

* [Overview](#-overview)
* [Features](#-features)
* [Hardware Requirements](#-hardware-requirements)
* [System Architecture](#-system-architecture)
* [Pinout & Wiring](#-pinout--wiring)
* [Software Setup](#-software-setup)
* [Repository Structure](#-repository-structure)
* [Installation & Flashing](#-installation--flashing)
* [Usage Guide](#-usage-guide)
* [Performance Results](#-performance-results)
* [Team Contributions](#-team-contributions)

---

## 📋 Overview

**MilkGuard** is a low-cost, portable IoT device built to screen raw milk for common adulterants in under **3 minutes**.

The system combines:

* **ESP32 Microcontroller** for sensor acquisition and real-time control.
* **ADS1115** 16-bit ADC for precise analog readings (pH, TDS, NH₃).
* **Rule-based Decision Tree** (ported to both Python and C++) for instant classification.
* **MQTT Protocol** for wireless data streaming to a cloud dashboard.

The device achieves **96.7% accuracy** on a 30-sample test set, with a Limit of Detection (LOD) as low as **2%** (Salt) and **3%** (Water).

---

## 🚀 Features

| Feature                     | Description                                                             |
| :-------------------------- | :---------------------------------------------------------------------- |
| **7-Class Classification**  | PURE, WATER, UREA, SALT, SUGAR, STARCH, SPOILED                         |
| **Non-Blocking Firmware**   | Uses `millis()` timers – never hangs during the 120-second test cycle   |
| **Median Filtering**        | Removes sensor noise (10 samples per reading) for stable TDS values     |
| **Kinetic Analysis**        | Calculates NH₃ slope over 2 minutes to detect Urea                      |
| **TDS Stabilisation Logic** | Measures time to reach 90% of final TDS to detect Starch                |
| **Auto MQTT Reconnect**     | Automatically reconnects to WiFi/Broker if the signal drops             |
| **Visual & Audio Alerts**   | OLED progress bars, Green (Pure) / Red (Adulterated) LEDs, and a Buzzer |
| **Modular Python Scripts**  | Separate files for thresholds, classification, stats, and LOD           |

---

## 🔧 Hardware Requirements

### Bill of Materials (BOM)

| Component                     | Quantity | Specification                                    |
| :---------------------------- | :------: | :----------------------------------------------- |
| **ESP32 NodeMCU** (30-pin)    |     1    | Dual-core, WiFi + Bluetooth                      |
| **ADS1115 Module**            |     1    | 16-bit ADC, I2C (address `0x48`)                 |
| **pH Sensor Kit**             |     1    | Analog output, BNC connector                     |
| **TDS Sensor**                |     1    | Analog, Indian manufacturer                      |
| **DS18B20 Probe**             |     1    | Waterproof, 1-Wire digital temperature sensor    |
| **MQ-135 Module**             |     1    | Gas sensor (NH₃ detection)                       |
| **OLED Display**              |     1    | 0.96 inch, 128x64, I2C (SSD1306, address `0x3C`) |
| **Push Button**               |     1    | Normally open, pull-up to GPIO0                  |
| **LEDs (Red & Green)**        |     2    | 5mm, with 220Ω resistors                         |
| **Passive Buzzer**            |     1    | 5V                                               |
| **Breadboard + Jumper Wires** |   1 set  | MB102 840-point, FF wires                        |

---

## 🧠 System Architecture

```text
[Physical Milk Sample]
        │
        ▼
┌───────────────────┐      I2C      ┌─────────────┐
│   Sensors         │──────────────▶│   ADS1115   │
│  - pH (A0)        │               │   (ADC)     │
│  - TDS (A1)       │               └──────┬──────┘
│  - MQ135 (A2)     │                      │
└───────────────────┘                      │
        │ (1-Wire)                         │
        ▼                                  ▼
┌──────────────────────────────────────────────────────┐
│   ESP32 (M2 Firmware)                                │
│  - Reads sensors every second/10 seconds             │
│  - Median filter + TDS 90% stabilisation time       │
│  - Computes NH₃ slope over 120 seconds              │
│  - Runs local C++ decision tree                     │
│  - Publishes JSON via MQTT                           │
└──────────────────┬───────────────────────────────────┘
                   │ MQTT
                   ▼
┌──────────────────────────────────────────────────────┐
│   PC / Cloud (M1 AI Logic)                            │
│  - Python decision tree (classifier.py)              │
│  - Validation & Statistical analysis (stats)         │
│  - Confusion Matrix / F1 / LOD                       │
└──────────────────┬───────────────────────────────────┘
                   │
                   ▼
┌──────────────────────────────────────────────────────┐
│   Dashboard (M4) & OLED Display                      │
│  - Shows "PURE" / "ADULTERATED" with confidence     │
└──────────────────────────────────────────────────────┘
```

---

## 📌 Pinout & Wiring

|  ESP32 GPIO | Connected To               | Notes                           |
| :---------: | :------------------------- | :------------------------------ |
|  **GPIO 0** | Push Button (START)        | Internal pull-up enabled        |
|  **GPIO 2** | Passive Buzzer             | Active low for tone             |
|  **GPIO 4** | LED (Green)                | Pure milk indicator             |
|  **GPIO 5** | LED (Red)                  | Adulterated indicator           |
| **GPIO 15** | DS18B20 (Data)             | 4.7kΩ pull-up resistor required |
| **GPIO 21** | I2C SDA                    | OLED & ADS1115                  |
| **GPIO 22** | I2C SCL                    | OLED & ADS1115                  |
|   **3.3V**  | VCC (ADS1115, OLED, MQ135) | Power                           |
|    **5V**   | VCC (TDS, pH, DS18B20)     | Some sensors require 5V         |
|   **GND**   | All grounds                | Common ground                   |

### ADS1115 Channel Mapping

* **A0** → pH Sensor
* **A1** → TDS Sensor
* **A2** → MQ-135 (NH₃) Sensor

---

## 💻 Software Setup

### 1. Arduino IDE (for ESP32)

* Install **Arduino IDE** (v2.x or 1.8.x).
* Add ESP32 board URL:

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

* Install the following libraries via **Library Manager**:

  * `Adafruit ADS1X15`
  * `OneWire`
  * `DallasTemperature`
  * `Adafruit SSD1306`
  * `Adafruit GFX`
  * `PubSubClient`
  * `ArduinoJson` (version 6)

### 2. Python Environment (for M1 scripts)

* Ensure Python 3.8+ is installed.
* No external dependencies required – all scripts use the **standard library** (`csv`, `math`, `collections`, etc.).
* *(Optional)* For advanced ML, install `scikit-learn` and `pandas` if you want to run extra benchmarks.

---

## 📁 Repository Structure

```text
MilkGuard/
├── firmware/                        # Member 2 (ESP32 Code)
│   ├── M2_Firmware.ino              # 🏆 MAIN PRODUCTION FIRMWARE
│   ├── I2C_Scanner.ino              # Day 1: Debug I2C addresses
│   ├── TDS_Stabilization_Test.ino   # Day 3: Test median filter & 90% time
│   ├── NH3_Slope_Test.ino            # Day 4: Test 2-min NH3 slope
│   ├── OLED_Display_Test.ino         # Day 5: Display & LED test
│   ├── MQTT_Reconnect_Test.ino       # Day 6: WiFi/MQTT auto-reconnect
│   ├── NonBlocking_Timers.ino        # Day 7: millis() example
│   └── README_M2.md                  # Firmware flashing guide
│
├── ai_logic/                         # Member 1 (Python Code)
│   ├── thresholds_config.py          # Day 1: Literature thresholds
│   ├── classifier.py                 # Day 2: Decision tree function
│   ├── validation.py                 # Day 3: Unit tests (8 samples)
│   ├── batch_tests.py                # Days 5-6: 18 & 30 test batches
│   ├── stats_analysis.py             # Day 7: Confusion matrix & metrics
│   ├── lod_analysis.py               # Day 8: Limit of Detection
│   └── run_all.py                    # Master script to run everything
│
├── reports/                          # Member 1 (Documentation)
│   ├── day9_results_discussion.txt   # Results & Discussion section
│   └── day10_abstract_conclusion.txt # Abstract & Conclusion
│
├── docs/
│   └── wiring_diagram.png            # Visual circuit schematic
│
└── README.md                         # This file
```

---

## ⚙️ Installation & Flashing

### Step 1: Clone the Repository

```bash
git clone https://github.com/yourusername/MilkGuard.git
cd MilkGuard
```

### Step 2: Wire the Hardware

Follow the **Pinout & Wiring** table above.

Double-check:

* **I2C pull-ups** are usually on the modules (or enable them on the ESP32).
* The **DS18B20** requires a 4.7kΩ resistor between VCC and Data.

### Step 3: Test I2C Communication

* Open `firmware/I2C_Scanner.ino` in Arduino IDE.
* Select **Board**: `ESP32 Dev Module` and the correct **COM Port**.
* Upload and open **Serial Monitor** (115200 baud).
* Expected output:

```text
Device found at 0x48
Device found at 0x3C
```

### Step 4: Flash the Main Firmware

* Open `firmware/M2_Firmware.ino`.
* Edit the WiFi credentials and MQTT broker:

```cpp
const char* WIFI_SSID     = "Your_SSID";
const char* WIFI_PASSWORD = "Your_Password";
const char* MQTT_BROKER   = "test.mosquitto.org";
```

* Upload the code to the ESP32.
* Press the **START button** (GPIO 0) to begin the test cycle.

### Step 5: Run Python AI Logic

Navigate to the `ai_logic/` folder and run:

```bash
cd ai_logic
python run_all.py
```

This executes validation, batch tests, generates the confusion matrix, and computes LOD values.

---

## 🧪 Usage Guide

### Normal Operation (Standalone)

1. **Power on** the ESP32. The OLED displays *"Milk Tester v2.0 – Press START"*.
2. **Dip the sensors** into a 50mL milk sample.
3. **Press the START button**.
4. The device performs:

   * **TDS Stabilisation**: 15 seconds.
   * **NH₃ Kinetics**: 120 seconds.
5. **Read the Result**:

   * OLED shows classification (e.g., `PURE` or `UREA DETECTED`) + Confidence %.
   * **Green LED** flashes for Pure.
   * **Red LED + Buzzer** indicates Adulterated.
6. Data is published via MQTT to topic `milk/sensor_data` as JSON:

```json
{
  "pH": 6.7,
  "TDS": 980,
  "NH3": 8.0,
  "temperature": 25.4,
  "tds_stable_time": 2.1,
  "nh3_slope": 0.02,
  "classification": "PURE",
  "confidence": 0.92
}
```

### Running the Python Suite

* Run `validation.py` to test the decision tree against 8 fixed samples.
* Run `batch_tests.py` to generate test result CSV files.
* Run `stats_analysis.py` to print the **Confusion Matrix**, **Sensitivity**, **Specificity**, and **F1-Scores**.

---

## 📊 Performance Results

| Metric                     | Value                     |
| :------------------------- | :------------------------ |
| **Overall Accuracy**       | **96.7%** (29/30 correct) |
| **Macro-average F1-Score** | **0.981**                 |
| **Test Cycle Duration**    | ~2 minutes 15 seconds     |

### Per-Class Limit of Detection (LOD)

| Adulterant |   LOD  |
| :--------- | :----: |
| Water      |   3%   |
| Urea       |   5%   |
| Salt       | **2%** |
| Sugar      |   8%   |
| Starch     |   4%   |

### Confusion Matrix (30 Tests)

```text
             PURE  WATER  UREA  SALT  SUGAR  STARCH  SPOILED
PURE           4      0     0     0      1       0        0
WATER          0      5     0     0      0       0        0
UREA           0      0     5     0      0       0        0
SALT           0      0     0     5      0       0        0
SUGAR          0      0     0     0      5       0        0
STARCH         0      0     0     0      0       5        0
SPOILED        0      0     0     0      0       0        5
```

> **Note:** The single misclassification (PURE → SUGAR) was due to TDS probe drift. This was fixed on Day 6 with a pH hysteresis rule in the code.

---

## 👥 Team Contributions

| Member | Role                           | Deliverables                                                                |
| :----: | :----------------------------- | :-------------------------------------------------------------------------- |
| **M1** | Team Leader, AI Logic, Floater | Python decision tree, validation, stats, LOD, report sections, coordination |
| **M2** | Firmware Engineer              | ESP32 code, TDS stabilisation loop, NH₃ slope, MQTT, OLED/LED control       |
| **M3** | Backend Developer              | Database schema, API endpoints, MQTT ingestion                              |
| **M4** | Dashboard Developer            | Real-time web dashboard, charts, classification history                     |
| **M5** | Hardware Engineer              | Sensor procurement, circuit assembly, physical sample preparation           |

---

## 📜 License

This project is open-source under the **MIT License**. Feel free to use, modify, and distribute it for educational and commercial purposes.

---

## 🙏 Acknowledgements

* FSSAI (India) for published milk quality standards.
* Adafruit & Espressif for hardware libraries.
* All team members for their dedication during the 10-day sprint.

---

**Made with ❤️ for safer dairy consumption.**
