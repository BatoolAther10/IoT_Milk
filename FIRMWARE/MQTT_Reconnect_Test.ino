/**
 * M2 - Day 6: WiFi/MQTT reconnection + button debounce.
 */
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "YourWiFiSSID";
const char* pass = "YourWiFiPassword";
const char* mqtt_server = "test.mosquitto.org";
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastReconnectAttempt = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received: ");
  for (int i=0; i<length; i++) Serial.print((char)payload[i]);
  Serial.println();
}

void reconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost, reconnecting...");
    WiFi.reconnect();
  }
  while (!client.connected()) {
    Serial.print("MQTT reconnect...");
    if (client.connect("ESP32_Client")) {
      Serial.println(" ✅");
      client.subscribe("test/topic");
    } else {
      Serial.print(" ❌ rc="); Serial.print(client.state());
      Serial.println(" retry in 5s");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(0, INPUT_PULLUP);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // Simple debounce test
  static unsigned long lastDebounce = 0;
  static int lastState = HIGH;
  int reading = digitalRead(0);
  if (reading != lastState) lastDebounce = millis();
  if ((millis() - lastDebounce) > 50) {
    if (reading == LOW) {
      Serial.println("Button pressed (debounced)");
      client.publish("test/topic", "Button pressed");
    }
  }
  lastState = reading;
  delay(10);
}
