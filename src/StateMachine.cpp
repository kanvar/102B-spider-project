#include <Arduino.h>
#include "StateMachine.h"
#include "LegController.h"
#include "DistanceSensor.h"
#include "SerialCommand.h"
#include "Config.h"

void checkDistanceSensor();

State currentState = IDLE;

void stateMachineSetup() {
  legSetup();
  distanceSensorSetup();
  Serial.println("State machine initialized. State: IDLE");
}

void stateMachineLoop() {
  // event checkers
  checkSerialCommand();
  checkDistanceSensor();

  switch (currentState) {
    case IDLE:     serviceIdle();     break;
    case WALKING:  serviceWalking();  break;
    case PLANTING: servicePlanting(); break;
  }
}

// ========== event checker: ultrasonic distance (10cm away we stop and change states))==========
void checkDistanceSensor() {
  float distance = readDistanceIfReady();
  if (distance < 0) return;
  if (currentState == IDLE) return;  // IDLE state is a manual override

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance <= PLANT_THRESHOLD_CM) {
    if (currentState != PLANTING) {
      Serial.print("EVENT: Object within 10 cm (");
      Serial.print(distance);
      Serial.println(" cm). -> PLANTING");
      currentState = PLANTING;
    }
  } else {
    if (currentState != WALKING) {
      Serial.print("EVENT: Path clear (");
      Serial.print(distance);
      Serial.println(" cm). -> WALKING");
      currentState = WALKING;
      resetGait();
    }
  }
}