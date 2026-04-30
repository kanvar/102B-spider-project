#include <Arduino.h>
#include "SeederMotor.h"
#include "../Config.h"

SeederMotor::SeederMotor() {
  stepIndex = 0;
}

void SeederMotor::begin() {
  pinMode(SEEDER_IN1_PIN, OUTPUT);
  pinMode(SEEDER_IN2_PIN, OUTPUT);
  pinMode(SEEDER_IN3_PIN, OUTPUT);
  pinMode(SEEDER_IN4_PIN, OUTPUT);
  release();
}

void SeederMotor::setStep(int a, int b, int c, int d) {
  digitalWrite(SEEDER_IN1_PIN, a);
  digitalWrite(SEEDER_IN2_PIN, b);
  digitalWrite(SEEDER_IN3_PIN, c);
  digitalWrite(SEEDER_IN4_PIN, d);
}

void SeederMotor::release() {
  setStep(LOW, LOW, LOW, LOW);
}

void SeederMotor::rotateSteps(int durationMs) {
  Serial.print("SEEDER_ACK:ON (");
  Serial.print(durationMs);
  Serial.println(" ms)");
  
  const int sequence[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
  };
  
  unsigned long startTime = millis();
  
  while (millis() - startTime < (unsigned long)durationMs) {
    stepIndex = (stepIndex + 1) % 8;
    setStep(
      sequence[stepIndex][0],
      sequence[stepIndex][1],
      sequence[stepIndex][2],
      sequence[stepIndex][3]
    );
    delay(SEEDER_STEP_DELAY_MS);
  }
  
  release();
  Serial.println("SEEDER_ACK:OFF");
  Serial.println("SEEDER_DONE");
}