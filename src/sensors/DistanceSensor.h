#pragma once

void  distanceSensorSetup();
float readDistanceIfReady();
float readRelativeDistanceIfReady();
bool  isCloseEnoughToDrill(float distanceCm);
bool  tareDistance();
float getLastDistance();
float getBaselineDistance();
bool  isDistanceTared();
void  resetTare();