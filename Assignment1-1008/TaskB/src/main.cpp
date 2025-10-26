// Title: Short Press - Long Press Detection using OLED
// Name:Ahmad Bin Zia
// Roll no: 23-NTU-CS-1008

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Pins Setup
#define LED1 15
#define LED2 16
#define LED3 17
#define BUTTON 25
#define BUZZER 12

unsigned long pressStartTime = 0;
bool buttonPressed = false;
int ledIndex = 0;
bool ledsState[3] = {false, false, false};

// Debounce variables
unsigned long lastButtonChange = 0;
const unsigned long debounceDelay = 50;

// Function declarations
void toggleNextLED();
void showMessage(String msg);

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  Serial.begin(9600);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Task B: 3 LED Control");
  display.display();
}

void loop() {
  bool currentState = (digitalRead(BUTTON) == LOW);
  unsigned long currentMillis = millis();

  // Debounce: only react if state is stable for debounceDelay ms
  static bool lastState = HIGH;

  if (currentState != lastState) {
    lastButtonChange = currentMillis;  // reset timer on change
  }

  if ((currentMillis - lastButtonChange) > debounceDelay) {
    // Stable state detected
    if (currentState && !buttonPressed) {
      pressStartTime = currentMillis;
      buttonPressed = true;
    }

    if (!currentState && buttonPressed) {
      unsigned long pressDuration = currentMillis - pressStartTime;
      buttonPressed = false;

      if (pressDuration < 1500) {
        // Short press → toggle next LED
        toggleNextLED();
        showMessage("Short Press: LED " + String(ledIndex + 1));
      } else {
        // Long press → buzzer tone
        tone(BUZZER, 1000, 500);
        showMessage("Long Press: Buzzer!");
      }
    }
  }

  lastState = currentState;
}

void toggleNextLED() {
  ledsState[ledIndex] = !ledsState[ledIndex];
  digitalWrite(LED1, ledsState[0]);
  digitalWrite(LED2, ledsState[1]);
  digitalWrite(LED3, ledsState[2]);

  ledIndex++;
  if (ledIndex > 2) ledIndex = 0;
}

void showMessage(String msg) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(msg);
  display.display();
}
