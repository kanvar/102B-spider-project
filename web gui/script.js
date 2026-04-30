'use strict';

// =============================================================
// CONFIG
// =============================================================

const ESP32_IP          = () => document.getElementById('esp32-ip')?.value?.trim() || '192.168.4.1';
const COMMAND_URL       = () => `http://${ESP32_IP()}/command`;
const STATUS_URL        = () => `http://${ESP32_IP()}/status`;
const POLL_INTERVAL_MS  = 1500;   // how often we poll /status

// =============================================================
// STATE
// =============================================================

let currentState  = 'IDLE';
let abortActive   = false;
let pollTimer     = null;
let isConnected   = false;
let lastPollTime  = 0;

// State descriptions matching the actual StateMachine.cpp behavior
const STATE_DESCRIPTIONS = {
  IDLE:     'Servos zeroed · Ultrasonic tared · Waiting for trigger',
  DRILLING: 'Drill on · Descending legs · Auto-lifting when at ground',
  SEEDING:  'Seeder motor running · Dropping seed into hole',
  COVERING: 'Right-middle leg sweeping · Covering hole (2 reps)',
  ABORT:    'Emergency stop · All actuators disabled · Awaiting reset',
};

// =============================================================
// INIT
// =============================================================

window.addEventListener('DOMContentLoaded', () => {
  setupButtons();
  setupIpField();
  renderState('IDLE');
  addLog('sys', 'GUI loaded — connect to SpiderRobot_102B Wi-Fi then click Start Sequence');
  addLog('sys', 'Polling /status every 1.5 s once connected');
  startPolling();
});

// =============================================================
// IP FIELD
// =============================================================

function setupIpField() {
  // If there's no IP field in the HTML, nothing breaks — we just use the default
  const field = document.getElementById('esp32-ip');
  if (!field) return;
  field.value = '192.168.4.1';
  field.addEventListener('change', () => {
    addLog('sys', `ESP32 IP updated to ${field.value.trim()}`);
  });
}

// =============================================================
// BUTTON WIRING
// =============================================================

function setupButtons() {
  // Abort button in header
  const abortBtn = document.getElementById('abortBtn');
  if (abortBtn) abortBtn.addEventListener('click', triggerAbort);

  // State buttons (left column)
  document.querySelectorAll('[data-state]').forEach(btn => {
    btn.addEventListener('click', () => {
      if (abortActive && btn.dataset.state !== 'IDLE') {
        addLog('warn', 'Clear ABORT first before changing state');
        return;
      }
      sendStateCommand(btn.dataset.state);
    });
  });

  // Sequence buttons
  document.querySelectorAll('[data-sequence]').forEach(btn => {
    btn.addEventListener('click', () => {
      if (abortActive) {
        addLog('warn', 'Clear ABORT before running sequence steps');
        return;
      }
      runSequence(btn.dataset.sequence);
    });
  });

  // Abort overlay clear button (wired via onclick in HTML, but also here for safety)
  const clearBtn = document.querySelector('.overlay-clear-btn');
  if (clearBtn) clearBtn.addEventListener('click', clearAbort);
}

// =============================================================
// POLLING /status
// =============================================================

function startPolling() {
  if (pollTimer) clearInterval(pollTimer);
  pollTimer = setInterval(pollStatus, POLL_INTERVAL_MS);
}

async function pollStatus() {
  try {
    const res = await fetch(STATUS_URL(), {
      method: 'GET',
      cache: 'no-store',
      signal: AbortSignal.timeout(1200),
    });

    if (!res.ok) throw new Error(`HTTP ${res.status}`);

    const data = await res.json();
    lastPollTime = Date.now();

    if (!isConnected) {
      isConnected = true;
      setConnectionPill('CONNECTED');
      addLog('rx', 'ESP32 connected');
    }

    handleStatusPayload(data);

  } catch (err) {
    if (isConnected) {
      isConnected = false;
      setConnectionPill('OFFLINE');
      addLog('err', `Lost connection: ${err.message}`);
    }
    setText('wifiValue', 'OFFLINE');
  }
}

