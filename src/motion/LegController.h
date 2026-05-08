#pragma once
#include <stdint.h>

void legSetup();
void zeroAllServos();
void writeServoAngle(uint8_t bus, uint8_t channel, int angle);
void setAllCoxas(int angle);
void walkingIDLE();
void move70mm();
void seedingMoveForward();
void sideStepRight();


void moveRightMiddleCOnly(int cStart, int cEnd, int fHold, int stepDelayMs);
void moveRightMiddleCAndF(int cStart, int cEnd, int fStart, int fEnd, 
                          int totalSteps, int stepDelayMs);