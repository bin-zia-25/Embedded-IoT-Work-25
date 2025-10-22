/* 
  Name: Ahmed Bin Zia
  Reg no.: 23-NTU-CS-FL-1008
  Week3_Home Task: Debounce Solution using Timer Interrupt
*/

#include <Arduino.h>

#define LED1 2
#define LED2 4
#define SWITCH1 12
#define SWITCH2 14

// LED states
volatile bool Led_State1 = LOW;
volatile bool Led_State2 = LOW;

// Debounce flags
volatile bool debounce1 = false;
volatile bool debounce2 = false;

// Timer objects
hw_timer_t *Timer1 = NULL;
hw_timer_t *Timer2 = NULL;

// ----------- Timer Interrupts -----------
void IRAM_ATTR Timer1_ISR() {
  debounce1 = false; // allow next button press
  timerStop(Timer1); // stop timer after delay
}

void IRAM_ATTR Timer2_ISR() {
  debounce2 = false;
  timerStop(Timer2);
}

// ----------- Switch Interrupts -----------
void IRAM_ATTR Switch1_ISR() {
  if (!debounce1) {
    debounce1 = true;
    Led_State1 = !Led_State1;
    digitalWrite(LED1, Led_State1);
    timerRestart(Timer1);
    timerStart(Timer1);
  }
}

void IRAM_ATTR Switch2_ISR() {
  if (!debounce2) {
    debounce2 = true;
    Led_State2 = !Led_State2;
    digitalWrite(LED2, Led_State2);
    timerRestart(Timer2);
    timerStart(Timer2);
  }
}

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(SWITCH1, INPUT_PULLUP);
  pinMode(SWITCH2, INPUT_PULLUP);

  // -------- Timer Configuration --------
  // timerBegin(timer number, divider, countUp)
  // 80 MHz / 80 = 1 MHz → 1 tick = 1 microsecond
  Timer1 = timerBegin(0, 80, true);
  Timer2 = timerBegin(1, 80, true);

  // Attach ISRs (true = edge triggered)
  timerAttachInterrupt(Timer1, &Timer1_ISR, true);
  timerAttachInterrupt(Timer2, &Timer2_ISR, true);

  // Set timers for 200ms (200,000 microseconds), no auto-reload
  timerAlarmWrite(Timer1, 200000, false);
  timerAlarmWrite(Timer2, 200000, false);

  // External interrupts for switches
  attachInterrupt(digitalPinToInterrupt(SWITCH1), Switch1_ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(SWITCH2), Switch2_ISR, FALLING);
}

void loop() {
  // nothing to do here
}
