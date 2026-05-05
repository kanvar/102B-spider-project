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

#define DRILL_TRIGGER_CM        3.0
#define COXA_START_ANGLE        90
#define COXA_MIN_ANGLE          45

#define DESCENT_FAR_DISTANCE    20.0
#define DESCENT_NEAR_DISTANCE   5.0
#define DESCENT_FAST_MS         30
#define DESCENT_SLOW_MS         200

#define DRILLING_PHASE_DESCEND  0
#define DRILLING_PHASE_LIFT     1

#define BASELINE_TOLERANCE_CM   0.5

// =================================================================
// Per-state firstEntry flags (module-level so SerialCommand can
// jump states and the new state always re-initializes cleanly)
// =================================================================

static State previousState      = IDLE;
static bool idleFirstEntry      = true;
static bool drillingFirstEntry  = true;
static bool seedingFirstEntry   = true;
static bool coveringFirstEntry  = true;

// Reset the entry flag for whatever state we just jumped to
static void onStateChanged(State newState) {
  switch (newState) {
    case IDLE:     idleFirstEntry     = true; break;
    case DRILLING: drillingFirstEntry = true; break;
    case SEEDING:  seedingFirstEntry  = true; break;
    case COVERING: coveringFirstEntry = true; break;
    default: break;
  }
  Serial.print("STATE_ACK:");
  Serial.println(stateToString(newState));
}

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
// Helper: compute step delay based on ultrasonic distance
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

  currentState  = IDLE;
  previousState = IDLE;

  Serial.print("StateMachine: starting in ");
  Serial.println(stateToString(currentState));
}

// =================================================================
// IDLE
// — zeros all 18 servos on first entry
// — tares ultrasonic baseline
// — waits for STATE:DRILLING
// =================================================================

void serviceIdle() {
  if (idleFirstEntry) {
    Serial.println("IDLE: zeroing servos and taring ultrasonic");

    zeroAllServos();
    delay(2000);
    tareDistance();

    idleFirstEntry = false;
    Serial.println("IDLE: ready, waiting for trigger");
  }

  readDistanceIfReady();
}

// =================================================================
// DRILLING
// — drill motor ON for entire phase
// — phase 0 (DESCEND): lower body coxa 90→45, speed scales with distance
// — phase 1 (LIFT):    raise body back to 90°
// — auto-advances to SEEDING when baseline restored or coxa at 90°
// =================================================================

void serviceDrilling() {
  static int currentCoxaAngle       = COXA_START_ANGLE;
  static unsigned long lastStepTime = 0;
  static int phase                  = DRILLING_PHASE_DESCEND;

  if (drillingFirstEntry) {
    Serial.println("DRILLING: drill on, beginning descent");

    drill.on();
    currentCoxaAngle   = COXA_START_ANGLE;
    setAllCoxas(currentCoxaAngle);
    phase              = DRILLING_PHASE_DESCEND;
    lastStepTime       = millis();
    drillingFirstEntry = false;
  }

  float distance = readDistanceIfReady();

  // ── DESCEND ──────────────────────────────────────────────
  if (phase == DRILLING_PHASE_DESCEND) {
    bool atGround   = (distance > 0 && distance <= DRILL_TRIGGER_CM);
    bool atMinAngle = (currentCoxaAngle <= COXA_MIN_ANGLE);

    if (atGround || atMinAngle) {
      Serial.println("DRILLING: descent complete, beginning lift");
      phase        = DRILLING_PHASE_LIFT;
      lastStepTime = millis();
    } else {
      unsigned long stepDelay = computeStepDelay(distance);
      if (millis() - lastStepTime >= stepDelay) {
        lastStepTime = millis();
        currentCoxaAngle--;
        setAllCoxas(currentCoxaAngle);
        Serial.print("DRILLING [DESCEND] coxa=");
        Serial.print(currentCoxaAngle);
        Serial.print(" dist=");
        Serial.println(distance);
      }
    }
  }

  // ── LIFT ─────────────────────────────────────────────────
  else if (phase == DRILLING_PHASE_LIFT) {

    // Hard stop — coxa back at 90°
    if (currentCoxaAngle >= COXA_START_ANGLE) {
      Serial.println("DRILLING: lift complete — drill off, advancing to SEEDING");
      drill.off();
      drillingFirstEntry = true;
      seedingFirstEntry  = true;
      currentState       = SEEDING;
      Serial.println("STATE_ACK:SEEDING");
      return;
    }

    // Soft trigger — ultrasonic baseline restored
    float relative = readRelativeDistanceIfReady();
    if (relative > -9000.0 && abs(relative) <= BASELINE_TOLERANCE_CM) {
      Serial.print("DRILLING: baseline restored (rel=");
      Serial.print(relative);
      Serial.println(") — drill off, advancing to SEEDING");
      drill.off();
      drillingFirstEntry = true;
      seedingFirstEntry  = true;
      currentState       = SEEDING;
      Serial.println("STATE_ACK:SEEDING");
      return;
    }

    unsigned long stepDelay = computeStepDelay(distance);
    if (millis() - lastStepTime >= stepDelay) {
      lastStepTime = millis();
      currentCoxaAngle++;
      setAllCoxas(currentCoxaAngle);
      Serial.print("DRILLING [LIFT] coxa=");
      Serial.print(currentCoxaAngle);
      Serial.print(" dist=");
      Serial.println(distance);
    }
  }
}

