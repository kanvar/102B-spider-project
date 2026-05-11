#include <Arduino.h>
#include "FireSensor.h"
#include "../Config.h"

static bool fireDetected = false;

void fireSensorSetup() {
  pinMode(FIRE_SENSOR_PIN, INPUT);
}

bool isFireDetected() {
  int val = digitalRead(FIRE_SENSOR_PIN);

  return (val == LOW);
}

void checkFireSensor() {
  fireDetected = isFireDetected();

  if (fireDetected) {
    Serial.println("ALERT:FIRE_DETECTED");
  }
}