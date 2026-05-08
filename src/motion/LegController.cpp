#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "LegController.h"
#include "../Config.h"

// =================================================================
// PCA9685 board objects — one per physical board
// =================================================================

static Adafruit_PWMServoDriver pwmBack = Adafruit_PWMServoDriver(0x40);
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

void legSetup()
{
  Serial.println("LegController: setup");

  Wire.begin(22, 20); // SDA, SCL

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

void writeServoAngle(uint8_t bus, uint8_t channel, int angle)
{
  // Clamp angle to valid range
  if (angle < 0)
    angle = 0;
  if (angle > 180)
    angle = 180;

  // Convert angle (0–180) to PCA9685 ticks
  int ticks = map(angle, 0, 180, PULSE_MIN_TICKS, PULSE_MAX_TICKS);

  // Send to the correct board
  if (bus == 0x40)
  {
    pwmBack.setPWM(channel, 0, ticks);
  }
  else if (bus == 0x60)
  {
    pwmFront.setPWM(channel, 0, ticks);
  }
  else
  {
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

void zeroAllServos()
{
  Serial.println("LegController: setting all servos to starting pose");

  // Hip servos (_H) → 90°
  writeServoAngle(leftFront_H.bus, leftFront_H.ch, 90);
  writeServoAngle(leftMiddle_H.bus, leftMiddle_H.ch, 90);
  writeServoAngle(leftBack_H.bus, leftBack_H.ch, 90);
  writeServoAngle(rightFront_H.bus, rightFront_H.ch, 90);
  writeServoAngle(rightMiddle_H.bus, rightMiddle_H.ch, 90);
  writeServoAngle(rightBack_H.bus, rightBack_H.ch, 90);
  delay(100);

  // Coxa servos (_C) → 90°
  writeServoAngle(leftFront_C.bus, leftFront_C.ch, 90);
  writeServoAngle(leftMiddle_C.bus, leftMiddle_C.ch, 90);
  writeServoAngle(leftBack_C.bus, leftBack_C.ch, 90);
  writeServoAngle(rightFront_C.bus, rightFront_C.ch, 90);
  writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, 90);
  writeServoAngle(rightBack_C.bus, rightBack_C.ch, 90);
  delay(100);

  // Femur servos (_F) → 0°
  writeServoAngle(leftFront_F.bus, leftFront_F.ch, 10);
  writeServoAngle(leftMiddle_F.bus, leftMiddle_F.ch, 10);
  writeServoAngle(leftBack_F.bus, leftBack_F.ch, 10);
  writeServoAngle(rightFront_F.bus, rightFront_F.ch, 10);
  writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, 10);
  writeServoAngle(rightBack_F.bus, rightBack_F.ch, 10);
  delay(100);

  Serial.println("LegController: starting pose set");
}

void walkingIDLE()
{
  Serial.println("LegController: setting all servos to walking pose");

  // J1 servos (_H) → 90°
  writeServoAngle(leftFront_H.bus, leftFront_H.ch, 90);
  writeServoAngle(leftMiddle_H.bus, leftMiddle_H.ch, 90);
  writeServoAngle(leftBack_H.bus, leftBack_H.ch, 90);
  writeServoAngle(rightFront_H.bus, rightFront_H.ch, 90);
  writeServoAngle(rightMiddle_H.bus, rightMiddle_H.ch, 90);
  writeServoAngle(rightBack_H.bus, rightBack_H.ch, 90);
  delay(100);

  // J2 servos (_C) → 50°
  writeServoAngle(leftFront_C.bus, leftFront_C.ch, 110);
  writeServoAngle(leftMiddle_C.bus, leftMiddle_C.ch, 110);
  writeServoAngle(leftBack_C.bus, leftBack_C.ch, 110);
  writeServoAngle(rightFront_C.bus, rightFront_C.ch, 110);
  writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, 110);
  writeServoAngle(rightBack_C.bus, rightBack_C.ch, 110);
  delay(100);

  // J3 servos (_F) → 60°
  writeServoAngle(leftFront_F.bus, leftFront_F.ch, 40);
  writeServoAngle(leftMiddle_F.bus, leftMiddle_F.ch, 40);
  writeServoAngle(leftBack_F.bus, leftBack_F.ch, 40);
  writeServoAngle(rightFront_F.bus, rightFront_F.ch, 40);
  writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, 40);
  writeServoAngle(rightBack_F.bus, rightBack_F.ch, 40);
  delay(100);

  Serial.println("LegController: starting pose set");
}

