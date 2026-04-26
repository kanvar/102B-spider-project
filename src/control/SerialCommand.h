#pragma once

// Reads incoming commands from GUI over serial
void checkSerialCommand();

// Handles one command string from the GUI
void handleSerialCommand(String cmd);