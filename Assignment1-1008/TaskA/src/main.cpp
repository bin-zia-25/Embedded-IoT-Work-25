// Title: Use One Button to cycle through LED Modes
// Name:Ahmad Bin Zia
// Roll no: 23-NTU-CS-1008

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pins setup:
#define LED1 15
#define LED2 16
#define LED3 17
#define BTN1 25
#define BTN2 26

int mode = 0;
unsigned long previousMillis = 0;
int ledState = 0;
int fadeValue = 0;
int fadeDir = 1;

// Debounce variables
unsigned long lastDebounceTime1 = 0;
unsigned long lastDebounceTime2 = 0;
const unsigned long debounceDelay = 200;

// Function declaration
void showMode();

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);

  // Display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 25);
  display.print("Mode: 1 (All OFF)");
  display.display();
}

void loop() {
  unsigned long currentMillis = millis();

  // --- Button 1 (Cycle mode) ---
  if (digitalRead(BTN1) == LOW && (currentMillis - lastDebounceTime1) > debounceDelay) {
    lastDebounceTime1 = currentMillis;
    mode = (mode + 1) % 4;  // cycle through 0-3
    showMode();
  }

  // --- Button 2 (Reset to OFF) ---
  if (digitalRead(BTN2) == LOW && (currentMillis - lastDebounceTime2) > debounceDelay) {
    lastDebounceTime2 = currentMillis;
    mode = 0;
    showMode();
  }

  switch (mode) {
    case 0: // All OFF
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, LOW);
      break;

    case 1: // Alternate blink (LEDs take turns)
      if (currentMillis - previousMillis >= 400) {
        previousMillis = currentMillis;
        ledState = (ledState + 1) % 3;
        digitalWrite(LED1, ledState == 0 ? HIGH : LOW);
        digitalWrite(LED2, ledState == 1 ? HIGH : LOW);
        digitalWrite(LED3, ledState == 2 ? HIGH : LOW);
      }
      break;

    case 2: // All ON
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, HIGH);
      digitalWrite(LED3, HIGH);
      break;

    case 3: // PWM Fade (all fade together)
      analogWrite(LED1, fadeValue);
      analogWrite(LED2, fadeValue);
      analogWrite(LED3, fadeValue);
      fadeValue += fadeDir * 5;
      if (fadeValue <= 0 || fadeValue >= 255) fadeDir *= -1;
      delay(30); // okay here, not affecting buttons anymore
      break;
  }
}

void showMode() {
  display.clearDisplay();
  display.setCursor(10, 25);
  switch (mode) {
    case 0: display.print("Mode: 1 (All OFF)"); break;
    case 1: display.print("Mode: 2 (Alternate Blink)"); break;
    case 2: display.print("Mode: 3 (All ON)"); break;
    case 3: display.print("Mode: 4 (PWM Fade)"); break;
  }
  display.display();
}
