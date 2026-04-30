#include <Arduino.h>
#include "control/StateMachine.h"
#include "actuators/DrillMotor.h"
#include "actuators/SeederMotor.h"
#include "motion/LegController.h"
#include "sensors/DistanceSensor.h"

DrillMotor drill;
SeederMotor seeder;

void setup() {
  Serial.begin(115200);
  delay(200);

  drill.begin();
  seeder.begin();
  legSetup();
  distanceSensorSetup();
  stateMachineSetup();
}

void loop() {
  stateMachineLoop();
}