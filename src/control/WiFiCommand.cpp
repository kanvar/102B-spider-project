#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "WiFiCommand.h"
#include "SerialCommand.h"
#include "StateMachine.h"
#include "../Config.h"
#include "../sensors/DistanceSensor.h"
#include "../sensors/FireSensor.h"

WebServer server(80);

void sendCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

String extractCommandFromJson(String body) {
  int keyIndex = body.indexOf("\"command\"");
  if (keyIndex == -1) return "";

  int colonIndex = body.indexOf(":", keyIndex);
  if (colonIndex == -1) return "";

  int firstQuote = body.indexOf("\"", colonIndex);
  if (firstQuote == -1) return "";

  int secondQuote = body.indexOf("\"", firstQuote + 1);
  if (secondQuote == -1) return "";

  return body.substring(firstQuote + 1, secondQuote);
}

void handleRoot() {
  sendCorsHeaders();
  server.send(200, "text/plain", "Spider Robot ESP32 WiFi Server Running");
}

void handleOptions() {
  sendCorsHeaders();
  server.send(204);
}

void handleCommandRequest() {
  sendCorsHeaders();

  String command = "";

  if (server.hasArg("cmd")) {
    command = server.arg("cmd");
  } else {
    String body = server.arg("plain");
    command = extractCommandFromJson(body);
  }

  command.trim();

  if (command.length() == 0) {
    server.send(400, "application/json",
      "{\"status\":\"error\",\"message\":\"No command found\"}");
    return;
  }

  Serial.print("WIFI_RX:");
  Serial.println(command);

  handleSerialCommand(command);

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// =================================================================
// /status — polled by the GUI every 1.5 s
// Returns a JSON snapshot of current robot state
// =================================================================

void handleStatusRequest() {
  sendCorsHeaders();

  float distance = getLastDistance();   // last reading cached in DistanceSensor
  bool  fire     = isFireDetected();
  bool  drill    = (currentState == DRILLING);
  bool  seeder   = (currentState == SEEDING);

  String json = "{";
  json += "\"state\":\"";   json += stateToString(currentState); json += "\",";
  json += "\"distance\":";  json += String(distance, 1);         json += ",";
  json += "\"fire\":";      json += fire   ? "true" : "false";   json += ",";
  json += "\"drill\":";     json += drill  ? "true" : "false";   json += ",";
  json += "\"seeder\":";    json += seeder ? "true" : "false";
  json += "}";

  server.send(200, "application/json", json);
}

void wifiCommandSetup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);

  Serial.println();
  Serial.println("Receiver ESP32 WiFi AP started.");
  Serial.print("SSID: ");
  Serial.println(WIFI_AP_SSID);
  Serial.print("Password: ");
  Serial.println(WIFI_AP_PASSWORD);
  Serial.print("Receiver ESP32 IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/",               HTTP_GET,     handleRoot);
  server.on("/command",        HTTP_GET,     handleCommandRequest);
  server.on("/command",        HTTP_POST,    handleCommandRequest);
  server.on("/command",        HTTP_OPTIONS, handleOptions);
  server.on("/status",         HTTP_GET,     handleStatusRequest);   // ← new
  server.on("/status",         HTTP_OPTIONS, handleOptions);         // ← CORS preflight

  server.begin();

  Serial.println("Receiver command server started.");
}

void wifiCommandLoop() {
  server.handleClient();
}