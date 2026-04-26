#pragma once
#ifndef CONFIG_H
#define CONFIG_H

// ---------- PIN DEFINITIONS ----------
// Leg servos
constexpr int SERVO1_PIN = 26;   // hip
constexpr int SERVO2_PIN = 13;   // knee
constexpr int SERVO3_PIN = 27;   // ankle

// Ultrasonic sensor
constexpr int TRIG_PIN = 14;
constexpr int ECHO_PIN = 32;

// Drill motor driver pins
constexpr int DRILL_INA_PIN = 25;   // CHANGE if needed
constexpr int DRILL_INB_PIN = 15;
constexpr int DRILL_PWM_PIN = 4;    // CHANGE if needed

// Seeder stepper motor pins
constexpr int SEEDER_IN1_PIN = 33;
constexpr int SEEDER_IN2_PIN = 27;  // conflict with SERVO3_PIN
constexpr int SEEDER_IN3_PIN = 12;
constexpr int SEEDER_IN4_PIN = 8;   // may be unsafe on some ESP32 boards

// ---------- BEHAVIOR CONSTANTS ----------
constexpr float PLANT_THRESHOLD_CM = 10.0;
constexpr float WALK_THRESHOLD_CM  = 10.0;

// Drill speed
constexpr int DRILL_DEFAULT_SPEED = 255;

// Seeder settings
constexpr int SEEDER_STEPS_PER_REV = 400;
constexpr int SEEDER_STEP_DELAY_MS = 2;

// ---------- TIMING ----------
constexpr unsigned long SENSOR_INTERVAL_MS = 100;
constexpr unsigned long GAIT_STEP_MS       = 600;
constexpr unsigned long IDLE_PRINT_MS      = 1000;

#endif