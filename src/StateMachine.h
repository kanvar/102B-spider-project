#pragma once

enum State {
  IDLE,
  WALKING,
  PLANTING
};

extern State currentState;

void stateMachineSetup();
void stateMachineLoop();