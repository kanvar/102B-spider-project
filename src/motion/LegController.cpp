#include <Arduino.h>
#include <ESP32Servo.h>
#include "LegController.h"
#include "Config.h"

static Servo servo1, servo2, servo3;
static unsigned long lastGaitStep = 0;
static int gaitPhase = 0;

void legSetup() {
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo3.attach(SERVO3_PIN);
  servo1.write(0);
  servo2.write(0);
  servo3.write(0);
}

void resetGait() {
  gaitPhase = 0;
  lastGaitStep = 0;
}

// ========== serivce function --> IDLE ==========
void serviceIdle() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > IDLE_PRINT_MS) {
    Serial.println("State: IDLE - holding neutral pose");
    lastPrint = millis();
  }
  servo1.write(0);
  servo2.write(0);
  servo3.write(0);
}

// ========== serice function --> walking ==========
void serviceWalking() {
  if (millis() - lastGaitStep < GAIT_STEP_MS) return;
  lastGaitStep = millis();

  switch (gaitPhase) {
    case 0:
      Serial.println("State: WALKING - phase 0 (lift)");
      servo1.write(0); servo2.write(0); servo3.write(0);
      break;
    case 1:
      Serial.println("State: WALKING - phase 1 (swing forward)");
      servo1.write(45);
      break;
    case 2:
      Serial.println("State: WALKING - phase 2 (plant foot)");
      servo2.write(60); servo3.write(50);
      break;
    case 3:
      Serial.println("State: WALKING - phase 3 (retract)");
      servo1.write(0);
      break;
  }
  gaitPhase = (gaitPhase + 1) % 4;
}

// ========== serice function --> planting ==========
void servicePlanting() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > IDLE_PRINT_MS) {
    Serial.println("State: PLANTING - returning to 0");
    lastPrint = millis();
  }
  servo1.write(0);
  servo2.write(0);
  servo3.write(0);
}