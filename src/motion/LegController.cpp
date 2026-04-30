#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#include "LegController.h"
#include "../Config.h"

// =================================================================
// PCA9685 board objects — one per physical board
// =================================================================

static Adafruit_PWMServoDriver pwmBack  = Adafruit_PWMServoDriver(0x40);
static Adafruit_PWMServoDriver pwmFront = Adafruit_PWMServoDriver(0x60);

// PWM frequency for hobby servos
#define SERVO_FREQ_HZ 50

// PCA9685 tick range corresponding to 0° and 180°
// Standard hobby servo: 0.5ms pulse (~102 ticks) to 2.5ms pulse (~512 ticks)
#define PULSE_MIN_TICKS 102
#define PULSE_MAX_TICKS 512

// =================================================================
// Setup — initialize both boards
// =================================================================

void legSetup() {
  Serial.println("LegController: setup");
  
  Wire.begin(22, 20);  // SDA, SCL
  
  pwmBack.begin();
  pwmBack.setPWMFreq(SERVO_FREQ_HZ);
  
  pwmFront.begin();
  pwmFront.setPWMFreq(SERVO_FREQ_HZ);
  
  delay(100);
  
  Serial.println("LegController: both boards initialized");
}

// =================================================================
// Send one servo to a specific angle (0–180°)
// =================================================================

void writeServoAngle(uint8_t bus, uint8_t channel, int angle) {
  // Clamp angle to valid range
  if (angle < 0)   angle = 0;
  if (angle > 180) angle = 180;
  
  // Convert angle (0–180) to PCA9685 ticks
  int ticks = map(angle, 0, 180, PULSE_MIN_TICKS, PULSE_MAX_TICKS);
  
  // Send to the correct board
  if (bus == 0x40) {
    pwmBack.setPWM(channel, 0, ticks);
  } else if (bus == 0x60) {
    pwmFront.setPWM(channel, 0, ticks);
  } else {
    Serial.print("LegController: unknown bus 0x");
    Serial.println(bus, HEX);
  }
}

// =================================================================
// Move all 18 servos to starting pose
//   Hip (_H)   → 90°
//   Coxa (_C)  → 90°
//   Femur (_F) → 102°
// =================================================================

void zeroAllServos() {
  Serial.println("LegController: setting all servos to starting pose");
  
  // Hip servos (_H) → 90°
  writeServoAngle(leftFront_H.bus,   leftFront_H.ch,   90);
  writeServoAngle(leftMiddle_H.bus,  leftMiddle_H.ch,  90);
  writeServoAngle(leftBack_H.bus,    leftBack_H.ch,    90);
  writeServoAngle(rightfront_H.bus,  rightfront_H.ch,  90);
  writeServoAngle(rightMiddle_H.bus, rightMiddle_H.ch, 90);
  writeServoAngle(rightBack_H.bus,   rightBack_H.ch,   90);
  delay(100);
  
  // Coxa servos (_C) → 90°
  writeServoAngle(leftFront_C.bus,   leftFront_C.ch,   90);
  writeServoAngle(leftMiddle_C.bus,  leftMiddle_C.ch,  90);
  writeServoAngle(leftBack_C.bus,    leftBack_C.ch,    90);
  writeServoAngle(rightFront_C.bus,  rightFront_C.ch,  90);
  writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, 90);
  writeServoAngle(rightBack_C.bus,   rightBack_C.ch,   90);
  delay(100);
  
  // Femur servos (_F) → 102°
  writeServoAngle(leftFront_F.bus,   leftFront_F.ch,   0);
  writeServoAngle(leftMiddle_F.bus,  leftMiddle_F.ch,  0);
  writeServoAngle(leftBack_F.bus,    leftBack_F.ch,    0);
  writeServoAngle(rightFront_F.bus,  rightFront_F.ch,  0);
  writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, 0);
  writeServoAngle(rightBack_F.bus,   rightBack_F.ch,   0);
  delay(100);
  
  Serial.println("LegController: starting pose set");
}

// =================================================================
// Set all 6 coxa servos to a specific angle
// Used during drilling descent and lift.
// =================================================================

void setAllCoxas(int angle) {
  writeServoAngle(leftFront_C.bus,   leftFront_C.ch,   angle);
  writeServoAngle(leftMiddle_C.bus,  leftMiddle_C.ch,  angle);
  writeServoAngle(leftBack_C.bus,    leftBack_C.ch,    angle);
  writeServoAngle(rightFront_C.bus,  rightFront_C.ch,  angle);
  writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, angle);
  writeServoAngle(rightBack_C.bus,   rightBack_C.ch,   angle);
}

// =================================================================
// Right-middle leg covering motion helpers
// =================================================================
//
// These move the right-middle leg's _C and _F servos during the 
// covering sequence.

void moveRightMiddleCOnly(int cStart, int cEnd, int fHold, int stepDelayMs) {
  if (cStart < cEnd) {
    for (int angle = cStart; angle <= cEnd; angle++) {
      writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, angle);
      writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, fHold);
      delay(stepDelayMs);
    }
  } else {
    for (int angle = cStart; angle >= cEnd; angle--) {
      writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, angle);
      writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, fHold);
      delay(stepDelayMs);
    }
  }
}

void moveRightMiddleCAndF(int cStart, int cEnd, int fStart, int fEnd, 
                          int totalSteps, int stepDelayMs) {
  if (totalSteps <= 0) {
    writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, cEnd);
    writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, fEnd);
    return;
  }
  
  for (int i = 0; i <= totalSteps; i++) {
    float t = (float)i / (float)totalSteps;
    
    int cAngle = cStart + (int)((cEnd - cStart) * t);
    int fAngle = fStart + (int)((fEnd - fStart) * t);
    
    writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, cAngle);
    writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, fAngle);
    
    delay(stepDelayMs);
  }
}