void seedingMoveForward()
{
  const int leftStart = 90;
  const int rightStart = 90;
  const int leftTarget = 120;
  const int rightTarget = 60;
  const int stepDelayMs = 10;

  int leftStep = (leftStart < leftTarget) ? 1 : -1;
  int rightStep = (rightStart < rightTarget) ? 1 : -1;

  int leftAngle = leftStart;
  int rightAngle = rightStart;

  while (leftAngle != leftTarget || rightAngle != rightTarget)
  {
    if (leftAngle != leftTarget)
      leftAngle += leftStep;
    if (rightAngle != rightTarget)
      rightAngle += rightStep;

    writeServoAngle(leftFront_H.bus, leftFront_H.ch, leftAngle);
    writeServoAngle(leftMiddle_H.bus, leftMiddle_H.ch, leftAngle);
    writeServoAngle(leftBack_H.bus, leftBack_H.ch, leftAngle);
    writeServoAngle(rightFront_H.bus, rightFront_H.ch, rightAngle);
    writeServoAngle(rightMiddle_H.bus, rightMiddle_H.ch, rightAngle);
    writeServoAngle(rightBack_H.bus, rightBack_H.ch, rightAngle);

    delay(stepDelayMs);
  }
}

void sideStepRight()
{
  writeServoAngle(leftFront_C.bus, leftFront_C.ch, 75);
  writeServoAngle(leftBack_C.bus, leftBack_C.ch, 75);
  writeServoAngle(leftFront_F.bus, leftFront_F.ch, 30);
  writeServoAngle(leftBack_F.bus, leftBack_F.ch, 30);
  writeServoAngle(leftFront_H.bus, leftFront_H.ch, 60);
  writeServoAngle(leftBack_H.bus, leftBack_H.ch, 120);
  delay(1000);
  writeServoAngle(leftFront_F.bus, leftFront_F.ch, 70);
  writeServoAngle(leftBack_F.bus, leftBack_F.ch, 70);
  writeServoAngle(leftFront_C.bus, leftFront_C.ch, 150);
  writeServoAngle(leftBack_C.bus, leftBack_C.ch, 150);
  delay(1000);
  writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, 100);
  writeServoAngle(leftMiddle_C.bus, leftMiddle_C.ch, 70);
  writeServoAngle(leftMiddle_F.bus, leftMiddle_F.ch, 30);
  writeServoAngle(rightFront_C.bus, rightFront_C.ch, 70);
  writeServoAngle(rightBack_C.bus, rightBack_C.ch, 70);
  writeServoAngle(rightFront_F.bus, rightFront_F.ch, 30);
  writeServoAngle(rightBack_F.bus, rightBack_F.ch, 30);
  delay(1000);

  // Synchronized sweep:
  // leftFront_F   70  -> 10
  // leftFront_C   150 -> 90
  // rightMiddle_C 90  -> 150
  // rightMiddle_F 10  -> 70
  const int stepDelayMs = 20; // ~50% speed

  int lfF = 70;  // leftFront_F   start
  int lfC = 150; // leftFront_C   start
  int rmC = 90;  // rightMiddle_C start
  int rmF = 10;  // rightMiddle_F start

  const int lfF_end = 10;
  const int lfC_end = 90;
  const int rmC_end = 150;
  const int rmF_end = 70;

  while (lfF != lfF_end || lfC != lfC_end || rmC != rmC_end || rmF != rmF_end)
  {
    if (lfF != lfF_end)
      lfF--; // 70  -> 10
    if (lfC != lfC_end)
      lfC--; // 150 -> 90
    if (rmC != rmC_end)
      rmC++; // 90  -> 150
    if (rmF != rmF_end)
      rmF++; // 10  -> 70

    writeServoAngle(leftFront_F.bus, leftFront_F.ch, lfF);
    writeServoAngle(leftBack_F.bus, leftBack_F.ch, lfF);
    writeServoAngle(leftFront_C.bus, leftFront_C.ch, lfC);
    writeServoAngle(leftBack_C.bus, leftBack_C.ch, lfC);
    writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, rmC);
    writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, rmF);

    delay(stepDelayMs);
  }

  delay(1000);

  writeServoAngle(leftMiddle_C.bus, leftMiddle_C.ch, 100);
  writeServoAngle(leftMiddle_F.bus, leftMiddle_F.ch, 10);
  writeServoAngle(rightFront_C.bus, rightFront_C.ch, 100);
  writeServoAngle(rightBack_C.bus, rightBack_C.ch, 100);
  delay(1000);

  writeServoAngle(leftBack_C.bus, leftBack_C.ch, 100);
  writeServoAngle(leftFront_C.bus, leftFront_C.ch, 100);
  writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, 70);
  delay(500);
  writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, 30);
  delay(1000);
  writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, 100);
  writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, 10);
  delay(1000);
}