// Handle the JSON object from /status
// Expected shape from WiFiCommand.cpp:
// {
//   "state": "IDLE",
//   "distance": 12.4,
//   "fire": false,
//   "drill": false,
//   "seeder": false
// }
function handleStatusPayload(data) {
  // State sync — the ESP32 is authoritative
  if (data.state && data.state !== currentState) {
    addLog('rx', `STATE_ACK: ${data.state}`);
    applyStateChange(data.state);
  }

  // Distance
  if (data.distance !== undefined && data.distance !== null) {
    const cm = parseFloat(data.distance);
    if (!isNaN(cm)) {
      setText('distanceValue', `${cm.toFixed(1)} cm`);
    }
  }

  // Fire sensor
  if (data.fire === true) {
    handleFireAlert();
  } else {
    const fireEl = document.getElementById('fireValue');
    if (fireEl) fireEl.innerHTML = '<span class="fire-safe">● SAFE</span>';
  }

  // Drill
  if (data.drill !== undefined) {
    setText('drillValue', data.drill ? 'ON' : 'OFF');
  }

  // Seeder
  if (data.seeder !== undefined) {
    setText('seederValue', data.seeder ? 'RUNNING' : 'IDLE');
  }

  setText('wifiValue', 'CONNECTED');

  // Update telem age
  const ageEl = document.getElementById('telem-age');
  if (ageEl) ageEl.textContent = `LIVE · ${new Date().toLocaleTimeString()}`;
}

// =============================================================
// SEND COMMAND  →  GET /command?cmd=...
// =============================================================

async function sendCommand(command) {
  setText('lastCommand', command);
  addLog('tx', `→ ${command}`);

  try {
    const url = `${COMMAND_URL()}?cmd=${encodeURIComponent(command)}`;
    const res = await fetch(url, {
      method: 'GET',
      cache: 'no-store',
      signal: AbortSignal.timeout(2000),
    });

    if (!res.ok) throw new Error(`HTTP ${res.status}`);

    addLog('rx', `ACK ← ${command}`);
    if (!isConnected) {
      isConnected = true;
      setConnectionPill('CONNECTED');
    }

  } catch (err) {
    addLog('err', `Send failed (${command}): ${err.message}`);
    setConnectionPill('OFFLINE');
    isConnected = false;
  }
}

// =============================================================
// STATE CONTROL
// =============================================================

function sendStateCommand(state) {
  sendCommand(`STATE:${state}`);
  // Optimistic local update — /status poll will correct if ESP32 disagrees
  applyStateChange(state);
}

function applyStateChange(state) {
  currentState = state;
  renderState(state);

  if (state === 'ABORT') {
    abortActive = true;
    showAbortOverlay();
  }
}

function renderState(state) {
  // Badge
  const badge = document.getElementById('currentState');
  if (badge) {
    badge.textContent = state;
    badge.className   = 'badge-value state-' + state.toLowerCase();
  }

  // Description
  const desc = document.getElementById('badge-desc');
  if (desc) desc.textContent = STATE_DESCRIPTIONS[state] || '';

  // Highlight active state button
  document.querySelectorAll('[data-state]').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.state === state);
  });

  // Footer mode
  setText('modeFooter', state);

  // Telemetry contextual updates
  applyTelemForState(state);
}

function applyTelemForState(state) {
  switch (state) {
    case 'IDLE':
      setText('drillValue',  'OFF');
      setText('seederValue', 'IDLE');
      setText('coverValue',  'READY');
      setText('safetyValue', 'ACTIVE');
      break;
    case 'DRILLING':
      setText('drillValue',  'ON');
      setText('seederValue', 'IDLE');
      setText('coverValue',  'WAITING');
      break;
    case 'SEEDING':
      setText('drillValue',  'OFF');
      setText('seederValue', 'RUNNING');
      setText('coverValue',  'WAITING');
      break;
    case 'COVERING':
      setText('drillValue',  'OFF');
      setText('seederValue', 'DONE');
      setText('coverValue',  'SWEEPING');
      break;
    case 'ABORT':
      setText('drillValue',  'OFF');
      setText('seederValue', 'OFF');
      setText('coverValue',  'STOPPED');
      setText('safetyValue', 'ABORT ACTIVE');
      break;
  }
}

// =============================================================
// SEQUENCE BUTTONS
// The sequence is mostly automatic on the ESP32 side now.
// START_PLANTING_SEQUENCE is the main one — everything else
// auto-advances: DRILLING → SEEDING → COVERING → IDLE
// The manual steps are kept for override/debug use.
// =============================================================

