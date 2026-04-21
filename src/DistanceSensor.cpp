#include <Arduino.h>
#include "DistanceSensor.h"
#include "Config.h"

static unsigned long lastRead = 0;

void distanceSensorSetup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

float readDistanceIfReady() {
  if (millis() - lastRead < SENSOR_INTERVAL_MS) return -1.0;
  lastRead = millis();

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1.0;

  return duration * 0.0343 / 2.0;
}