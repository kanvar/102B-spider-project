let currentState = "IDLE";
let abortActive = false;

// Later this can become your ESP32 WiFi endpoint.
// Example: const ESP32_COMMAND_URL = "http://192.168.4.1/command";
const ESP32_COMMAND_URL = "http://192.168.4.1/command";

// ---------- STARTUP ----------
window.addEventListener("DOMContentLoaded", () => {
  setupButtons();

  updateStatus({
    state: "IDLE",
    distance: "--",
    drill: "OFF",
    seeder: "IDLE",
    cover: "READY",
    fire: "SAFE",
    safety: "ACTIVE",
    wifi: "WAITING",
  });

  updateStateButtons("IDLE");
  addLog("Seeder web GUI loaded.");
  addLog("Safety monitor expected to run continuously on ESP32.");
});

// ---------- BUTTON SETUP ----------
function setupButtons() {
  const abortBtn = document.getElementById("abortBtn");
  if (abortBtn) abortBtn.addEventListener("click", triggerAbort);

  document.querySelectorAll("[data-state]").forEach((button) => {
    button.addEventListener("click", () => {
      setState(button.dataset.state);
    });
  });

  document.querySelectorAll("[data-sequence]").forEach((button) => {
    button.addEventListener("click", () => {
      runSequence(button.dataset.sequence);
    });
  });
}

// ---------- COMMAND SENDING ----------
// ---------- COMMAND SENDING ----------
async function sendCommand(command) {
  updateLastCommand(command);
  addLog(`TX → ${command}`);

  const ESP32_COMMAND_URL = "http://192.168.4.1/command";

  try {
    const url = `${ESP32_COMMAND_URL}?cmd=${encodeURIComponent(command)}`;

    const response = await fetch(url, {
      method: "GET",
      cache: "no-store",
    });

    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }

    addLog(`WiFi sent successfully → ${command}`);
    updateStatus({ wifi: "CONNECTED" });
  } catch (error) {
    addLog(`WiFi send error: ${error.message}`);
    updateStatus({ wifi: "ERROR" });
  }
}
// ---------- STATE CONTROL ----------
function setState(state) {
  if (abortActive && state !== "IDLE") {
    addLog("Cannot change state while ABORT is active. Clear abort first.");
    return;
  }

  currentState = state;

  updateStatus({ state });
  updateStateButtons(state);
  updateStateDescription(state);

  sendCommand(`STATE:${state}`);
}

function updateStateButtons(activeState) {
  document.querySelectorAll("[data-state]").forEach((button) => {
    button.classList.toggle("active", button.dataset.state === activeState);
  });
}

function updateStateDescription(state) {
  const desc = document.getElementById("badge-desc");
  if (!desc) return;

  const descriptions = {
    IDLE: "Power on · Zero servos · Zero ultrasonic",
    DRILLING: "Reading height · Lowering legs · Drilling when height is correct",
    SEEDING: "Move forward · Drop seed · Move sideways",
    COVERING: "Check hole distance · Scoop soil · Repeat covering motion",
    RETURN_HOME: "Safety or sequence complete · Returning to home position",
    ABORT: "Emergency stop active · All actuators disabled",
  };

  desc.textContent = descriptions[state] || "State updated";
}

// ---------- SEQUENCE CONTROL ----------
function runSequence(step) {
  if (abortActive) {
    addLog("Sequence blocked because ABORT is active.");
    return;
  }

  const command = `SEQUENCE:${step}`;

  switch (step) {
    case "START_PLANTING_SEQUENCE":
      currentState = "DRILLING";
      updateStatus({
        state: "DRILLING",
        drill: "ARMED",
        seeder: "IDLE",
        cover: "READY",
      });
      break;

    case "CHECK_HEIGHT":
      updateStatus({
        state: "DRILLING",
        drill: "WAITING_HEIGHT",
      });
      break;

    case "BEGIN_DRILLING":
      updateStatus({
        state: "DRILLING",
        drill: "ON",
      });
      break;

    case "DRILL_COMPLETE":
      currentState = "SEEDING";
      updateStatus({
        state: "SEEDING",
        drill: "OFF",
        seeder: "READY",
      });
      break;

    case "DROP_SEED":
      currentState = "SEEDING";
      updateStatus({
        state: "SEEDING",
        seeder: "DROPPING",
      });
      break;

    case "MOVE_SIDEWAYS":
      updateStatus({
        state: "SEEDING",
        cover: "MOVING_SIDEWAYS",
      });
      break;

    case "COVER_SEED":
      currentState = "COVERING";
      updateStatus({
        state: "COVERING",
        cover: "SCOOPING",
      });
      break;

    case "RETURN_HOME":
      currentState = "RETURN_HOME";
      updateStatus({
        state: "RETURN_HOME",
        drill: "OFF",
        seeder: "IDLE",
        cover: "RETURNING",
      });
      break;

    default:
      addLog(`Unknown sequence step: ${step}`);
      break;
  }

  updateStateButtons(currentState);
  updateStateDescription(currentState);
  sendCommand(command);
}

