#pragma once

enum State {
  IDLE,
  DRILLING,
  SEEDING,
  COVERING,
  RETURN_HOME,
  ABORT
};

extern State currentState;

void stateMachineSetup();
void stateMachineLoop();

void setRobotState(State newState);
const char* stateToString(State state);

void serviceIdle();
void serviceDrilling();
void serviceSeeding();
void serviceCovering();
void serviceReturnHome();
void serviceAbort();

void checkSafetyMonitor();
void checkDistanceSensor();