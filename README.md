# 102B Spider Robot Control System

## Overview

This project is a UC Berkeley ME 102B mechatronics capstone focused on developing a spider-style robotic system for automated seed planting. The system integrates a Python-based GUI with an ESP32 microcontroller to control actuators, process sensor data, and execute a structured state machine.

The architecture separates high-level control from low-level hardware execution, making the project easier to debug, expand, and collaborate on.

## System Architecture

```text
GUI (Python / PySide6)
        ↓ Serial USB
ESP32 (C++ / PlatformIO)
        ↓
Hardware (Servos, DC Motor, Stepper Motor, Sensors)
```

## Folder Structure

```text
gui/
├── main.py
├── requirements.txt
└── assets/
    └── logo.png

src/
├── main.cpp
├── Config.h
│
├── control/
│   ├── SerialCommand.cpp
│   ├── SerialCommand.h
│   ├── StateMachine.cpp
│   └── StateMachine.h
│
├── motion/
│   ├── LegController.cpp
│   └── LegController.h
│
├── sensors/
│   ├── DistanceSensor.cpp
│   ├── DistanceSensor.h
│   ├── FireSensor.cpp
│   └── FireSensor.h
│
└── actuators/
    ├── DrillMotor.cpp
    ├── DrillMotor.h
    ├── SeederMotor.cpp
    └── SeederMotor.h
```

## Workflow

### 1. GUI Sends Commands

The user interacts with the Python GUI. When a button is clicked, the GUI sends a serial command to the ESP32.

Example commands:

```text
STATE:IDLE
STATE:WALKING
STATE:PRE_PLANTING
STATE:PLANTING
ABORT
MOVE:FORWARD
MOVE:STOP
DRILL:ON
DRILL:OFF
SEEDER:ON
SEEDER:OFF
```

### 2. ESP32 Parses Commands

`SerialCommand.cpp` reads incoming serial commands and routes them to the correct system behavior.

Example:

```text
DRILL:ON → turns drill motor on
STATE:PRE_PLANTING → enters pre-planting state
ABORT → stops all systems
```

### 3. State Machine Controls Behavior

`StateMachine.cpp` manages the major robot states.

| State | Description |
|---|---|
| `IDLE` | Robot is stopped and waiting for input. |
| `WALKING` | Robot moves using servo-controlled legs. |
| `PRE_PLANTING` | Ultrasonic sensor checks if the drill is close enough to the ground. |
| `PLANTING` | Drill stops and the seeder stepper motor activates. |
| `ABORT` | Emergency stop. All actuators should stop immediately. |

### 4. ESP32 Sends Feedback to GUI

The ESP32 can send status messages back to the GUI.

Example messages:

```text
DISTANCE:8.5
DRILL_ACK:ON
DRILL_ACK:OFF
SEEDER_DONE
STATE_ACK:WALKING
ALERT:ABORT_TRIGGERED
```

The GUI displays these messages in the serial log and can later use them to update dashboard values.

## Subsystem Responsibilities

### GUI

Located in:

```text
gui/main.py
```

Responsible for:

- Serial connection to ESP32
- Sending robot commands
- Displaying current state
- Displaying sensor and actuator status
- Logging ESP32 messages

### Control

Located in:

```text
src/control/
```

Responsible for:

- Parsing serial commands
- Updating robot state
- Managing the state machine

### Motion

Located in:

```text
src/motion/
```

Responsible for:

- Servo-based leg movement
- Walking gait behavior

### Sensors

Located in:

```text
src/sensors/
```

Responsible for:

- Ultrasonic distance sensing
- Fire sensor safety input

### Actuators

Located in:

```text
src/actuators/
```

Responsible for:

- Drill DC motor control
- Seeder stepper motor control

## Planting Logic

The robot follows this general planting sequence:

```text
IDLE
  ↓
WALKING
  ↓
PRE_PLANTING
  ↓
If ultrasonic distance is close enough:
  Drill turns on
  ↓
PLANTING
  Drill turns off
  Seeder stepper activates
  ↓
WALKING
```

The ultrasonic sensor no longer decides whether the robot should walk. Instead, it determines whether the drill is at the correct height from the ground during `PRE_PLANTING`.

## Safety Logic

The `ABORT` state should be reachable from any state.

When `ABORT` is triggered:

- Leg movement stops
- Drill motor turns off
- Seeder motor stops/releases
- Robot remains stopped until reset to `IDLE`

Future safety improvements may include:

- Fire sensor auto-abort
- Serial connection loss detection
- Invalid ultrasonic reading detection

## Running the GUI

From the main project folder:

```bash
cd gui
pip install -r requirements.txt
python main.py
```

## Building and Uploading ESP32 Code

Using PlatformIO:

```bash
pio run
pio run --target upload
```

## Git Workflow

Before making changes:

```bash
git pull
```

After making changes:

```bash
git status
git add .
git commit -m "Describe the change"
git push
```

## Notes

This project uses a modular structure so each subsystem can be developed separately. The GUI handles user interaction, while the ESP32 handles real-time robot control and hardware execution.
