#pragma once
#ifndef CONFIG_H
#define CONFIG_H
#include <stdint.h>

// { i2c_address, channel (0-15 on that board) }
struct ServoAddr { uint8_t bus; uint8_t ch; };

//Left Legs

constexpr ServoAddr leftFront_H = { 0x40, 2 };
constexpr ServoAddr leftFront_C = { 0x40, 3 };
constexpr ServoAddr leftFront_F = { 0x40, 1 };

constexpr ServoAddr leftMiddle_H = { 0x60, 7 };
constexpr ServoAddr leftMiddle_C = { 0x60, 5 };
constexpr ServoAddr leftMiddle_F = { 0x60, 6 };

constexpr ServoAddr leftBack_H = { 0x40, 7 };
constexpr ServoAddr leftBack_C = { 0x40, 5 };
constexpr ServoAddr leftBack_F = { 0x40, 6 };

//Right Legs

constexpr ServoAddr rightfront_H = { 0x60, 12 };
constexpr ServoAddr rightFront_C = { 0x60, 13 };
constexpr ServoAddr rightFront_F = { 0x60, 14 };

constexpr ServoAddr rightMiddle_H = { 0x60, 9 };
constexpr ServoAddr rightMiddle_C = { 0x60, 10 };
constexpr ServoAddr rightMiddle_F = { 0x60, 8 };

constexpr ServoAddr rightBack_H = { 0x40, 12 };
constexpr ServoAddr rightBack_C = { 0x40, 14 };
constexpr ServoAddr rightBack_F = { 0x40, 13 };

// U-S sensor
constexpr int TRIG_PIN = 4;
constexpr int ECHO_PIN = 5;

//Flame sensor
constexpr int FIRE_SENSOR_PIN = 15;

// 12V Driver
constexpr int DRILL_INA_PIN = 13;
constexpr int DRILL_INB_PIN = 33;
constexpr int DRILL_PWM_PIN = 27;

// Seeder driver
constexpr int SEEDER_IN1_PIN = 19;
constexpr int SEEDER_IN2_PIN = 21;
constexpr int SEEDER_IN3_PIN = 14;
constexpr int SEEDER_IN4_PIN = 32;

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

constexpr const char* WIFI_AP_SSID = "SpiderRobot_102B";
constexpr const char* WIFI_AP_PASSWORD = "spider102B";

#endif