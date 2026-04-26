#include <Arduino.h>
#include "SerialCommand.h"
#include "StateMachine.h"
#include "../motion/LegController.h"

// Later, when these are fully connected, we can include:
// #include "../actuators/DrillMotor.h"
// #include "../actuators/SeederMotor.h"

// ========== EVENT CHECKER: GUI serial commands ==========
void checkSerialCommand() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.length() == 0) return;

  handleSerialCommand(cmd);
}

// ========== COMMAND HANDLER ==========
void handleSerialCommand(String cmd) {
  Serial.print("RX:");
  Serial.println(cmd);

  // ---------- ABORT / EMERGENCY STOP ----------
  if (cmd == "ABORT" || cmd == "STATE:ABORT") {
    currentState = ABORT;

    // Stop all motion/actuators here
    resetGait();

    // Later add:
    // drillMotor.off();
    // seederMotor.release();

    Serial.println("STATE_ACK:ABORT");
    Serial.println("ALERT:ABORT_TRIGGERED");
    return;
  }

  // ---------- STATE COMMANDS ----------
  if (cmd == "STATE:IDLE") {
    currentState = IDLE;

    resetGait();

    // Later add:
    // drillMotor.off();
    // seederMotor.release();

    Serial.println("STATE_ACK:IDLE");
  }

  else if (cmd == "STATE:WALKING") {
    currentState = WALKING;

    resetGait();

    Serial.println("STATE_ACK:WALKING");
  }

  else if (cmd == "STATE:PRE_PLANTING") {
    currentState = PRE_PLANTING;

    // In this state, ultrasonic sensor decides when the drill can turn on.
    // The drill should NOT automatically turn on from GUI alone here.
    // StateMachine will check distance and then activate drill.

    Serial.println("STATE_ACK:PRE_PLANTING");
  }

  else if (cmd == "STATE:PLANTING") {
    currentState = PLANTING;

    // In this state, drill should stop and seeder should activate.
    // Later add:
    // drillMotor.off();
    // seederMotor.rotateOneRevolution();

    Serial.println("STATE_ACK:PLANTING");
  }

  // ---------- MOVEMENT COMMANDS ----------
  else if (cmd == "MOVE:FORWARD") {
    currentState = WALKING;
    resetGait();

    Serial.println("MOVE_ACK:FORWARD");
  }

  else if (cmd == "MOVE:BACKWARD") {
    currentState = WALKING;
    resetGait();

    Serial.println("MOVE_ACK:BACKWARD");
  }

  else if (cmd == "MOVE:LEFT") {
    currentState = WALKING;
    resetGait();

    Serial.println("MOVE_ACK:LEFT");
  }

  else if (cmd == "MOVE:RIGHT") {
    currentState = WALKING;
    resetGait();

    Serial.println("MOVE_ACK:RIGHT");
  }

  else if (cmd == "MOVE:STOP") {
    resetGait();

    Serial.println("MOVE_ACK:STOP");
  }

  // ---------- DRILL COMMANDS ----------
  else if (cmd == "DRILL:ON") {
    // Later this will call:
    // drillMotor.on();

    Serial.println("DRILL_ACK:ON_REQUESTED");
  }

  else if (cmd == "DRILL:OFF") {
    // Later this will call:
    // drillMotor.off();

    Serial.println("DRILL_ACK:OFF_REQUESTED");
  }

  // ---------- SEEDER COMMANDS ----------
  else if (cmd == "SEEDER:ON") {
    // Later this will call:
    // seederMotor.rotateOneRevolution();

    Serial.println("SEEDER_ACK:ON_REQUESTED");
  }

  else if (cmd == "SEEDER:OFF") {
    // Later this will call:
    // seederMotor.release();

    Serial.println("SEEDER_ACK:OFF_REQUESTED");
  }

  // ---------- UNKNOWN COMMAND ----------
  else {
    Serial.print("UNKNOWN_COMMAND:");
    Serial.println(cmd);
  }
}