#include <Arduino.h>
#include "control/StateMachine.h"

void setup() {
  Serial.begin(115200);
  delay(200);

  stateMachineSetup();
}

void loop() {
  stateMachineLoop();
}