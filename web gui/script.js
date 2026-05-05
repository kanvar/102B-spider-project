'use strict';

// =============================================================
// CONFIG
// =============================================================

const ESP32_IP         = () => document.getElementById('esp32-ip')?.value?.trim() || '192.168.4.1';
const COMMAND_URL      = () => `http://${ESP32_IP()}/command`;
const STATUS_URL       = () => `http://${ESP32_IP()}/status`;
const POLL_INTERVAL_MS = 1500;

// How long to wait before logging a "still blocking" message
// SEEDING and COVERING block the ESP32 for several seconds
const BLOCKING_WARN_MS = 8000;

// =============================================================
// STATE
// =============================================================

let currentState      = 'IDLE';
let abortActive       = false;
let pollTimer         = null;
let isConnected       = false;
let blockingStartTime = null;   // tracks when a blocking state began

const STATE_DESCRIPTIONS = {
  IDLE:     'Servos zeroed · Ultrasonic tared · Waiting for STATE:DRILLING',
  DRILLING: 'Drill ON · Coxa descending 90°→45° · Auto-lifting to baseline',
  SEEDING:  'Drill OFF · Stepper rotating 2500 steps · [ESP32 blocking — brief disconnect normal]',
  COVERING: 'Right-middle leg sweeping C+F · 2 reps · [ESP32 blocking — brief disconnect normal]',
  ABORT:    'All actuators disabled · Send STATE:IDLE to reset',
};

const VALID_STATES = ['IDLE', 'DRILLING', 'SEEDING', 'COVERING', 'ABORT'];

// =============================================================
// INIT
// =============================================================

window.addEventListener('DOMContentLoaded', () => {
  setupButtons();
  setupIpField();
  renderState('IDLE');
  addLog('sys', 'GUI loaded — connect to SpiderRobot_102B (pw: spider102B)');
  addLog('sys', 'Click START PLANTING SEQUENCE or any state button to begin.');
  addLog('sys', 'Brief disconnect during SEEDING / COVERING is normal — ESP32 will reconnect automatically.');
  startPolling();
});

// =============================================================
// IP FIELD
// =============================================================