// ---------- SAFETY ----------
function triggerAbort() {
  abortActive = true;
  currentState = "ABORT";

  updateStatus({
    state: "ABORT",
    drill: "OFF",
    seeder: "OFF",
    cover: "STOPPED",
    safety: "ABORT_ACTIVE",
  });

  updateStateButtons("ABORT");
  updateStateDescription("ABORT");

  const overlay = document.getElementById("abort-overlay");
  if (overlay) overlay.classList.remove("hidden");

  sendCommand("ABORT");
  addLog("ABORT triggered.");
}

function clearAbort() {
  abortActive = false;

  const overlay = document.getElementById("abort-overlay");
  if (overlay) overlay.classList.add("hidden");

  setState("IDLE");
  updateStatus({
    drill: "OFF",
    seeder: "IDLE",
    cover: "READY",
    safety: "ACTIVE",
  });

  addLog("Abort cleared. Returned to IDLE.");
}

function handleFireAlert() {
  updateStatus({
    fire: "ALERT",
    safety: "FIRE_DETECTED",
  });

  addLog("FIRE SENSOR ALERT. Returning home / abort sequence required.");
  sendCommand("SAFETY:FIRE_DETECTED");
  triggerAbort();
}

function handleJamAlert() {
  updateStatus({
    safety: "JAM_DETECTED",
  });

  addLog("Potential jam detected. Returning home.");
  sendCommand("SAFETY:JAM_DETECTED");
  setState("RETURN_HOME");
}

// ---------- STATUS UPDATE ----------
function updateStatus(data) {
  if (data.state !== undefined) {
    setText("currentState", data.state);
  }

  if (data.distance !== undefined) {
    const value = data.distance === "--" ? "-- cm" : `${data.distance} cm`;
    setText("distanceValue", value);
  }

  if (data.drill !== undefined) {
    setText("drillValue", data.drill);
  }

  if (data.seeder !== undefined) {
    setText("seederValue", data.seeder);
  }

  if (data.cover !== undefined) {
    setText("coverValue", data.cover);
  }

  if (data.safety !== undefined) {
    setText("safetyValue", data.safety);
  }

  if (data.fire !== undefined) {
    const fireEl = document.getElementById("fireValue");

    if (fireEl) {
      if (data.fire === "ALERT") {
        fireEl.innerHTML = `<span class="fire-alert">● ALERT</span>`;
      } else {
        fireEl.innerHTML = `<span class="fire-safe">● SAFE</span>`;
      }
    }
  }

  if (data.wifi !== undefined) {
    setText("wifiValue", data.wifi);
    updateConnectionStatus(data.wifi);
  }
}

function updateConnectionStatus(status) {
  const pill = document.getElementById("connectionStatus");
  if (!pill) return;

  pill.innerHTML = `<span class="pill-dot"></span><span>${status}</span>`;

  pill.classList.remove("pill-online", "pill-offline");

  if (status === "CONNECTED") {
    pill.classList.add("pill-online");
  } else {
    pill.classList.add("pill-offline");
  }
}

function updateLastCommand(command) {
  setText("lastCommand", command);
}

function setText(id, value) {
  const el = document.getElementById(id);
  if (el) el.textContent = value;
}

// ---------- ESP32 MESSAGE PARSING ----------
function parseESP32Message(message) {
  addLog(`RX ← ${message}`);

  if (message.startsWith("DISTANCE:")) {
    updateStatus({ distance: message.replace("DISTANCE:", "") });
  }

  else if (message.startsWith("STATE_ACK:")) {
    const state = message.replace("STATE_ACK:", "");
    currentState = state;
    updateStatus({ state });
    updateStateButtons(state);
    updateStateDescription(state);
  }

  else if (message === "DRILL_ACK:ON") {
    updateStatus({ drill: "ON" });
  }

  else if (message === "DRILL_ACK:OFF") {
    updateStatus({ drill: "OFF" });
  }

  else if (message === "SEEDER_DONE") {
    updateStatus({ seeder: "DONE" });
  }

  else if (message === "COVERING_DONE") {
    updateStatus({ cover: "DONE" });
  }

  else if (message === "RETURN_HOME_DONE") {
    currentState = "IDLE";
    updateStatus({
      state: "IDLE",
      cover: "READY",
      safety: "ACTIVE",
    });
  }

  else if (message === "ALERT:FIRE_DETECTED") {
    handleFireAlert();
  }

  else if (message === "ALERT:JAM_DETECTED") {
    handleJamAlert();
  }
}

// ---------- LOGGING ----------
function addLog(message) {
  const logBox = document.getElementById("logBox");
  if (!logBox) return;

  const time = new Date().toLocaleTimeString();

  const line = document.createElement("div");
  line.className = "log-line";
  line.textContent = `[${time}] ${message}`;

  logBox.appendChild(line);
  logBox.scrollTop = logBox.scrollHeight;
}

function clearLog() {
  const logBox = document.getElementById("logBox");
  if (logBox) logBox.innerHTML = "";

  addLog("Log cleared.");
}

// ---------- KEYBOARD SAFETY ----------
document.addEventListener("keydown", (event) => {
  if (event.key === "Escape") {
    triggerAbort();
  }
});