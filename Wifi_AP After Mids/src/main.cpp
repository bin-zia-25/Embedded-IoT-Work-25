#include <WiFi.h>

// 1. Define the name and password for your new network
const char *ssid = "My_ESP32_Hotspot";
const char *password = "12345678"; // Password must be at least 8 characters

void setup() {
  Serial.begin(115200);

  // 2. Start the Access Point (AP)
  Serial.println("Setting up Access Point...");
  
  // formatting: WiFi.softAP(ssid, password)
  // If you want an open network (no password), use NULL instead of password
  WiFi.softAP(ssid, password);

  // 3. Print the IP address
  // The default IP for ESP32 AP mode is usually 192.168.4.1
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP Created! Connect to '");
  Serial.print(ssid);
  Serial.println("'");
  Serial.print("IP Address: ");
  Serial.println(IP);
}

void loop() {
  // 4. Check how many devices are connected to us
  int stations = WiFi.softAPgetStationNum();
  
  Serial.print("Devices connected: ");
  Serial.println(stations);
  
  delay(3000); // Check every 3 seconds
}