void move70mm()
{
  writeServoAngle(leftFront_C.bus, leftFront_C.ch, 110);
  writeServoAngle(leftBack_C.bus, leftBack_C.ch, 110);
  writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, 110);
  delay(2000);
  writeServoAngle(leftFront_H.bus, leftFront_H.ch, 110);
  writeServoAngle(leftBack_H.bus, leftBack_H.ch, 110);
  writeServoAngle(rightMiddle_H.bus, rightMiddle_H.ch, 80);
  delay(2000);
  writeServoAngle(leftFront_C.bus, leftFront_C.ch, 130);
  writeServoAngle(leftBack_C.bus, leftBack_C.ch, 130);
  writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, 130);
  delay(2000);
  writeServoAngle(rightFront_C.bus, rightFront_C.ch, 90);
  writeServoAngle(rightBack_C.bus, rightBack_C.ch, 90);
  writeServoAngle(leftMiddle_C.bus, leftMiddle_C.ch, 90);
  delay(2000);
  writeServoAngle(leftFront_H.bus, leftFront_H.ch, 80);
  writeServoAngle(leftBack_H.bus, leftBack_H.ch, 80);
  writeServoAngle(rightMiddle_H.bus, rightMiddle_H.ch, 110);

  writeServoAngle(rightFront_H.bus, rightFront_H.ch, 80);
  writeServoAngle(rightBack_H.bus, rightBack_H.ch, 80);
  writeServoAngle(leftMiddle_H.bus, leftMiddle_H.ch, 110);
  delay(2000);

  writeServoAngle(rightFront_C.bus, rightFront_C.ch, 130);
  writeServoAngle(rightBack_C.bus, rightBack_C.ch, 130);
  writeServoAngle(leftMiddle_C.bus, leftMiddle_C.ch, 130);
}

// =================================================================
// Set all 6 coxa servos to a specific angle
// Used during drilling descent and lift.
// =================================================================

void setAllCoxas(int angle)
{
  writeServoAngle(leftFront_C.bus, leftFront_C.ch, angle);
  writeServoAngle(leftMiddle_C.bus, leftMiddle_C.ch, angle);
  writeServoAngle(leftBack_C.bus, leftBack_C.ch, angle);
  writeServoAngle(rightFront_C.bus, rightFront_C.ch, angle);
  writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, angle);
  writeServoAngle(rightBack_C.bus, rightBack_C.ch, angle);
}

// =================================================================
// Right-middle leg covering motion helpers
// =================================================================
//
// These move the right-middle leg's _C and _F servos during the
// covering sequence.

void moveRightMiddleCOnly(int cStart, int cEnd, int fHold, int stepDelayMs)
{
  if (cStart < cEnd)
  {
    for (int angle = cStart; angle <= cEnd; angle++)
    {
      writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, angle);
      writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, fHold);
      delay(stepDelayMs);
    }
  }
  else
  {
    for (int angle = cStart; angle >= cEnd; angle--)
    {
      writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, angle);
      writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, fHold);
      delay(stepDelayMs);
    }
  }
}

void moveRightMiddleCAndF(int cStart, int cEnd, int fStart, int fEnd,
                          int totalSteps, int stepDelayMs)
{
  if (totalSteps <= 0)
  {
    writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, cEnd);
    writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, fEnd);
    return;
  }

  for (int i = 0; i <= totalSteps; i++)
  {
    float t = (float)i / (float)totalSteps;

    int cAngle = cStart + (int)((cEnd - cStart) * t);
    int fAngle = fStart + (int)((fEnd - fStart) * t);

    writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, cAngle);
    writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, fAngle);

    delay(stepDelayMs);
  }
}