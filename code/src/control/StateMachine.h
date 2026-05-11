#pragma once

enum State {
  IDLE,
  DRILLING,
  SEEDING,
  COVERING,
  ABORT
};

extern State currentState;

void stateMachineSetup();
void stateMachineLoop();

void serviceIdle();
void serviceDrilling();
void serviceSeeding();
void serviceCovering();

const char* stateToString(State s);