// =================================================================
// SEEDING
// — stepper runs 2500 steps to drop seeds into hole
// — auto-advances to COVERING
// =================================================================

void serviceSeeding() {
  if (seedingFirstEntry) {
    Serial.println("SEEDING: running stepper 2500 steps");

    seeder.rotateSteps(2500);

    Serial.println("SEEDING: done — advancing to COVERING");
    seedingFirstEntry  = false;
    coveringFirstEntry = true;
    currentState       = COVERING;
    Serial.println("STATE_ACK:COVERING");
  }
}

// =================================================================
// COVERING
// — right-middle leg sweeps to push soil over seed hole
// — path repeated 2 times:
//     Step 1: C 90→60, F holds at 0
//     Step 2: C 60→140, F 0→90  (synchronized)
//     Step 3: C 140→90, F 90→0  (synchronized return)
// — auto-advances to IDLE
// =================================================================

void serviceCovering() {
  const int STEP_DELAY_MS = 20;
  const int HOLD_MS       = 700;
  const int NUM_REPS      = 2;

  if (coveringFirstEntry) {
    Serial.println("COVERING: starting sweep");

    writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, 90);
    writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, 0);
    delay(HOLD_MS);

    for (int rep = 0; rep < NUM_REPS; rep++) {
      Serial.print("COVERING: rep "); Serial.println(rep + 1);

      moveRightMiddleCOnly(90, 60, 0, STEP_DELAY_MS);
      delay(HOLD_MS);

      moveRightMiddleCAndF(60, 140, 0, 90, 90, STEP_DELAY_MS);
      delay(HOLD_MS);

      moveRightMiddleCAndF(140, 90, 90, 0, 90, STEP_DELAY_MS);
      delay(HOLD_MS);
    }

    // Return to home position
    writeServoAngle(rightMiddle_C.bus, rightMiddle_C.ch, 90);
    writeServoAngle(rightMiddle_F.bus, rightMiddle_F.ch, 0);

    Serial.println("COVERING: complete — returning to IDLE");
    coveringFirstEntry = false;
    idleFirstEntry     = true;
    currentState       = IDLE;
    Serial.println("STATE_ACK:IDLE");
  }
}

// =================================================================
// Main loop
// =================================================================

void stateMachineLoop() {
  checkSerialCommand();
  wifiCommandLoop();

  // If SerialCommand.cpp changed currentState externally,
  // detect it here and reset the firstEntry flag for the new state
  if (currentState != previousState) {
    onStateChanged(currentState);
    previousState = currentState;
  }

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