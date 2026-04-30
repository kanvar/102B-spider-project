#pragma once
#include <stdint.h>

void legSetup();
void zeroAllServos();
void writeServoAngle(uint8_t bus, uint8_t channel, int angle);
void setAllCoxas(int angle);

// Right-middle leg covering motion helpers
void moveRightMiddleCOnly(int cStart, int cEnd, int fHold, int stepDelayMs);
void moveRightMiddleCAndF(int cStart, int cEnd, int fStart, int fEnd, int totalSteps, int stepDelayMs);