function setupIpField() {
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
  // Header abort button
  const abortBtn = document.getElementById('abortBtn');
  if (abortBtn) abortBtn.addEventListener('click', triggerAbort);

  // State buttons — each sends STATE:X directly to ESP32
  document.querySelectorAll('[data-state]').forEach(btn => {
    btn.addEventListener('click', () => {
      const state = btn.dataset.state;
      if (!VALID_STATES.includes(state)) {
        addLog('warn', `Unknown state: ${state}`);
        return;
      }
      if (state === 'ABORT') {
        triggerAbort();
        return;
      }
      if (abortActive && state !== 'IDLE') {
        addLog('warn', 'Clear ABORT first — use CLEAR & RETURN TO IDLE');
        return;
      }
      // Send STATE:X directly — this is what SerialCommand.cpp handles
      sendStateCommand(state);
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

  // Abort overlay clear button
  const clearBtn = document.querySelector('.overlay-clear-btn');
  if (clearBtn) clearBtn.addEventListener('click', clearAbort);
}

// =============================================================
// POLLING /status  (every 1.5 s)
// =============================================================

function startPolling() {
  if (pollTimer) clearInterval(pollTimer);
  pollTimer = setInterval(pollStatus, POLL_INTERVAL_MS);
}

async function pollStatus() {
  try {
    const res = await fetch(STATUS_URL(), {
      method: 'GET',
      cache:  'no-store',
      signal: AbortSignal.timeout(1200),
    });

    if (!res.ok) throw new Error(`HTTP ${res.status}`);

    const data = await res.json();

    // Reconnected after blocking state
    if (!isConnected) {
      isConnected = true;
      blockingStartTime = null;
      setConnectionPill('CONNECTED');
      addLog('rx', 'ESP32 reconnected');
    }

    handleStatusPayload(data);

  } catch (err) {
    if (isConnected) {
      isConnected = false;
      setConnectionPill('OFFLINE');

      // Only log alarming message if not in a known blocking state
      if (currentState === 'SEEDING' || currentState === 'COVERING') {
        if (!blockingStartTime) blockingStartTime = Date.now();
        addLog('sys', `ESP32 executing ${currentState} — polling paused, will auto-reconnect`);
      } else {
        blockingStartTime = null;
        addLog('err', `Connection lost: ${err.message}`);
      }
    } else if (blockingStartTime) {
      // Already offline in blocking state — log a reminder if taking very long
      const elapsed = Date.now() - blockingStartTime;
      if (elapsed > BLOCKING_WARN_MS && elapsed < BLOCKING_WARN_MS + POLL_INTERVAL_MS) {
        addLog('warn', `${currentState} still running (${Math.round(elapsed/1000)}s) — waiting for ESP32...`);
      }
    }

    setText('wifiValue', 'OFFLINE');
  }
}

// Expected /status JSON: { "state": "IDLE", "distance": 12.4, "drill": false, "seeder": false }
function handleStatusPayload(data) {
  // ESP32 is authoritative — sync GUI state if it drifted
  if (data.state && VALID_STATES.includes(data.state) && data.state !== currentState) {
    addLog('rx', `State sync: ${currentState} → ${data.state}`);
    applyStateChange(data.state);
  }

  // Distance
  if (data.distance !== undefined && data.distance !== null) {
    const cm = parseFloat(data.distance);
    if (!isNaN(cm) && cm > 0) {
      setText('distanceValue', `${cm.toFixed(1)} cm`);
      const bar = document.getElementById('distBar');
      if (bar) bar.style.width = `${Math.min(100, (cm / 30) * 100)}%`;
    }
  }

  // Drill motor
  if (data.drill !== undefined) {
    setText('drillValue', data.drill ? 'ON' : 'OFF');
    const tcDrill = document.getElementById('tc-drill');
    if (tcDrill) tcDrill.classList.toggle('active', data.drill);
  }

  // Seeder motor
  if (data.seeder !== undefined) {
    setText('seederValue', data.seeder ? 'RUNNING' : 'IDLE');
    const tcSeeder = document.getElementById('tc-seeder');
    if (tcSeeder) tcSeeder.classList.toggle('active', data.seeder);
  }

  setText('wifiValue', 'CONNECTED');

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
      cache:  'no-store',
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
// All state changes go through STATE:X — this is what
// SerialCommand.cpp's setRobotState() responds to.
// =============================================================

function sendStateCommand(state) {
  sendCommand(`STATE:${state}`);
  applyStateChange(state);        // optimistic UI update
}

function applyStateChange(state) {
  currentState = state;
  renderState(state);

  // Track blocking state start time for disconnect handling
  if (state === 'SEEDING' || state === 'COVERING') {
    blockingStartTime = Date.now();
  } else {
    blockingStartTime = null;
  }

  if (state === 'ABORT') {
    abortActive = true;
    showAbortOverlay();
  }
}

function renderState(state) {
  const badge = document.getElementById('currentState');
  if (badge) {
    badge.textContent = state;
    badge.className   = 'badge-value state-' + state.toLowerCase();
  }

  const desc = document.getElementById('badge-desc');
  if (desc) desc.textContent = STATE_DESCRIPTIONS[state] || '';

  document.querySelectorAll('[data-state]').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.state === state);
  });

  setText('modeFooter', state);
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
      setText('safetyValue', 'ACTIVE');
      break;
    case 'SEEDING':
      setText('drillValue',  'OFF');
      setText('seederValue', 'RUNNING');
      setText('coverValue',  'WAITING');
      setText('safetyValue', 'ACTIVE');
      break;
    case 'COVERING':
      setText('drillValue',  'OFF');
      setText('seederValue', 'DONE');
      setText('coverValue',  'SWEEPING');
      setText('safetyValue', 'ACTIVE');
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
// START_PLANTING_SEQUENCE sends STATE:DRILLING directly —
// the ESP32 auto-advances DRILLING → SEEDING → COVERING → IDLE.
// All other buttons are manual overrides that also use STATE:X.
// =============================================================

function runSequence(step) {
  if (abortActive) {
    addLog('warn', 'Clear ABORT before running sequence steps');
    return;
  }

  switch (step) {
    case 'START_PLANTING_SEQUENCE':
      addLog('sys', 'Starting sequence — robot will auto-advance DRILLING → SEEDING → COVERING → IDLE');
      sendStateCommand('DRILLING');   // STATE:DRILLING triggers the full auto sequence
      return;

    case 'CHECK_HEIGHT':
      addLog('sys', 'Manual: checking ultrasonic height');
      sendCommand('SEQUENCE:CHECK_HEIGHT');
      return;

    case 'BEGIN_DRILLING':
      addLog('sys', 'Manual override: begin drilling');
      sendStateCommand('DRILLING');
      return;

    case 'DRILL_COMPLETE':
      addLog('sys', 'Manual override: advance to SEEDING');
      sendStateCommand('SEEDING');
      return;

    case 'DROP_SEED':
      addLog('sys', 'Manual override: advance to SEEDING');
      sendStateCommand('SEEDING');
      return;

    case 'COVER_SEED':
      addLog('sys', 'Manual override: advance to COVERING');
      sendStateCommand('COVERING');
      return;

    case 'RETURN_HOME':
      addLog('sys', 'Manual override: return to IDLE');
      sendStateCommand('IDLE');
      return;

    default:
      addLog('warn', `Unknown sequence step: ${step}`);
  }
}

// =============================================================
// ABORT
// =============================================================

function triggerAbort() {
  abortActive = true;
  blockingStartTime = null;
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
  tx:   'log-tx',
  rx:   'log-rx',
  sys:  'log-sys',
  warn: 'log-warn',
  err:  'log-err',
};

function addLog(type, message) {
  const logBox = document.getElementById('logBox');
  if (!logBox) return;

  const time = new Date().toLocaleTimeString('en-US', { hour12: false });
  const line = document.createElement('div');
  line.className = `log-line ${LOG_COLORS[type] || ''}`;
  line.textContent = `[${time}] ${message}`;

  logBox.appendChild(line);

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
// KEYBOARD — ESC = ABORT
// =============================================================

document.addEventListener('keydown', e => {
  if (e.key === 'Escape') triggerAbort();
});