#include <Arduino.h>
#include "StateMachine.h"
#include "SerialCommand.h"
#include "../motion/LegController.h"
#include "../sensors/DistanceSensor.h"
#include "../Config.h"

// Later, when connected:
// #include "../actuators/DrillMotor.h"
// #include "../actuators/SeederMotor.h"

State currentState = IDLE;

static bool drillStarted = false;
static bool seederStarted = false;

void checkDistanceSensor();

void stateMachineSetup() {
  legSetup();
  distanceSensorSetup();

  currentState = IDLE;
  drillStarted = false;
  seederStarted = false;

  Serial.println("State machine initialized. State: IDLE");
}

void stateMachineLoop() {
  // Event checker: GUI commands
  checkSerialCommand();

  // Event checker: ultrasonic sensor
  checkDistanceSensor();

  // State services
  switch (currentState) {
    case IDLE:
      serviceIdle();
      break;

    case WALKING:
      serviceWalking();
      break;

    case PRE_PLANTING:
      servicePrePlanting();
      break;

    case PLANTING:
      servicePlanting();
      break;

    case ABORT:
      serviceAbort();
      break;
  }
}

// ========== EVENT CHECKER: ULTRASONIC SENSOR ==========
// New logic:
// The ultrasonic sensor does NOT decide walking anymore.
// It only decides whether the robot is close enough to drill during PRE_PLANTING.
void checkDistanceSensor() {
  float distance = readDistanceIfReady();

  if (distance < 0) return;

  Serial.print("DISTANCE:");
  Serial.println(distance);

  if (currentState != PRE_PLANTING) return;

  if (isCloseEnoughToDrill(distance)) {
    if (!drillStarted) {
      drillStarted = true;

      Serial.print("EVENT: Correct drilling height reached: ");
      Serial.print(distance);
      Serial.println(" cm");

      // Later add:
      // drillMotor.on();

      Serial.println("DRILL_ACK:ON_REQUESTED");
    }
  } 
  else {
    if (drillStarted) {
      drillStarted = false;

      // Later add:
      // drillMotor.off();

      Serial.println("DRILL_ACK:OFF_REQUESTED");
    }

    Serial.print("EVENT: Not close enough to drill: ");
    Serial.print(distance);
    Serial.println(" cm");
  }
}

// ========== STATE SERVICES ==========

void serviceIdle() {
  // Robot waiting.
  // Motors should be off.
  drillStarted = false;
  seederStarted = false;

  // Later add:
  // drillMotor.off();
  // seederMotor.release();
}

void serviceWalking() {
  // Walking state:
  // Only leg servos should move.
  // Drill and seeder should be off.
  drillStarted = false;
  seederStarted = false;

  serviceWalkingGait();

  // Later add:
  // drillMotor.off();
  // seederMotor.release();
}

void servicePrePlanting() {
  // Pre-planting state:
  // Robot checks ultrasonic distance.
  // Drill turns on only when distance <= PLANT_THRESHOLD_CM.
  // Actual drill activation is handled in checkDistanceSensor().
}

void servicePlanting() {
  // Planting state:
  // Drill should stop.
  // Seeder should activate once.
  // Then robot returns to WALKING.

  if (!seederStarted) {
    seederStarted = true;
    drillStarted = false;

    // Later add:
    // drillMotor.off();
    // seederMotor.rotateOneRevolution();

    Serial.println("DRILL_ACK:OFF_REQUESTED");
    Serial.println("SEEDER_ACK:ON_REQUESTED");
    Serial.println("SEEDER_DONE");

    currentState = WALKING;
    resetGait();

    Serial.println("STATE_ACK:WALKING");
  }
}

void serviceAbort() {
  // Abort state:
  // Everything must stop and stay stopped until GUI sends STATE:IDLE.
  drillStarted = false;
  seederStarted = false;

  resetGait();

  // Later add:
  // drillMotor.off();
  // seederMotor.release();
}

const char* stateToString(State state) {
  switch (state) {
    case IDLE:
      return "IDLE";
    case WALKING:
      return "WALKING";
    case PRE_PLANTING:
      return "PRE_PLANTING";
    case PLANTING:
      return "PLANTING";
    case ABORT:
      return "ABORT";
    default:
      return "UNKNOWN";
  }
}