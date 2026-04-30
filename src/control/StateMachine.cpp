#include <Arduino.h>
#include "StateMachine.h"
#include "SerialCommand.h"
#include "WiFiCommand.h"
#include "../motion/LegController.h"
#include "../sensors/DistanceSensor.h"
#include "../actuators/DrillMotor.h"
#include "../actuators/SeederMotor.h"
#include "../Config.h"

extern SeederMotor seeder;
extern DrillMotor drill;

State currentState = IDLE;

// =================================================================
// Drilling configuration
// =================================================================

#define DRILL_TRIGGER_CM       3.0
#define COXA_START_ANGLE       90
#define COXA_MIN_ANGLE         45

#define DESCENT_FAR_DISTANCE   20.0
#define DESCENT_NEAR_DISTANCE  5.0
#define DESCENT_FAST_MS        30
#define DESCENT_SLOW_MS        200

#define DRILLING_PHASE_DESCEND  0
#define DRILLING_PHASE_LIFT     1

#define BASELINE_TOLERANCE_CM   0.5

// =================================================================
// Helper: state to string
// =================================================================

const char* stateToString(State s) {
  switch (s) {
    case IDLE:     return "IDLE";
    case DRILLING: return "DRILLING";
    case SEEDING:  return "SEEDING";
    case COVERING: return "COVERING";
    case ABORT:    return "ABORT";
    default:       return "UNKNOWN";
  }
}

// =================================================================
// Helper: compute step delay based on current ultrasonic distance
// =================================================================

unsigned long computeStepDelay(float distance) {
  if (distance < 0) {
    return DESCENT_SLOW_MS;
  } else if (distance >= DESCENT_FAR_DISTANCE) {
    return DESCENT_FAST_MS;
  } else if (distance <= DESCENT_NEAR_DISTANCE) {
    return DESCENT_SLOW_MS;
  } else {
    float t = (distance - DESCENT_NEAR_DISTANCE) /
              (DESCENT_FAR_DISTANCE - DESCENT_NEAR_DISTANCE);
    return DESCENT_SLOW_MS +
           (unsigned long)((DESCENT_FAST_MS - DESCENT_SLOW_MS) * t);
  }
}

// =================================================================
// Helper: smooth servo movement
// =================================================================

void smoothMoveServoAngle(uint8_t bus, uint8_t ch, int startAngle, int endAngle, int stepDelayMs) {
  if (startAngle < endAngle) {
    for (int angle = startAngle; angle <= endAngle; angle++) {
      writeServoAngle(bus, ch, angle);
      delay(stepDelayMs);
    }
  } else {
    for (int angle = startAngle; angle >= endAngle; angle--) {
      writeServoAngle(bus, ch, angle);
      delay(stepDelayMs);
    }
  }
}

// =================================================================
// Setup
// =================================================================

void stateMachineSetup() {
  Serial.println("StateMachine: setup");

  wifiCommandSetup();

  currentState = IDLE;

  Serial.print("StateMachine: starting in ");
  Serial.println(stateToString(currentState));
}

// =================================================================
// IDLE state
// =================================================================

void serviceIdle() {
  static bool firstEntry = true;

  if (firstEntry) {
    Serial.println("IDLE: first entry — zeroing servos and taring ultrasonic");

    zeroAllServos();
    delay(2000);
    tareDistance();

    firstEntry = false;

    Serial.println("IDLE: setup complete, monitoring for trigger");
  }

  readDistanceIfReady();

  if (currentState != IDLE) {
    firstEntry = true;
  }
}

// =================================================================
// DRILLING state — descend, then lift, drill on the whole time
// =================================================================

