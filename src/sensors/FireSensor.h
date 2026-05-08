#pragma once

void fireSensorSetup();

// Call every loop (non-blocking)
void checkFireSensor();

// Returns true if fire is detected
bool isFireDetected();