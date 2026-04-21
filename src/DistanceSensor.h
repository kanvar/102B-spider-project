#pragma once

void distanceSensorSetup();

// Returns distance in cm if new reading is ready, -1 otherwise.
// Non-blocking: call every loop, it rate-limits itself.
float readDistanceIfReady();