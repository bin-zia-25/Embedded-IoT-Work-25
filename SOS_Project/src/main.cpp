#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- CONFIGURATION ---
#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER 1024 

#include <TinyGsmClient.h>
#include <TinyGPS++.h>

// --- USER SETTINGS ---
const char apn[]  = "jazzconnect"; 
const char user[] = "";
const char pass[] = "";
const char ownerNumber[] = "+923246739908s"; // <--- REPLACE WITH YOUR REAL NUMBER

// MIDDLEWARE SETTINGS (THINGSPEAK)
const char server[] = "api.thingspeak.com";
const char thingspeak_key[] = "XSWUSSV911D098SD"; 
const int port = 80; 

// --- PINS ---
#define SIM_TX      27
#define SIM_RX      26
#define GPS_RX      16
#define GPS_TX      17

// UI PINS
#define BTN_UP      32
#define BTN_DOWN    33
#define BTN_SEL     25
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// --- OBJECTS ---
HardwareSerial simSerial(1);
HardwareSerial gpsSerial(2);
TinyGsm modem(simSerial);
TinyGsmClient client(modem); 
TinyGPSPlus gps;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- GLOBAL VARIABLES ---
unsigned long lastUpload = 0;
unsigned long lastPoll = 0;
unsigned long lastUiUpdate = 0;
unsigned long lastStatsUpdate = 0;
unsigned long lastSMSCheck = 0;

// Button State for Long Press
unsigned long selPressedTime = 0;
bool selHandled = false;

// UI State
enum ScreenState { HOME, MENU, GPS_DETAIL, SYSTEM_INFO, MESSAGE };
ScreenState currentScreen = HOME;
int menuCursor = 0;
String messageText = ""; 
int cachedSignal = 0;
int cachedBattery = 0;

// --- PROTOTYPES ---
void handleButtons();
void drawUI();
void uploadLocation();
void checkIncomingSMS();
bool getLBS(float *lat, float *lng);
void triggerPanic();
void makeCall(String number);

void setup() {
  Serial.begin(115200);
  
  // Buttons
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SEL, INPUT_PULLUP);

  // 1. Init OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 failed"));
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,20);
  display.println(F("Booting System..."));
  display.display();

  // 2. Init Serials
  simSerial.begin(9600, SERIAL_8N1, SIM_RX, SIM_TX);
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  // 3. Init Modem
  display.println(F("Modem Init...")); display.display();
  modem.restart();
  delay(3000);
  
  // 4. Wait for Network Registration (CRITICAL for calls!)
  display.println(F("Wait Network...")); display.display();
  for (int i = 0; i < 30; i++) {
    simSerial.println("AT+CREG?");
    delay(500);
    String resp = "";
    while(simSerial.available()) resp += (char)simSerial.read();
    
    if (resp.indexOf("0,1") > 0 || resp.indexOf("0,5") > 0) {
      display.println(F("Network Ready!")); display.display();
      break;
    }
    delay(1000);
  }
  
  // 5. Configure Call Settings
  simSerial.println("AT+COLP=1"); // Connected line ID
  delay(300);
  simSerial.println("AT+CLIP=1"); // Calling line ID
  delay(300);
  simSerial.println("AT+CHFA=0"); // Audio to Mic/Headset
  delay(300);
  simSerial.println("AT+CLVL=100"); // Max Volume
  delay(300);
  
  // 6. Connect GPRS
  display.println(F("Connecting Jazz...")); display.display();
  if (!modem.gprsConnect(apn, user, pass)) {
    display.println(F("Net Fail (Retry Later)")); 
  } else {
    display.println(F("Net Connected!")); 
  }
  
  // 7. Config SMS
  modem.sendAT(GF("+CMGF=1")); // Text Mode
  modem.waitResponse();
  modem.sendAT(GF("+CNMI=0,0,0,0,0")); // No immediate notifications
  modem.waitResponse();

  display.display();
  delay(1000);
}

void loop() {
  // 1. ALWAYS Feed GPS
  while (gpsSerial.available() > 0) gps.encode(gpsSerial.read());

  // 2. UI Handling (Fast Loop)
  handleButtons();
  
  if (millis() - lastUiUpdate > 100) { 
    drawUI();
    lastUiUpdate = millis();
  }

  // 3. Background Stats (Every 10s)
  if (millis() - lastStatsUpdate > 10000) {
    cachedSignal = modem.getSignalQuality();
    cachedBattery = modem.getBattPercent();
    lastStatsUpdate = millis();
  }

  // 4. Auto Upload (Every 5 mins)
  // Only upload if NOT currently in a call/message screen
  if (currentScreen != MESSAGE && millis() - lastUpload > 300000) { 
    // Ensure GPRS is connected before upload
    if (!modem.isGprsConnected()) {
       modem.gprsConnect(apn, user, pass);
    }
    uploadLocation();
    lastUpload = millis();
  }
  
  // 5. Check SMS (Every 10s)
  // Only check if NOT currently in a call/message screen
  if (currentScreen != MESSAGE && millis() - lastSMSCheck > 10000) {
    checkIncomingSMS();
    lastSMSCheck = millis();
  }

  // 6. Reconnect GPRS Logic
  // If we are NOT in a call (MESSAGE screen) and GPRS dropped (due to a previous call), reconnect.
  if (currentScreen != MESSAGE && !modem.isGprsConnected() && millis() - lastUpload > 10000) {
      modem.gprsConnect(apn, user, pass);
  }
}

