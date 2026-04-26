#pragma once

enum State {
  IDLE,
  WALKING,
  PRE_PLANTING,
  PLANTING,
  ABORT
};

extern State currentState;

void stateMachineSetup();
void stateMachineLoop();

void serviceIdle();
void serviceWalking();
void servicePrePlanting();
void servicePlanting();
void serviceAbort();

const char* stateToString(State state);