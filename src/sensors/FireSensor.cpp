#include <Arduino.h>
#include "FireSensor.h"
#include "../Config.h"

static bool fireDetected = false;

void fireSensorSetup() {
  pinMode(FIRE_SENSOR_PIN, INPUT);
}

// Simple digital read
bool isFireDetected() {
  int val = digitalRead(FIRE_SENSOR_PIN);

  // Most modules:
  // LOW = flame detected
  // HIGH = no flame
  return (val == LOW);
}

void checkFireSensor() {
  fireDetected = isFireDetected();

  if (fireDetected) {
    Serial.println("ALERT:FIRE_DETECTED");
  }
}