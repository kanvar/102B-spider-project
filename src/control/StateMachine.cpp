#include <Arduino.h>
#include "StateMachine.h"
#include "SerialCommand.h"
#include "WiFiCommand.h"

#include "../motion/LegController.h"
#include "../sensors/DistanceSensor.h"
#include "../sensors/FireSensor.h"
#include "../Config.h"

// Later, when connected:
// #include "../actuators/DrillMotor.h"
// #include "../actuators/SeederMotor.h"

State currentState = IDLE;

static bool drillStarted = false;
static bool seederStarted = false;
static bool coveringStarted = false;
static bool returningHome = false;

static unsigned long lastStatusPrint = 0;

// =====================================================
// SETUP
// =====================================================

void stateMachineSetup() {
  legSetup();
  distanceSensorSetup();
  fireSensorSetup();

  wifiCommandSetup();

  currentState = IDLE;

  drillStarted = false;
  seederStarted = false;
  coveringStarted = false;
  returningHome = false;

  Serial.println("SYSTEM_READY");
  Serial.println("STATE_ACK:IDLE");
  Serial.println("State machine initialized. State: IDLE");
  Serial.println("Safety monitor initialized.");
}

// =====================================================
// MAIN LOOP
// =====================================================

void stateMachineLoop() {
  // USB serial command listener
  checkSerialCommand();

  // WiFi command listener
  wifiCommandLoop();

  // Safety monitor must always run, even during ABORT
  checkSafetyMonitor();

  // Distance sensor is mainly used during DRILLING
  checkDistanceSensor();

  // State services
  switch (currentState) {
    case IDLE:
      serviceIdle();
      break;

    case DRILLING:
      serviceDrilling();
      break;

    case SEEDING:
      serviceSeeding();
      break;

    case COVERING:
      serviceCovering();
      break;

    case RETURN_HOME:
      serviceReturnHome();
      break;

    case ABORT:
      serviceAbort();
      break;
  }
}

// =====================================================
// STATE CHANGE HELPER
// =====================================================

void setRobotState(State newState) {
  if (currentState == newState) return;

  State previousState = currentState;
  currentState = newState;

  Serial.print("STATE_CHANGE:");
  Serial.print(stateToString(previousState));
  Serial.print("->");
  Serial.println(stateToString(currentState));

  Serial.print("STATE_ACK:");
  Serial.println(stateToString(currentState));

  // Reset one-time action flags when entering a new state
  if (newState == IDLE) {
    drillStarted = false;
    seederStarted = false;
    coveringStarted = false;
    returningHome = false;
  }

  if (newState == DRILLING) {
    drillStarted = false;
  }

  if (newState == SEEDING) {
    seederStarted = false;
  }

  if (newState == COVERING) {
    coveringStarted = false;
  }

  if (newState == RETURN_HOME) {
    returningHome = false;
  }

  if (newState == ABORT) {
    drillStarted = false;
    seederStarted = false;
    coveringStarted = false;
    returningHome = false;
  }
}

// =====================================================
// ALWAYS-ON SAFETY MONITOR
// =====================================================

void checkSafetyMonitor() {
  checkFireSensor();

  if (isFireDetected()) {
    if (currentState != ABORT) {
      Serial.println("ALERT:FIRE_DETECTED");
      setRobotState(ABORT);
    }
  }

  // Later add other safety checks here:
  // - jam detection
  // - invalid ultrasonic values
  // - unexpected position
  // - lost communication
}

// =====================================================
// DISTANCE SENSOR CHECK
// =====================================================

void checkDistanceSensor() {
  float distance = readDistanceIfReady();

  if (distance < 0) return;

  Serial.print("DISTANCE:");
  Serial.println(distance);

  // Distance only controls drill permission during DRILLING
  if (currentState != DRILLING) return;

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

// =====================================================
// STATE SERVICES
// =====================================================

void serviceIdle() {
  // IDLE:
  // - Robot waiting
  // - Motors off
  // - Servos at home/zero position if needed

  drillStarted = false;
  seederStarted = false;
  coveringStarted = false;
  returningHome = false;

  // Later add:
  // drillMotor.off();
  // seederMotor.release();
  // resetGait();

  if (millis() - lastStatusPrint > 1000) {
    lastStatusPrint = millis();
    Serial.println("STATUS:IDLE");
  }
}

void serviceDrilling() {
  // DRILLING:
  // - Ultrasonic sensor checks height
  // - Drill turns on only when height is correct
  // - Drill complete is triggered by GUI command:
  //   SEQUENCE:DRILL_COMPLETE

  if (millis() - lastStatusPrint > 1000) {
    lastStatusPrint = millis();
    Serial.println("STATUS:DRILLING");
  }
}

void serviceSeeding() {
  // SEEDING:
  // - Drill should be off
  // - Seeder stepper drops one seed
  // - Robot may move sideways after seed drop

  if (!seederStarted) {
    seederStarted = true;
    drillStarted = false;

    // Later add:
    // drillMotor.off();
    // seederMotor.rotateOneRevolution();

    Serial.println("DRILL_ACK:OFF_REQUESTED");
    Serial.println("SEEDER_ACK:ON_REQUESTED");
    Serial.println("SEEDER_DONE");
  }

  if (millis() - lastStatusPrint > 1000) {
    lastStatusPrint = millis();
    Serial.println("STATUS:SEEDING");
  }
}

void serviceCovering() {
  // COVERING:
  // - Middle/side leg covers seed
  // - Can repeat covering motion
  // - Eventually returns home

  if (!coveringStarted) {
    coveringStarted = true;

    // Later add:
    // coverSeedWithMiddleLeg();

    Serial.println("COVERING_ACK:STARTED");
  }

  if (millis() - lastStatusPrint > 1000) {
    lastStatusPrint = millis();
    Serial.println("STATUS:COVERING");
  }
}

void serviceReturnHome() {
  // RETURN_HOME:
  // - Stop drill
  // - Stop/release seeder
  // - Return servos/legs to home position
  // - Then return to IDLE when complete

  if (!returningHome) {
    returningHome = true;

    drillStarted = false;
    seederStarted = false;
    coveringStarted = false;

    // Later add:
    // drillMotor.off();
    // seederMotor.release();
    // returnLegsHome();

    Serial.println("RETURN_HOME_ACK:STARTED");
  }

  if (millis() - lastStatusPrint > 1000) {
    lastStatusPrint = millis();
    Serial.println("STATUS:RETURN_HOME");
  }
}

void serviceAbort() {
  // ABORT:
  // - Stop everything immediately
  // - Stay here until GUI sends STATE:IDLE
  // - Fire sensor still keeps monitoring

  drillStarted = false;
  seederStarted = false;
  coveringStarted = false;
  returningHome = false;

  // Later add:
  // drillMotor.off();
  // seederMotor.release();
  // resetGait();

  if (millis() - lastStatusPrint > 1000) {
    lastStatusPrint = millis();
    Serial.println("STATUS:ABORT");
  }
}

// =====================================================
// STATE NAME HELPER
// =====================================================

const char* stateToString(State state) {
  switch (state) {
    case IDLE:
      return "IDLE";

    case DRILLING:
      return "DRILLING";

    case SEEDING:
      return "SEEDING";

    case COVERING:
      return "COVERING";

    case RETURN_HOME:
      return "RETURN_HOME";

    case ABORT:
      return "ABORT";

    default:
      return "UNKNOWN";
  }
}