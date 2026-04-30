#include <Arduino.h>
#include "DistanceSensor.h"
#include "../Config.h"


static unsigned long lastRead = 0;
static unsigned long lastPrint = 0;
static float lastDistanceCm = -1.0;

// Tare state
static float baselineDistance = -1.0;
static bool tared = false;


#define DISTANCE_PRINT_INTERVAL_MS 5000  // print to serial every 5 seconds


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
    // Don't spam the serial — only complain every 5 seconds
    if (millis() - lastPrint >= DISTANCE_PRINT_INTERVAL_MS) {
      Serial.println("DISTANCE_ERROR:NO_ECHO");
      lastPrint = millis();
    }
    return -1.0;
  }

  // Convert time to distance in cm
  float distanceCm = duration * 0.0343 / 2.0;
  lastDistanceCm = distanceCm;

  // Throttle serial output to once every 5 seconds
  if (millis() - lastPrint >= DISTANCE_PRINT_INTERVAL_MS) {
    Serial.print("DISTANCE:");
    Serial.print(distanceCm);
    
    if (tared) {
      Serial.print(" REL:");
      Serial.print(distanceCm - baselineDistance);
    }
    Serial.println();
    
    lastPrint = millis();
  }

  return distanceCm;
}

bool isCloseEnoughToDrill(float distanceCm) {
  if (distanceCm < 0) {
    return false;
  }

  return distanceCm <= PLANT_THRESHOLD_CM;
}


// Tare

bool tareDistance() {
  Serial.println("TARE:STARTING");
  
  const int SAMPLES_NEEDED = 5;
  const unsigned long TIMEOUT_MS = 3000;
  const unsigned long SAMPLE_DELAY_MS = 100;
  
  float sum = 0;
  int count = 0;
  unsigned long startTime = millis();
  
  // Save the throttle state and force fresh reads for taring
  unsigned long savedLastRead = lastRead;
  lastRead = 0;
  
  while (count < SAMPLES_NEEDED && (millis() - startTime) < TIMEOUT_MS) {
    // Force-read regardless of throttle interval
    lastRead = 0;
    float d = readDistanceIfReady();
    
    if (d > 0) {
      sum += d;
      count++;
      Serial.print("TARE:SAMPLE_");
      Serial.print(count);
      Serial.print(":");
      Serial.println(d);
    }
    delay(SAMPLE_DELAY_MS);
  }
  
  if (count >= 3) {
    baselineDistance = sum / count;
    tared = true;
    Serial.print("TARE:SUCCESS:BASELINE=");
    Serial.println(baselineDistance);
    return true;
  } else {
    Serial.println("TARE:FAILED");
    return false;
  }
}

float readRelativeDistanceIfReady() {
  if (!tared) return -9999.0;
  
  float raw = readDistanceIfReady();
  if (raw < 0) return -9999.0;
  
  return raw - baselineDistance;
}

bool isDistanceTared() {
  return tared;
}

float getBaselineDistance() {
  return tared ? baselineDistance : -1.0;
}

void resetTare() {
  tared = false;
  baselineDistance = -1.0;
  Serial.println("TARE:RESET");
}