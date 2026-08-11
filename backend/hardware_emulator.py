import json
import time
import paho.mqtt.client as mqtt

MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC = "m3_lab/sensor_readings"

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.connect(MQTT_BROKER, MQTT_PORT)

# Simulated hardware runs
test_runs = [
    {
        "session_id": "DEMO_SESS_PURE_01",
        "ph": 6.7,
        "tds": 820.0,
        "temperature": 25.0,
        "nh3": 1.8,
        "tds_stable_time": 7.5,
        "nh3_slope": 0.01,
        "concentration": "Pure Sample Test"
    },
    {
        "session_id": "DEMO_SESS_SALT_02",
        "ph": 6.5,
        "tds": 2600.0,
        "temperature": 24.8,
        "nh3": 2.2,
        "tds_stable_time": 8.1,
        "nh3_slope": 0.02,
        "concentration": "5% Salt Adulterated"
    },
    {
        "session_id": "DEMO_SESS_UREA_03",
        "ph": 7.3,
        "tds": 990.0,
        "temperature": 25.2,
        "nh3": 5.1,
        "tds_stable_time": 11.2,
        "nh3_slope": 0.62,
        "concentration": "5% Urea Adulterated"
    }
]

print("🚀 Hardware Emulator Running... Publishing live test payloads to MQTT!")

for run in test_runs:
    payload_str = json.dumps(run)
    print(f"\n📡 Publishing sample '{run['concentration']}' to {MQTT_TOPIC}...")
    client.publish(MQTT_TOPIC, payload_str, qos=1)
    time.sleep(10)  # Simulates 10 seconds between hardware test runs

print("\n✅ All demo test payloads published!")