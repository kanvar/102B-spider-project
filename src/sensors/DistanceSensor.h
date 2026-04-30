#pragma once

void  distanceSensorSetup();
float readDistanceIfReady();
float readRelativeDistanceIfReady();
bool  isCloseEnoughToDrill(float distanceCm);
bool  tareDistance();           // returns true on success, false on timeout
float getLastDistance();        // returns last cached reading
float getBaselineDistance();    // returns tare baseline, -1 if not tared
bool  isDistanceTared();
void  resetTare();