void serviceDrilling() {
  static bool firstEntry = true;
  static int currentCoxaAngle = COXA_START_ANGLE;
  static unsigned long lastStepTime = 0;
  static int phase = DRILLING_PHASE_DESCEND;

  if (firstEntry) {
    Serial.println("DRILLING: first entry — drill on, beginning descent");

    drill.on();

    currentCoxaAngle = COXA_START_ANGLE;
    setAllCoxas(currentCoxaAngle);

    phase = DRILLING_PHASE_DESCEND;
    lastStepTime = millis();
    firstEntry = false;
  }

  float distance = readDistanceIfReady();

  // ---------- DESCEND PHASE ----------
  if (phase == DRILLING_PHASE_DESCEND) {

    bool atGround   = (distance > 0 && distance <= DRILL_TRIGGER_CM);
    bool atMinAngle = (currentCoxaAngle <= COXA_MIN_ANGLE);

    if (atGround || atMinAngle) {
      Serial.println("DRILLING: descent complete, beginning lift");
      phase = DRILLING_PHASE_LIFT;
      lastStepTime = millis();
    } else {
      unsigned long stepDelay = computeStepDelay(distance);

      if (millis() - lastStepTime >= stepDelay) {
        lastStepTime = millis();
        currentCoxaAngle--;
        setAllCoxas(currentCoxaAngle);

        Serial.print("DRILLING [DESCEND]: coxa=");
        Serial.print(currentCoxaAngle);
        Serial.print(" distance=");
        Serial.print(distance);
        Serial.print(" step_delay=");
        Serial.println(stepDelay);
      }
    }
  }

  // ---------- LIFT PHASE ----------
  else if (phase == DRILLING_PHASE_LIFT) {

    // Hard stop: reached 90° — done
    if (currentCoxaAngle >= COXA_START_ANGLE) {
      Serial.println("DRILLING: lift complete (reached 90°) — drill off, advancing to SEEDING");
      drill.off();
      firstEntry = true;
      currentState = SEEDING;
      Serial.println("STATE_ACK:SEEDING");
      return;
    }

    // Soft trigger: ultrasonic confirmed baseline
    float relative = readRelativeDistanceIfReady();
    if (relative > -9000.0 && abs(relative) <= BASELINE_TOLERANCE_CM) {
      Serial.print("DRILLING: returned to baseline (rel=");
      Serial.print(relative);
      Serial.println(") — drill off, advancing to SEEDING");
      drill.off();
      firstEntry = true;
      currentState = SEEDING;
      Serial.println("STATE_ACK:SEEDING");
      return;
    }

    unsigned long stepDelay = computeStepDelay(distance);

    if (millis() - lastStepTime >= stepDelay) {
      lastStepTime = millis();
      currentCoxaAngle++;
      setAllCoxas(currentCoxaAngle);

      Serial.print("DRILLING [LIFT]: coxa=");
      Serial.print(currentCoxaAngle);
      Serial.print(" distance=");
      Serial.print(distance);
      Serial.print(" step_delay=");
      Serial.println(stepDelay);
    }
  }

  if (currentState != DRILLING) {
    firstEntry = true;
  }
}

// =================================================================
// SEEDING state — run seeder, then advance to COVERING
// =================================================================

void serviceSeeding() {
  static bool firstEntry = true;

  if (firstEntry) {
    Serial.println("SEEDING: first entry — running seeder");

    seeder.rotateSteps(2500);

    Serial.println("SEEDING: seeder done — advancing to COVERING");

    firstEntry = false;
    currentState = COVERING;
    Serial.println("STATE_ACK:COVERING");
  }

  if (currentState != SEEDING) {
    firstEntry = true;
  }
}

// =================================================================
// COVERING state — right-middle leg sweeps to cover the seed
// =================================================================
//
// Path (repeated 2 times):
//   Step 1: C 90 → 60   (F holds at 0)
//   Step 2: C 60 → 140 + F 0 → 90    (synchronized, 90 steps)
//   Step 3: C 140 → 90 + F 90 → 0    (synchronized return)
//
// After 2 reps: holds at C=90, F=0, then transitions to IDLE.

void serviceCovering() {
  static bool firstEntry = true;

  const int STEP_DELAY_MS = 20;
  const int HOLD_MS       = 700;
  const int NUM_REPS      = 2;

  if (firstEntry) {
    Serial.println("COVERING: first entry — moving to start position");

    writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, 90);
    writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, 0);
    delay(HOLD_MS);

    for (int rep = 0; rep < NUM_REPS; rep++) {
      Serial.print("COVERING: rep ");
      Serial.println(rep + 1);

      Serial.println("COVERING: step 1 — C 90 → 60");
      moveRightMiddleCOnly(90, 60, 0, STEP_DELAY_MS);
      delay(HOLD_MS);

      Serial.println("COVERING: step 2 — C 60 → 140, F 0 → 90");
      moveRightMiddleCAndF(60, 140, 0, 90, 90, STEP_DELAY_MS);
      delay(HOLD_MS);

      Serial.println("COVERING: step 3 — C 140 → 90, F 90 → 0");
      moveRightMiddleCAndF(140, 90, 90, 0, 90, STEP_DELAY_MS);
      delay(HOLD_MS);
    }

    Serial.println("COVERING: complete — holding at start, returning to IDLE");

    writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, 90);
    writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, 0);

    firstEntry = false;
    currentState = IDLE;
    Serial.println("STATE_ACK:IDLE");
  }

  if (currentState != COVERING) {
    firstEntry = true;
  }
}

// =================================================================
// Main loop
// =================================================================

void stateMachineLoop() {
  checkSerialCommand();
  wifiCommandLoop();

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 2000) {
    lastPrint = millis();
    Serial.print("CURRENT_STATE:");
    Serial.println(stateToString(currentState));
  }

  switch (currentState) {
    case IDLE:     serviceIdle();     break;
    case DRILLING: serviceDrilling(); break;
    case SEEDING:  serviceSeeding();  break;
    case COVERING: serviceCovering(); break;
    case ABORT:                       break;
  }
}