function runSequence(step) {
  if (abortActive) {
    addLog('warn', 'Clear ABORT before running sequence');
    return;
  }

  // Optimistic UI hints for each step
  switch (step) {
    case 'START_PLANTING_SEQUENCE':
      addLog('sys', 'Starting planting sequence — robot will auto-advance through states');
      applyStateChange('DRILLING');
      break;
    case 'CHECK_HEIGHT':
      addLog('sys', 'Checking ultrasonic height');
      break;
    case 'BEGIN_DRILLING':
      addLog('sys', 'Requesting drill on');
      setText('drillValue', 'ON');
      break;
    case 'DRILL_COMPLETE':
      addLog('sys', 'Marking drill complete → SEEDING');
      applyStateChange('SEEDING');
      break;
    case 'DROP_SEED':
      addLog('sys', 'Seeder running');
      setText('seederValue', 'RUNNING');
      break;
    case 'COVER_SEED':
      addLog('sys', 'Covering seed → COVERING');
      applyStateChange('COVERING');
      break;
    case 'RETURN_HOME':
      addLog('sys', 'Returning to IDLE');
      applyStateChange('IDLE');
      break;
    default:
      addLog('warn', `Unknown sequence step: ${step}`);
  }

  sendCommand(`SEQUENCE:${step}`);
}

// =============================================================
// ABORT
// =============================================================

function triggerAbort() {
  abortActive = true;
  sendCommand('ABORT');
  applyStateChange('ABORT');
  addLog('err', 'ABORT triggered — all actuators stopping');
}

function clearAbort() {
  abortActive = false;
  hideAbortOverlay();
  sendCommand('STATE:IDLE');
  applyStateChange('IDLE');
  addLog('sys', 'Abort cleared — returned to IDLE');
}

function showAbortOverlay() {
  const overlay = document.getElementById('abort-overlay');
  if (overlay) overlay.classList.remove('hidden');
}

function hideAbortOverlay() {
  const overlay = document.getElementById('abort-overlay');
  if (overlay) overlay.classList.add('hidden');
}

// =============================================================
// FIRE / SAFETY ALERTS
// =============================================================

function handleFireAlert() {
  const fireEl = document.getElementById('fireValue');
  if (fireEl) fireEl.innerHTML = '<span class="fire-alert">● ALERT</span>';
  setText('safetyValue', 'FIRE DETECTED');

  if (!abortActive) {
    addLog('err', 'FIRE SENSOR ALERT — triggering abort');
    triggerAbort();
  }
}

// =============================================================
// CONNECTION PILL
// =============================================================

function setConnectionPill(status) {
  const pill = document.getElementById('connectionStatus');
  if (!pill) return;

  pill.innerHTML = `<span class="pill-dot"></span><span>${status}</span>`;
  pill.classList.remove('pill-online', 'pill-offline');
  pill.classList.add(status === 'CONNECTED' ? 'pill-online' : 'pill-offline');
  setText('wifiValue', status);
}

// =============================================================
// LOG
// =============================================================

const LOG_COLORS = {
  tx:   'log-tx',    // yellow — outgoing command
  rx:   'log-rx',    // green  — incoming / ack
  sys:  'log-sys',   // muted  — system info
  warn: 'log-warn',  // orange — warnings
  err:  'log-err',   // red    — errors / alerts
};

function addLog(type, message) {
  const logBox = document.getElementById('logBox');
  if (!logBox) return;

  const time = new Date().toLocaleTimeString('en-US', { hour12: false });
  const line = document.createElement('div');
  line.className = `log-line ${LOG_COLORS[type] || ''}`;
  line.textContent = `[${time}] ${message}`;

  logBox.appendChild(line);

  // Keep log from growing forever
  while (logBox.children.length > 200) {
    logBox.removeChild(logBox.firstChild);
  }

  logBox.scrollTop = logBox.scrollHeight;
}

function clearLog() {
  const logBox = document.getElementById('logBox');
  if (logBox) logBox.innerHTML = '';
  addLog('sys', 'Log cleared');
}

// =============================================================
// HELPERS
// =============================================================

function setText(id, value) {
  const el = document.getElementById(id);
  if (el) el.textContent = value;
}

// =============================================================
// KEYBOARD SHORTCUT — ESC = ABORT
// =============================================================

document.addEventListener('keydown', e => {
  if (e.key === 'Escape') {
    triggerAbort();
  }
});