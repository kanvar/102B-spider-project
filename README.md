# 102B Spider Robot Control System

UC Berkeley ME 102B capstone: a spider-style robot for automated seed planting. A Python GUI talks to an ESP32 over serial; the ESP32 runs the state machine and drives the hardware.

## Architecture

```
GUI (Python / PySide6)
      ↓ Serial USB
ESP32 (C++ / PlatformIO)
      ↓
Servos · DC Motor · Stepper · Sensors
```

## Folder Structure

```
gui/
├── main.py
├── requirements.txt
└── assets/logo.png

src/
├── main.cpp
├── Config.h
├── control/      # SerialCommand, StateMachine
├── motion/       # LegController
├── sensors/      # DistanceSensor, FireSensor
└── actuators/    # DrillMotor, SeederMotor
```

## Serial Protocol

**GUI → ESP32**

```
STATE:IDLE | WALKING | PRE_PLANTING | PLANTING
ABORT
MOVE:FORWARD | STOP
DRILL:ON | OFF
SEEDER:ON | OFF
```

**ESP32 → GUI**

```
DISTANCE:<cm>
DRILL_ACK:ON | OFF
SEEDER_DONE
STATE_ACK:<state>
ALERT:ABORT_TRIGGERED
```
