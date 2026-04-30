#include <Arduino.h>
#include "SerialCommand.h"
#include "StateMachine.h"

// ========== EVENT CHECKER ==========
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

  // ---------- ABORT ----------
  if (cmd == "ABORT" || cmd == "STATE:ABORT") {
    currentState = ABORT;
    Serial.println("STATE_ACK:ABORT");
    return;
  }

  // ---------- STATE COMMANDS ----------
  if (cmd == "STATE:IDLE") {
    currentState = IDLE;
    Serial.println("STATE_ACK:IDLE");
  }
  else if (cmd == "STATE:DRILLING") {
    currentState = DRILLING;
    Serial.println("STATE_ACK:DRILLING");
  }
  else if (cmd == "STATE:SEEDING") {
    currentState = SEEDING;
    Serial.println("STATE_ACK:SEEDING");
  }
  else if (cmd == "STATE:COVERING") {
    currentState = COVERING;
    Serial.println("STATE_ACK:COVERING");
  }

  // ---------- UNKNOWN ----------
  else {
    Serial.print("UNKNOWN_COMMAND:");
    Serial.println(cmd);
  }
}