// ==========================================
//   UI & DISPLAY LOGIC
// ==========================================

void drawStatusBar() {
  display.fillRect(0, 0, 128, 10, WHITE); 
  display.setTextColor(BLACK, WHITE);
  display.setTextSize(1);
  display.setCursor(2, 1);
  display.print(F("Sig:")); display.print(cachedSignal);
  display.setCursor(50, 1);
  if (modem.isGprsConnected()) display.print("G"); else display.print("x");
  display.setCursor(85, 1);
  display.print(cachedBattery); display.print(F("%"));
  display.setTextColor(WHITE);
}

void drawUI() {
  display.clearDisplay();
  drawStatusBar();
  display.setCursor(0, 14); 

  switch (currentScreen) {
    case HOME:
      display.setTextSize(1);
      display.println(F("STATUS: TRACKING"));
      display.println(F("Hold SEL for PANIC"));
      display.println(F("---------------------"));
      display.setTextSize(2);
      if (gps.location.isValid()) {
        display.print(gps.location.lat(), 4);
        display.setCursor(0, 50);
        display.print(gps.location.lng(), 4);
      } else {
        display.println(F("NO GPS"));
        display.setTextSize(1);
        display.println(F("Searching Sky..."));
      }
      break;

    case MENU:
      display.setTextSize(1);
      display.println(F("MAIN MENU"));
      if(menuCursor == 0) display.print(F("> ")); else display.print(F("  ")); display.println(F("Panic Trigger"));
      if(menuCursor == 1) display.print(F("> ")); else display.print(F("  ")); display.println(F("Force Upload"));
      if(menuCursor == 2) display.print(F("> ")); else display.print(F("  ")); display.println(F("GPS Details"));
      if(menuCursor == 3) display.print(F("> ")); else display.print(F("  ")); display.println(F("System Info"));
      break;

    case GPS_DETAIL:
      display.setTextSize(1);
      display.println(F("SATELLITES"));
      display.print(F("Vis: ")); display.println(gps.satellites.value());
      display.print(F("Alt: ")); display.print(gps.altitude.meters()); display.println("m");
      display.print(F("Spd: ")); display.print(gps.speed.kmph()); display.println("km/h");
      display.println(F("\n[SELECT] Back"));
      break;
      
    case SYSTEM_INFO:
      display.println(F("SYSTEM INFO"));
      display.print(F("APN: ")); display.println(apn);
      display.print(F("Owner: ")); display.println(ownerNumber);
      display.println(F("\n[SELECT] Back"));
      break;

    case MESSAGE:
      display.setTextSize(1);
      display.println(F("ALERT:"));
      display.println(F("----------------"));
      display.println(messageText);
      break;
  }
  display.display();
}

void handleButtons() {
  static unsigned long lastDebounce = 0;
  
  // --- PANIC LONG PRESS LOGIC ---
  if (digitalRead(BTN_SEL) == LOW) {
    if (selPressedTime == 0) {
      selPressedTime = millis();
      selHandled = false;
    } else if (!selHandled && (millis() - selPressedTime > 5000)) {
      // 5 Seconds passed!
      selHandled = true;
      triggerPanic();
    }
  } else {
    // Button Released
    if (selPressedTime > 0 && !selHandled) {
      // Short Press Action
      if (millis() - lastDebounce > 200) {
        if (currentScreen == MENU) {
          switch (menuCursor) {
            case 0: triggerPanic(); break;
            case 1: 
              currentScreen = MESSAGE; messageText = "Uploading..."; drawUI();
              uploadLocation(); currentScreen = HOME; 
              break;
            case 2: currentScreen = GPS_DETAIL; break;
            case 3: currentScreen = SYSTEM_INFO; break;
          }
        } else if (currentScreen != HOME) {
          currentScreen = MENU;
        } else {
          currentScreen = MENU;
        }
        lastDebounce = millis();
      }
    }
    selPressedTime = 0;
  }

  // --- NAV BUTTONS ---
  if (millis() - lastDebounce > 200) {
    if (digitalRead(BTN_DOWN) == LOW) {
      if (currentScreen == MENU) { menuCursor++; if (menuCursor > 3) menuCursor = 0; }
      else if (currentScreen == HOME) currentScreen = MENU;
      lastDebounce = millis();
    }
    if (digitalRead(BTN_UP) == LOW) {
      if (currentScreen == MENU) { menuCursor--; if (menuCursor < 0) menuCursor = 3; }
      else if (currentScreen != HOME) currentScreen = HOME;
      lastDebounce = millis();
    }
  }
}

