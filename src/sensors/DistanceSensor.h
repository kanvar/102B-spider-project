#pragma once

void distanceSensorSetup();

float readDistanceIfReady();

bool isCloseEnoughToDrill(float distanceCm);

bool tareDistance();

float readRelativeDistanceIfReady();

bool isDistanceTared();

float getBaselineDistance();

void resetTare();