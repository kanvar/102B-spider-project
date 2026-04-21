#include <Arduino.h>
#include "SerialCommand.h"
#include "StateMachine.h"
#include "LegController.h"

// ========== EVENT CHECKER: GUI serial commands ==========
void checkSerialCommand() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.length() == 0) return;

  Serial.print("EVENT: Received command '");
  Serial.print(cmd);
  Serial.println("'");

  if (cmd == "STOP" || cmd == "ESTOP") {
    currentState = IDLE;
    Serial.println("-> IDLE");
  }
  else if (cmd == "MOVE_UP") {
    currentState = WALKING;
    resetGait();
    Serial.println("-> WALKING");
  }
  else if (cmd == "MOVE_DOWN") {
    currentState = PLANTING;
    Serial.println("-> PLANTING");
  }
  else {
    Serial.println("(unmapped command)");
  }
}