#include <Arduino.h>
#include "SerialCommand.h"
#include "StateMachine.h"

void checkSerialCommand() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.length() == 0) return;

  handleSerialCommand(cmd);
}

void handleSerialCommand(String cmd) {
  Serial.print("RX:");
  Serial.println(cmd);

  // Abort state
  if (cmd == "ABORT" || cmd == "STATE:ABORT") {
    currentState = ABORT;
    Serial.println("STATE_ACK:ABORT");
    Serial.println("ALERT:ABORT_TRIGGERED");
    return;
  }

  // commands
  if (cmd == "STATE:IDLE") {
    currentState = IDLE;
    Serial.println("STATE_ACK:IDLE");
    return;
  }

  if (cmd == "STATE:DRILLING") {
    currentState = DRILLING;
    Serial.println("STATE_ACK:DRILLING");
    return;
  }

  if (cmd == "STATE:SEEDING") {
    currentState = SEEDING;
    Serial.println("STATE_ACK:SEEDING");
    return;
  }

  if (cmd == "STATE:COVERING") {
    currentState = COVERING;
    Serial.println("STATE_ACK:COVERING");
    return;
  }

  if (cmd == "SEQUENCE:START_PLANTING_SEQUENCE") {
    Serial.println("ACK:START_PLANTING_SEQUENCE");
    currentState = DRILLING;
    Serial.println("STATE_ACK:DRILLING");
    return;
  }

  if (cmd == "SEQUENCE:CHECK_HEIGHT") {
    Serial.println("ACK:CHECK_HEIGHT");
    return;
  }

  if (cmd == "SEQUENCE:BEGIN_DRILLING") {
    Serial.println("ACK:BEGIN_DRILLING");
    currentState = DRILLING;
    Serial.println("STATE_ACK:DRILLING");
    return;
  }

  if (cmd == "SEQUENCE:DRILL_COMPLETE") {
    Serial.println("ACK:DRILL_COMPLETE");
    currentState = SEEDING;
    Serial.println("STATE_ACK:SEEDING");
    return;
  }

  if (cmd == "SEQUENCE:DROP_SEED") {
    Serial.println("ACK:DROP_SEED");
    currentState = SEEDING;
    Serial.println("STATE_ACK:SEEDING");
    return;
  }

  if (cmd == "SEQUENCE:COVER_SEED") {
    Serial.println("ACK:COVER_SEED");
    currentState = COVERING;
    Serial.println("STATE_ACK:COVERING");
    return;
  }

  if (cmd == "SEQUENCE:RETURN_HOME") {
    Serial.println("ACK:RETURN_HOME");
    currentState = IDLE;
    Serial.println("STATE_ACK:IDLE");
    return;
  }

  Serial.print("UNKNOWN_COMMAND:");
  Serial.println(cmd);
}