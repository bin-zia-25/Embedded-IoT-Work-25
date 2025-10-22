#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// --- Pin configuration ---
#define DHTPIN 14
#define DHTTYPE DHT11
#define LDR_PIN 36

#define SDA_PIN 21
#define SCL_PIN 22

// --- OLED setup ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- DHT sensor setup ---
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 with DHT22 + LDR + OLED");

  // Initialize I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();

  // Initialize DHT
  dht.begin();
  delay(1000);
}

void loop() {
  // --- Read Sensors ---
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int adcValue = analogRead(LDR_PIN);
  float voltage = (adcValue / 4095.0) * 3.3;

  // Check sensor read errors
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Error reading DHT22 sensor!");
    return;
  }

  // --- Serial Monitor Output ---
  Serial.printf("Temp: %.1f °C | Humidity: %.1f %% | LDR ADC: %d | Voltage: %.2f V\n",
                temperature, humidity, adcValue, voltage);

  // --- OLED Display Output ---
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ESP32 IoT Monitor");

  display.setCursor(0, 14);
  display.print("Temp: ");
  display.print(temperature, 1);
  display.println(" C");

  display.setCursor(0, 28);
  display.print("Hum:  ");
  display.print(humidity, 1);
  display.println(" %");

  display.setCursor(0, 42);
  display.print("Light intensity: ");
  display.print(adcValue);
  display.display();

  delay(2000);
}
