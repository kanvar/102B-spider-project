#pragma once

// ---------- PIN DEFINITIONS ----------
constexpr int SERVO1_PIN = 26;   // hip
constexpr int SERVO2_PIN = 13;   // knee
constexpr int SERVO3_PIN = 27;   // ankle
constexpr int TRIG_PIN   = 14;
constexpr int ECHO_PIN   = 32;

// ---------- BEHAVIOR CONSTANTS ----------
constexpr float PLANT_THRESHOLD_CM = 10.0;   // below -> plant
constexpr float WALK_THRESHOLD_CM  = 10.0;   // above -> walk

// ---------- TIMING ----------
constexpr unsigned long SENSOR_INTERVAL_MS = 100;
constexpr unsigned long GAIT_STEP_MS       = 600;
constexpr unsigned long IDLE_PRINT_MS      = 1000;