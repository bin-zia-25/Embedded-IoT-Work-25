/****************************************************
 * ESP32 + DHT22 + MQTT (PUBLISHER ONLY)
 * Topic:
 *   home/lab1/temp
 *   home/lab1/hum
 ****************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// ---------- WiFi ----------
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

// ---------- MQTT ----------
const char* mqtt_server = " 10.35.230.62";  // Mosquitto / Cloud broker
const int   mqtt_port   = 1883;

// Topics
const char* TOPIC_HUM  = "pub1/lab2/hum";

// ---------- DHT ----------
#define DHTPIN  23
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ---------- MQTT Client ----------
WiFiClient espClient;
PubSubClient mqtt(espClient);

// ---------- Functions ----------
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqtt.connect("Hum_Pub")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqtt.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  dht.begin();
  connectWiFi();

  mqtt.setServer(mqtt_server, mqtt_port);
  connectMQTT();
}

void loop() {
  if (!mqtt.connected()) {
    connectMQTT();
  }
  mqtt.loop();

  float temperature = dht.readTemperature();
  float humidity    = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT read failed");
    delay(2000);
    return;
  }

  char tBuf[8], hBuf[8];

  dtostrf(humidity,    4, 2, hBuf);
  mqtt.publish(TOPIC_HUM,  hBuf);

  Serial.print(" C | Hum: ");
  Serial.print(hBuf);
  Serial.println(" %");

  delay(5000);  // publish every 5 seconds
}