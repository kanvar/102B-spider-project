#include <Arduino.h>
#include "DrillMotor.h"
#include "../Config.h"

DrillMotor::DrillMotor() {
  speed = DRILL_DEFAULT_SPEED;
  running = false;
}

void DrillMotor::begin() {
  pinMode(DRILL_INA_PIN, OUTPUT);
  pinMode(DRILL_INB_PIN, OUTPUT);
  pinMode(DRILL_PWM_PIN, OUTPUT);

  off();
}

void DrillMotor::on() {
  digitalWrite(DRILL_INA_PIN, HIGH);
  digitalWrite(DRILL_INB_PIN, LOW);

  analogWrite(DRILL_PWM_PIN, speed);

  running = true;

  Serial.println("DRILL_ACK:ON");
  Serial.print("DRILL_SPEED:");
  Serial.println(speed);
}

void DrillMotor::off() {
  analogWrite(DRILL_PWM_PIN, 0);

  digitalWrite(DRILL_INA_PIN, LOW);
  digitalWrite(DRILL_INB_PIN, LOW);

  running = false;

  Serial.println("DRILL_ACK:OFF");
  Serial.println("DRILL_SPEED:0");
}

void DrillMotor::setSpeed(int newSpeed) {
  speed = constrain(newSpeed, 0, 255);

  if (running) {
    analogWrite(DRILL_PWM_PIN, speed);
  }

  Serial.print("DRILL_SPEED:");
  Serial.println(speed);
}

int DrillMotor::getSpeed() {
  return speed;
}

bool DrillMotor::isRunning() {
  return running;
}