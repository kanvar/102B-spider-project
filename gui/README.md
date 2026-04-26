# 102B Spider Robot GUI

This folder contains the Python GUI for controlling the 102B Spider Robot over a serial connection.

The GUI can:
- Connect to the ESP32 through a selected serial port
- Send movement commands
- Turn the drill on and off
- Turn the seed dispenser on and off
- Send an emergency stop command
- Display serial messages from the ESP32
- Toggle between light mode and dark mode

## Folder Contents

```text
gui/
├── main.py
├── requirements.txt
├── README.md
└── assets/