// ==========================================
//   LOGIC: PANIC & SMS
// ==========================================

void makeCall(String number) {
  // Disconnect GPRS (SIM800L can't do data + voice simultaneously)
  Serial.println("Disconnecting GPRS...");
  modem.gprsDisconnect();
  delay(2000); // Wait for full disconnect
  
  // Verify network registration
  Serial.println("Checking registration...");
  simSerial.println("AT+CREG?");
  delay(500);
  
  // Clear serial buffer
  while(simSerial.available()) simSerial.read();
  
  // Make call using RAW serial command (bypass TinyGSM wrapper)
  number.trim();
  number.replace("\"", ""); // Remove any quotes
  
  String callCmd = "ATD" + number + ";";
  Serial.println("Calling: " + callCmd);
  
  simSerial.println(callCmd); // Direct command to modem
  delay(500);
  
  // Read modem response
  String response = "";
  unsigned long timeout = millis();
  while (millis() - timeout < 3000) {
    if (simSerial.available()) {
      response += (char)simSerial.read();
    }
  }
  Serial.println("Modem response: " + response);
}

void triggerPanic() {
  currentScreen = MESSAGE;
  messageText = "Calling Owner...";
  drawUI();

  Serial.println(F("=== PANIC TRIGGERED ==="));
  makeCall(String(ownerNumber));
  
  delay(3000); // Wait for call to establish
  currentScreen = HOME;
}

void checkIncomingSMS() {
  modem.sendAT(GF("+CMGL=\"REC UNREAD\""));
  if (modem.waitResponse(1000, GF("+CMGL: ")) == 1) {
    int index = modem.stream.parseInt();
    modem.stream.readStringUntil('"'); // Skip
    modem.stream.readStringUntil('"'); // Skip status
    modem.stream.readStringUntil('"'); // Skip
    String senderNumber = modem.stream.readStringUntil('"');
    
    modem.stream.readStringUntil('\n'); // Skip remainder
    String message = modem.stream.readStringUntil('\n'); // Read body
    message.trim();
    
    Serial.print("SMS from: "); Serial.print(senderNumber);
    Serial.print(" - Message: "); Serial.println(message);
    message.toLowerCase();
    
    if (message.indexOf("hi") >= 0) {
      modem.sendSMS(senderNumber, "MENU:\n1.Loc\n2.Bat\n3.Call");
    } 
    else if (message.indexOf("1") >= 0) {
      float lat, lng;
      if(gps.location.isValid()) { 
        lat=gps.location.lat(); 
        lng=gps.location.lng(); 
      }
      else { 
        getLBS(&lat, &lng); 
      }
      String reply = "Loc: https://maps.google.com/?q=" + String(lat, 6) + "," + String(lng, 6);
      modem.sendSMS(senderNumber, reply);
    }
    else if (message.indexOf("2") >= 0) {
      modem.sendSMS(senderNumber, "Bat: " + String(cachedBattery) + "%");
    }
    else if (message.indexOf("3") >= 0) {
      Serial.println("SMS Call Request from: " + senderNumber);
      makeCall(senderNumber);
    }
    
    // Delete processed SMS
    modem.sendAT("+CMGD=" + String(index));
    modem.waitResponse();
  }
}

// ==========================================
//   NETWORKING (ThingSpeak Bridge)
// ==========================================

void uploadLocation() {
  float lat, lng;
  String type = "GPS";
  
  if (gps.location.isValid()) {
    lat = gps.location.lat();
    lng = gps.location.lng();
  } else {
    if (!getLBS(&lat, &lng)) {
      messageText = "Loc Failed!";
      return;
    }
    type = "LBS";
  }

  if (client.connect(server, port)) {
    // ThingSpeak Bridge URL (GET Request)
    String url = "/apps/thinghttp/send_request?api_key=" + String(thingspeak_key) + 
                 "&lat=" + String(lat, 6) + 
                 "&lng=" + String(lng, 6) + 
                 "&type=" + type + 
                 "&battery=" + String(cachedBattery);

    client.print("GET " + url + " HTTP/1.1\r\n");
    client.print("Host: " + String(server) + "\r\n");
    client.print("Connection: close\r\n\r\n");
    
    unsigned long timeout = millis();
    while (millis() - timeout < 5000) {
      if (client.available()) client.read();
    }
    client.stop();
    messageText = "Upload OK!";
    Serial.println("Location uploaded: " + String(lat, 6) + "," + String(lng, 6));
  } else {
    messageText = "Connect Fail!";
    Serial.println("ThingSpeak connection failed");
  }
}

bool getLBS(float *lat, float *lng) {
  modem.sendAT(GF("+CIPGSMLOC=1,1"));
  if (modem.waitResponse(10000, GF("+CIPGSMLOC:")) == 1) {
    int status = modem.stream.parseInt();
    if(status == 0) {
      *lng = modem.stream.parseFloat();
      *lat = modem.stream.parseFloat();
      return true;
    }
  }
  return false;
}