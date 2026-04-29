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

  if (cmd == "ABORT" || cmd == "STATE:ABORT") {
    setRobotState(ABORT);
    Serial.println("ALERT:ABORT_TRIGGERED");
    return;
  }

  if (cmd == "SAFETY:FIRE_DETECTED") {
    Serial.println("ALERT:FIRE_DETECTED");
    setRobotState(ABORT);
    return;
  }

  if (cmd == "SAFETY:JAM_DETECTED") {
    Serial.println("ALERT:JAM_DETECTED");
    setRobotState(RETURN_HOME);
    return;
  }

  if (cmd == "STATE:IDLE") {
    setRobotState(IDLE);
    return;
  }

  if (cmd == "STATE:DRILLING") {
    setRobotState(DRILLING);
    return;
  }

  if (cmd == "STATE:SEEDING") {
    setRobotState(SEEDING);
    return;
  }

  if (cmd == "STATE:COVERING") {
    setRobotState(COVERING);
    return;
  }

  if (cmd == "STATE:RETURN_HOME") {
    setRobotState(RETURN_HOME);
    return;
  }

  if (cmd == "SEQUENCE:START_PLANTING_SEQUENCE") {
    Serial.println("ACK:START_PLANTING_SEQUENCE");
    setRobotState(DRILLING);
    return;
  }

  if (cmd == "SEQUENCE:CHECK_HEIGHT") {
    Serial.println("ACK:CHECK_HEIGHT");
    setRobotState(DRILLING);
    return;
  }

  if (cmd == "SEQUENCE:BEGIN_DRILLING") {
    Serial.println("ACK:BEGIN_DRILLING");
    setRobotState(DRILLING);
    Serial.println("DRILL_ACK:ON_REQUESTED");
    return;
  }

  if (cmd == "SEQUENCE:DRILL_COMPLETE") {
    Serial.println("ACK:DRILL_COMPLETE");
    Serial.println("DRILL_ACK:OFF_REQUESTED");
    setRobotState(SEEDING);
    return;
  }

  if (cmd == "SEQUENCE:DROP_SEED") {
    Serial.println("ACK:DROP_SEED");
    setRobotState(SEEDING);
    Serial.println("SEEDER_ACK:ON_REQUESTED");
    return;
  }

  if (cmd == "SEQUENCE:MOVE_SIDEWAYS") {
    Serial.println("ACK:MOVE_SIDEWAYS");
    setRobotState(SEEDING);
    Serial.println("MOVE_ACK:SIDEWAYS_REQUESTED");
    return;
  }

  if (cmd == "SEQUENCE:COVER_SEED") {
    Serial.println("ACK:COVER_SEED");
    setRobotState(COVERING);
    Serial.println("COVERING_ACK:REQUESTED");
    return;
  }

  if (cmd == "SEQUENCE:RETURN_HOME") {
    Serial.println("ACK:RETURN_HOME");
    setRobotState(RETURN_HOME);
    Serial.println("RETURN_HOME_ACK:REQUESTED");
    return;
  }

  if (cmd == "DRILL:ON") {
    Serial.println("DRILL_ACK:ON_REQUESTED");
    return;
  }

  if (cmd == "DRILL:OFF") {
    Serial.println("DRILL_ACK:OFF_REQUESTED");
    return;
  }

  if (cmd == "SEEDER:ON") {
    Serial.println("SEEDER_ACK:ON_REQUESTED");
    return;
  }

  if (cmd == "SEEDER:OFF") {
    Serial.println("SEEDER_ACK:OFF_REQUESTED");
    return;
  }

  Serial.print("UNKNOWN_COMMAND:");
  Serial.println(cmd);
}