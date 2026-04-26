#include <Arduino.h>
#include "DistanceSensor.h"
#include "../Config.h"

static unsigned long lastRead = 0;

void distanceSensorSetup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);
}

float readDistanceIfReady() {
  // Only read sensor every SENSOR_INTERVAL_MS
  if (millis() - lastRead < SENSOR_INTERVAL_MS) {
    return -1.0;
  }

  lastRead = millis();

  // Trigger ultrasonic pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  // Read echo pulse
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  // If no echo is detected, return invalid reading
  if (duration == 0) {
    Serial.println("DISTANCE_ERROR:NO_ECHO");
    return -1.0;
  }

  // Convert time to distance in cm
  float distanceCm = duration * 0.0343 / 2.0;

  Serial.print("DISTANCE:");
  Serial.println(distanceCm);

  return distanceCm;
}

bool isCloseEnoughToDrill(float distanceCm) {
  if (distanceCm < 0) {
    return false;
  }

  return distanceCm <= PLANT_THRESHOLD_CM;
}