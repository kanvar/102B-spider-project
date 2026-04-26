#pragma once

void distanceSensorSetup();

// Returns distance in cm if a new reading is ready.
// Returns -1.0 if no new reading is ready or if reading failed.
float readDistanceIfReady();

// Returns true if the measured distance is close enough to drill.
bool isCloseEnoughToDrill(float distanceCm);