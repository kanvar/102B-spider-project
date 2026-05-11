"""
UC Berkeley ME 102B — Spider Robot Control GUI
Version 2.0 — Redesigned Dashboard

Folder layout:
    gui/
      assets/logo.png
      main.py
      requirements.txt
      README.md

Serial command vocabulary:
    STATE:IDLE | STATE:WALKING | STATE:PRE_PLANTING | STATE:PLANTING | STATE:ABORT
    MOVE:FORWARD | MOVE:BACKWARD | MOVE:LEFT | MOVE:RIGHT | MOVE:STOP
    DRILL:ON | DRILL:OFF
    SEEDER:ON | SEEDER:OFF
    ABORT
"""

# ── Standard library ──────────────────────────────────────────────────────────
import sys
import platform
from datetime import datetime

# ── Windows taskbar app-ID fix (no-op on macOS / Linux) ──────────────────────
if platform.system() == "Windows":
    try:
        import ctypes
        ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(
            "ucberkeley.102b.spiderrobot.gui"
        )
    except Exception:
        pass

# ── Third-party ───────────────────────────────────────────────────────────────
import serial
import serial.tools.list_ports

from PySide6.QtCore import Qt, QTimer
from PySide6.QtGui import QPixmap, QIcon, QFont
from PySide6.QtWidgets import (
    QApplication, QWidget, QLabel, QPushButton,
    QTextEdit, QVBoxLayout, QHBoxLayout, QGridLayout,
    QComboBox, QMessageBox, QGroupBox, QFrame,
    QSplitter, QSizePolicy,
)

# ─────────────────────────────────────────────────────────────────────────────
#  DESIGN TOKENS  ── Edit here to retheme the entire application
# ─────────────────────────────────────────────────────────────────────────────
C: dict[str, str] = {
    # Backgrounds (darkest → lightest)
    "bg":        "#080c12",
    "surface":   "#0d1219",
    "card":      "#111820",
    "elevated":  "#161e28",
    # Borders
    "border":    "#1e2d3d",
    "border_hi": "#2a3f55",
    # Accent colours
    "cyan":      "#38bdf8",
    "green":     "#34d399",
    "amber":     "#fbbf24",
    "red":       "#f87171",
    "purple":    "#a78bfa",
    # Text
    "text":      "#dde6f0",
    "muted":     "#506070",
    "dim":       "#2a3a4a",
    # Per-state colours
    "s_idle":      "#38bdf8",
    "s_walking":   "#34d399",
    "s_preplant":  "#fbbf24",
    "s_planting":  "#a78bfa",
    "s_abort":     "#f87171",
}

# State label → accent colour
STATE_COLOR: dict[str, str] = {
    "IDLE":         C["s_idle"],
    "WALKING":      C["s_walking"],
    "PRE_PLANTING": C["s_preplant"],
    "PLANTING":     C["s_planting"],
    "ABORT":        C["s_abort"],
}


# ─────────────────────────────────────────────────────────────────────────────
#  MAIN WINDOW
# ─────────────────────────────────────────────────────────────────────────────
class ControlWindow(QWidget):

    # ── Initialisation ────────────────────────────────────────────────────────
    def __init__(self) -> None:
        super().__init__()

        # --- Serial ---
        self.ser: serial.Serial | None = None
        self.connected: bool = False

        # --- Robot state ---
        self.robot_state: str = "IDLE"

        # --- Sensor / actuator placeholders ---
        # TODO (ESP32): These are updated by update_sensor_values() once
        #               _parse_serial_line() is implemented.
        self.sensor_values: dict = {
            "drill_pct": 0,           # int   0–100 %
            "distance":  "--",        # str   distance in cm, e.g. "12.5"
            "fire":      "SAFE",      # str   "SAFE" | "ALERT"
            "seeder":    "OFF",       # str   "ON"   | "OFF"
            "legs":      "STOPPED",   # str   "FORWARD" | "BACKWARD" | …
        }

        # --- Qt object registries populated during build ---
        self._sensor_labels: dict[str, QLabel] = {}
        self._state_buttons: dict[str, QPushButton] = {}

        # --- Window chrome ---
        self.setWindowTitle("ME 102B — Spider Robot Control")
        self.setMinimumSize(1120, 740)
        self.setFocusPolicy(Qt.StrongFocus)
        self.setWindowIcon(QIcon("assets/logo.png"))

        self._build_ui()
        self._apply_stylesheet()
        self.refresh_ports()

        # Serial polling — 100 ms interval
        self._serial_timer = QTimer(self)
        self._serial_timer.timeout.connect(self._read_serial)
        self._serial_timer.start(100)

    # =========================================================================
    #  UI CONSTRUCTION
    # =========================================================================

    def _build_ui(self) -> None:
        root = QVBoxLayout(self)
        root.setContentsMargins(0, 0, 0, 0)
        root.setSpacing(0)

        root.addWidget(self._make_header())

        # Three-column body separated by a 1-px splitter handle
        splitter = QSplitter(Qt.Horizontal)
        splitter.setHandleWidth(1)
        splitter.addWidget(self._make_left_panel())
        splitter.addWidget(self._make_center_panel())
        splitter.addWidget(self._make_right_panel())
        splitter.setSizes([270, 540, 310])
        splitter.setStretchFactor(1, 1)   # center panel absorbs extra width

        root.addWidget(splitter, 1)

    # ── Header bar ────────────────────────────────────────────────────────────

    def _make_header(self) -> QFrame:
        frame = QFrame()
        frame.setObjectName("Header")
        frame.setFixedHeight(62)

        h = QHBoxLayout(frame)
        h.setContentsMargins(18, 0, 18, 0)
        h.setSpacing(14)

        # Logo
        logo = QLabel()
        px = QPixmap("assets/logo.png")
        if not px.isNull():
            logo.setPixmap(
                px.scaled(38, 38, Qt.KeepAspectRatio, Qt.SmoothTransformation)
            )
        else:
            logo.setText("🕷")
            logo.setFont(QFont("Arial", 22))
        h.addWidget(logo)

        # Title
        title = QLabel("ME 102B  ·  SPIDER ROBOT  ·  CONTROL SYSTEM")
        title.setObjectName("HeaderTitle")
        h.addWidget(title)

        h.addStretch(1)

        # Connection status pill
        self.conn_pill = QLabel("⬤  DISCONNECTED")
        self.conn_pill.setObjectName("PillOff")
        h.addWidget(self.conn_pill)

        # Visual separator
        div = QFrame()
        div.setFrameShape(QFrame.VLine)
        div.setObjectName("HDivider")
        h.addWidget(div)

        # ── ABORT — always visible, always reachable ──────────────────────
        # This button must remain accessible at all times regardless of
        # which panel or control has focus.
        self.abort_btn = QPushButton("⬛  ABORT")
        self.abort_btn.setObjectName("AbortBtn")
        self.abort_btn.setFixedSize(130, 40)
        self.abort_btn.setToolTip("Emergency stop all actuators  [Esc]")
        self.abort_btn.clicked.connect(self.trigger_abort)
        h.addWidget(self.abort_btn)

        return frame

    # ── Left panel ────────────────────────────────────────────────────────────

    def _make_left_panel(self) -> QWidget:
        w = QWidget()
        w.setObjectName("LeftPanel")
        v = QVBoxLayout(w)
        v.setContentsMargins(12, 12, 6, 12)
        v.setSpacing(10)

        v.addWidget(self._make_connection_card())
        v.addWidget(self._make_state_card())
        v.addWidget(self._make_movement_card())
        v.addStretch(1)
        return w

    def _make_connection_card(self) -> QGroupBox:
        box = QGroupBox("CONNECTION")
        v = QVBoxLayout(box)
        v.setSpacing(8)

        # Port selector + refresh
        row = QHBoxLayout()
        self.port_combo = QComboBox()
        self.port_combo.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

        self.refresh_btn = QPushButton("↺")
        self.refresh_btn.setObjectName("SmallBtn")
        self.refresh_btn.setFixedSize(32, 32)
        self.refresh_btn.setToolTip("Refresh serial ports")
        self.refresh_btn.clicked.connect(self.refresh_ports)

        row.addWidget(self.port_combo)
        row.addWidget(self.refresh_btn)
        v.addLayout(row)

        # Fixed baud rate label
        baud_lbl = QLabel("Baud rate: 115200")
        baud_lbl.setObjectName("Muted")
        v.addWidget(baud_lbl)

        # Connect / disconnect button
        self.connect_btn = QPushButton("CONNECT")
        self.connect_btn.setObjectName("ConnectBtn")
        self.connect_btn.clicked.connect(self.toggle_connection)
        v.addWidget(self.connect_btn)

        # Human-readable status line
        self.status_lbl = QLabel("No device connected")
        self.status_lbl.setObjectName("Muted")
        self.status_lbl.setWordWrap(True)
        v.addWidget(self.status_lbl)

        return box

    def _make_state_card(self) -> QGroupBox:
        """
        State-selector card.

        Each button transitions the robot to that state and sends the
        corresponding serial command.

        State machine:
          IDLE → WALKING → PRE_PLANTING → PLANTING → WALKING (loop)
          ANY  → ABORT   (always reachable via header ABORT button or Esc)

        TODO (ESP32): After sending STATE:X, firmware should echo back
                      STATE_ACK:X so the GUI can confirm the transition.
        """
        box = QGroupBox("ROBOT STATE")
        v = QVBoxLayout(box)
        v.setSpacing(6)

        state_defs = [
            ("IDLE",         "s_idle",    "STATE:IDLE"),
            ("WALKING",      "s_walking", "STATE:WALKING"),
            ("PRE_PLANTING", "s_preplant","STATE:PRE_PLANTING"),
            ("PLANTING",     "s_planting","STATE:PLANTING"),
        ]

        for label, color_key, cmd in state_defs:
            btn = QPushButton(label)
            btn.setObjectName("StateBtn")
            # Store the per-state accent colour as a custom property so the
            # active-highlight logic can read it back without a lookup table.
            btn.setProperty("activeColor", C[color_key])
            btn.setMinimumHeight(34)
            btn.clicked.connect(
                lambda _=False, lbl=label, c=cmd: self._set_state(lbl, c)
            )
            v.addWidget(btn)
            self._state_buttons[label] = btn

        # Highlight the default state on startup
        self._highlight_state_btn("IDLE")
        return box

    def _make_movement_card(self) -> QGroupBox:
        """
        D-pad manual movement controls.

        Keyboard shortcuts are mirrored in keyPressEvent():
          Arrow keys → MOVE:FORWARD/BACKWARD/LEFT/RIGHT
          Space      → MOVE:STOP
          Esc        → ABORT
          W / I / P  → WALKING / IDLE / PRE_PLANTING state shortcuts
        """
        box = QGroupBox("MANUAL CONTROL  [ ↑↓←→  SPC=stop  ESC=abort ]")
        grid = QGridLayout(box)
        grid.setSpacing(6)
        grid.setContentsMargins(16, 8, 16, 8)

        def mv_btn(symbol: str, cmd: str, obj: str = "MoveBtn") -> QPushButton:
            b = QPushButton(symbol)
            b.setObjectName(obj)
            b.setFixedSize(54, 54)
            b.clicked.connect(lambda: self._send_move(cmd))
            return b

        self.fwd_btn = mv_btn("▲", "MOVE:FORWARD")
        self.bwd_btn = mv_btn("▼", "MOVE:BACKWARD")
        self.lft_btn = mv_btn("◀", "MOVE:LEFT")
        self.rgt_btn = mv_btn("▶", "MOVE:RIGHT")
        self.stp_btn = mv_btn("■", "MOVE:STOP", "StopBtn")

        grid.addWidget(self.fwd_btn, 0, 1)
        grid.addWidget(self.lft_btn, 1, 0)
        grid.addWidget(self.stp_btn, 1, 1)
        grid.addWidget(self.rgt_btn, 1, 2)
        grid.addWidget(self.bwd_btn, 2, 1)

        return box

    # ── Centre panel ──────────────────────────────────────────────────────────

    def _make_center_panel(self) -> QWidget:
        w = QWidget()
        w.setObjectName("CenterPanel")
        v = QVBoxLayout(w)
        v.setContentsMargins(6, 12, 6, 12)
        v.setSpacing(10)

        v.addWidget(self._make_state_badge())
        v.addWidget(self._make_sensor_dashboard())
        v.addWidget(self._make_actuator_card())
        v.addStretch(1)
        return w

    def _make_state_badge(self) -> QFrame:
        """Large current-state display that changes colour with the state."""
        frame = QFrame()
        frame.setObjectName("StateBadge")
        frame.setFixedHeight(96)

        h = QHBoxLayout(frame)
        h.setContentsMargins(24, 0, 24, 0)

        left = QVBoxLayout()
        subtitle = QLabel("CURRENT STATE")
        subtitle.setObjectName("BadgeSub")
        left.addWidget(subtitle)

        self.state_badge_lbl = QLabel("IDLE")
        self.state_badge_lbl.setObjectName("BadgeMain")
        left.addWidget(self.state_badge_lbl)

        h.addLayout(left)
        h.addStretch(1)

        # Animated-style status dot (colour mirrors badge label)
        self.state_dot = QLabel("⬤")
        self.state_dot.setObjectName("StateDot")
        h.addWidget(self.state_dot)

        return frame

    def _make_sensor_dashboard(self) -> QGroupBox:
        """
        Live sensor / actuator status cards.

        Card values are placeholder strings on startup.
        TODO (ESP32): Call update_sensor_values(data_dict) from
                      _parse_serial_line() to push live values here.
        """
        box = QGroupBox("LIVE SENSORS  /  ACTUATOR STATUS")
        grid = QGridLayout(box)
        grid.setSpacing(8)

        defs = [
            ("DRILL SPEED",  "drill_pct", "0 %"),
            ("DISTANCE",     "distance",  "-- cm"),
            ("FIRE SENSOR",  "fire",      "SAFE"),
            ("SEEDER",       "seeder",    "OFF"),
            ("LEG STATUS",   "legs",      "STOPPED"),
        ]

        for idx, (title, key, init) in enumerate(defs):
            card, val_lbl = self._make_sensor_card(title, init)
            self._sensor_labels[key] = val_lbl
            grid.addWidget(card, idx // 3, idx % 3)

        return box

    def _make_sensor_card(self, title: str, init: str) -> tuple[QFrame, QLabel]:
        """Returns (card_frame, value_label) for the sensor dashboard."""
        frame = QFrame()
        frame.setObjectName("SensorCard")

        v = QVBoxLayout(frame)
        v.setContentsMargins(10, 8, 10, 8)
        v.setSpacing(2)

        t = QLabel(title)
        t.setObjectName("CardTitle")
        v.addWidget(t)

        val = QLabel(init)
        val.setObjectName("CardValue")
        v.addWidget(val)

        return frame, val

    def _make_actuator_card(self) -> QGroupBox:
        """
        Direct actuator override controls.

        These allow manual drill / seeder control independent of the
        state machine — useful for testing and calibration.

        TODO (ESP32): DRILL:ON sets the drill DC motor enable + PWM.
                      SEEDER:ON steps the stepper motor for one seed cycle.
        """
        box = QGroupBox("ACTUATOR CONTROLS")
        grid = QGridLayout(box)
        grid.setSpacing(8)

        self.drill_on_btn  = QPushButton("DRILL  ▶  ON")
        self.drill_off_btn = QPushButton("DRILL  ■  OFF")
        self.seed_on_btn   = QPushButton("SEEDER  ▶  ON")
        self.seed_off_btn  = QPushButton("SEEDER  ■  OFF")

        for b in (self.drill_on_btn, self.seed_on_btn):
            b.setObjectName("ActOnBtn")
        for b in (self.drill_off_btn, self.seed_off_btn):
            b.setObjectName("ActOffBtn")

        self.drill_on_btn.clicked.connect(lambda: self.send_command("DRILL:ON"))
        self.drill_off_btn.clicked.connect(lambda: self.send_command("DRILL:OFF"))
        self.seed_on_btn.clicked.connect(lambda: self.send_command("SEEDER:ON"))
        self.seed_off_btn.clicked.connect(lambda: self.send_command("SEEDER:OFF"))

        grid.addWidget(self.drill_on_btn,  0, 0)
        grid.addWidget(self.drill_off_btn, 0, 1)
        grid.addWidget(self.seed_on_btn,   1, 0)
        grid.addWidget(self.seed_off_btn,  1, 1)

        return box

    # ── Right panel ───────────────────────────────────────────────────────────

    def _make_right_panel(self) -> QWidget:
        w = QWidget()
        w.setObjectName("RightPanel")
        v = QVBoxLayout(w)
        v.setContentsMargins(6, 12, 12, 12)
        v.setSpacing(8)

        # Header row with CLEAR button
        hdr = QHBoxLayout()
        ttl = QLabel("SERIAL LOG")
        ttl.setObjectName("PanelTitle")
        hdr.addWidget(ttl)
        hdr.addStretch(1)
        clr = QPushButton("CLEAR")
        clr.setObjectName("SmallBtn")
        clr.clicked.connect(self._clear_log)
        hdr.addWidget(clr)
        v.addLayout(hdr)

        # Log area — read-only, auto-scrolls, accepts rich HTML for coloured text
        self.log_box = QTextEdit()
        self.log_box.setReadOnly(True)
        self.log_box.setObjectName("LogBox")
        v.addWidget(self.log_box, 1)

        return w

    def _clear_log(self) -> None:
        self.log_box.clear()
        self.log("Log cleared.")

    # =========================================================================
    #  SERIAL — CONNECTION
    # =========================================================================

    def refresh_ports(self) -> None:
        self.port_combo.clear()
        ports = serial.tools.list_ports.comports()
        if ports:
            for p in ports:
                self.port_combo.addItem(p.device)
            self.log(f"Found {len(ports)} serial port(s).")
        else:
            self.port_combo.addItem("No ports found")
            self.log("No serial ports detected.")

    def toggle_connection(self) -> None:
        if self.connected:
            self._disconnect_serial()
        else:
            self._connect_serial()

    def _connect_serial(self) -> None:
        port = self.port_combo.currentText()
        if port == "No ports found":
            QMessageBox.warning(self, "No Port", "No serial ports are available.")
            return
        try:
            # Baud rate is hardcoded to 115200 to match ESP32 firmware.
            # TODO (ESP32): Confirm this matches Serial.begin() in firmware.
            self.ser = serial.Serial(port, 115200, timeout=0.05)
            self.connected = True
            self._set_pill(True, port)
            self.connect_btn.setText("DISCONNECT")
            self.connect_btn.setObjectName("DisconnectBtn")
            self._repolish(self.connect_btn)
            self.log(f"Connected to {port} @ 115200 baud.")
        except Exception as e:
            QMessageBox.critical(self, "Connection Error", str(e))
            self.log(f"Connection failed: {e}")

    def _disconnect_serial(self) -> None:
        # TODO (ESP32): Consider sending STATE:IDLE before closing to
        #               leave the robot in a safe resting state.
        try:
            if self.ser and self.ser.is_open:
                self.ser.close()
        except Exception as e:
            self.log(f"Disconnect error: {e}")
        finally:
            self.connected = False
            self.ser = None
            self._set_pill(False)
            self.connect_btn.setText("CONNECT")
            self.connect_btn.setObjectName("ConnectBtn")
            self._repolish(self.connect_btn)
            self.log("Disconnected.")

    def _set_pill(self, online: bool, port: str = "") -> None:
        """Update the header connection-status pill."""
        if online:
            self.conn_pill.setText(f"⬤  {port}")
            self.conn_pill.setObjectName("PillOn")
            self.status_lbl.setText(f"Connected: {port}")
        else:
            self.conn_pill.setText("⬤  DISCONNECTED")
            self.conn_pill.setObjectName("PillOff")
            self.status_lbl.setText("No device connected")
        self._repolish(self.conn_pill)

    @staticmethod
    def _repolish(widget: QWidget) -> None:
        """Force Qt stylesheet re-evaluation after objectName change."""
        widget.style().unpolish(widget)
        widget.style().polish(widget)

    # =========================================================================
    #  SERIAL — SEND / RECEIVE
    # =========================================================================

    def send_command(self, command: str) -> None:
        """
        Send a newline-terminated command string to the ESP32.

        If not connected, the attempt is logged but silently dropped so
        the GUI remains usable in offline/testing mode.

        TODO (ESP32): The firmware serial loop should call Serial.readStringUntil('\\n')
                      and dispatch based on the prefix (STATE:, MOVE:, DRILL:, etc.).
        """
        if not self.connected or self.ser is None:
            self.log(f"[NOT CONNECTED]  ✗  {command}")
            return
        try:
            self.ser.write((command + "\n").encode("utf-8"))
            self.log(f"TX → {command}")
        except Exception as e:
            self.log(f"Send error: {e}")
            # TODO (SAFETY): Track consecutive send failures; call
            #                failsafe_serial_lost() after a threshold is reached.

    def _read_serial(self) -> None:
        """
        Drain the serial receive buffer every 100 ms.

        Each complete line is handed to _parse_serial_line().

        TODO (ESP32): Define a structured status packet that the firmware
                      broadcasts periodically, e.g.:
                        STATUS:state=WALKING,drill=50,dist=12.5,fire=0,seeder=0
                      Then implement _parse_serial_line() to decode it.

        TODO (SAFETY): Add a watchdog counter here. If no bytes arrive for
                       N consecutive polls while self.connected is True,
                       call failsafe_serial_lost().
        """
        if not self.connected or self.ser is None:
            return
        try:
            while self.ser.in_waiting:
                raw = self.ser.readline().decode("utf-8", errors="ignore").strip()
                if raw:
                    self.log(f"RX ← {raw}")
                    self._parse_serial_line(raw)
        except Exception as e:
            self.log(f"Read error: {e}")
            # TODO (SAFETY): Call failsafe_serial_lost() after repeated errors.

    def _parse_serial_line(self, line: str) -> None:
        """
        Parse a structured status message arriving from the ESP32.

        TODO (ESP32): Implement this once the serial protocol is defined.

        Suggested skeleton — replace 'pass' with real parsing:

            if line.startswith("STATUS:"):
                pairs = line[7:].split(",")
                data  = dict(p.split("=") for p in pairs if "=" in p)
                self.update_sensor_values(data)

            elif line.startswith("STATE_ACK:"):
                self.log(f"Firmware confirmed state: {line[10:]}")

            elif line.startswith("ALERT:"):
                alert = line[6:]
                if alert == "FIRE":
                    self.failsafe_fire_sensor()

        For now the raw line is already echoed to the log by _read_serial().
        """
        pass  # ← Replace with real protocol parsing

    # =========================================================================
    #  STATE MANAGEMENT
    # =========================================================================

    def _set_state(self, state: str, command: str) -> None:
        """
        Transition the robot to a new state.

        Sends the command, refreshes the state badge + active-button
        highlight, and logs the transition.

        State machine overview:
          IDLE         — All actuators stopped; robot waiting.
          WALKING      — Servo legs active; no drill or seeder.
          PRE_PLANTING — Legs may slow/stop; drill arms up. Drill activates
                         only when ultrasonic reads close enough to the ground.
                         (Ground-proximity gating is enforced in firmware.)
          PLANTING     — Drill stops; stepper motor fires one seed cycle.
                         Firmware auto-returns to WALKING after seed drop.
          ABORT        — Everything stops immediately. Latched until IDLE sent.
        """
        prev = self.robot_state
        self.robot_state = state
        self.send_command(command)

        color = STATE_COLOR.get(state, C["cyan"])
        self.state_badge_lbl.setText(state.replace("_", " "))
        self.state_badge_lbl.setStyleSheet(f"color: {color};")
        self.state_dot.setStyleSheet(f"color: {color};")
        self._highlight_state_btn(state)

        self.log(f"STATE  {prev}  ➜  {state}")

    def _highlight_state_btn(self, active: str) -> None:
        """Highlight the active state button; reset all others."""
        for state, btn in self._state_buttons.items():
            col = btn.property("activeColor")
            if state == active:
                btn.setStyleSheet(
                    f"background: {col}22; border: 1px solid {col}; "
                    f"color: {col}; font-weight: bold;"
                )
            else:
                btn.setStyleSheet("")   # reset to stylesheet default

    def trigger_abort(self) -> None:
        """
        Emergency stop — reachable from any state.

        Sends ABORT over serial and latches the GUI into ABORT state.
        The header ABORT button and the Esc key both call this method.

        TODO (ESP32): The firmware ABORT handler must:
          1. Zero all servo PWM signals immediately (stop legs).
          2. Set drill motor PWM / enable to 0.
          3. Pull stepper driver EN pin HIGH (disable driver).
          4. Remain in ABORT until STATE:IDLE is received from the GUI.
        """
        self._set_state("ABORT", "ABORT")
        self.log("⚠  ABORT — all actuators should be stopping.")

    # =========================================================================
    #  SAFETY PLACEHOLDERS
    # =========================================================================

    def failsafe_fire_sensor(self) -> None:
        """
        Handle a fire sensor ALERT.

        TODO (SAFETY): Call this from _parse_serial_line() when
                       fire=1 or ALERT:FIRE is received from the ESP32.
        """
        self.log("🔥 FIRE SENSOR ALERT — ABORT triggered!")
        QMessageBox.critical(
            self, "⚠ Safety Alert",
            "Fire sensor triggered!\nABORT has been activated."
        )
        self.trigger_abort()

    def failsafe_serial_lost(self) -> None:
        """
        Handle unexpected loss of serial communication.

        TODO (SAFETY): Trigger from the watchdog in _read_serial() when
                       no data arrives for a configurable timeout while
                       self.connected is True.
        """
        self.log("⚠  Serial connection lost — ABORT triggered!")
        self.trigger_abort()
        self._disconnect_serial()

    def failsafe_bad_ultrasonic(self, raw_value: object) -> None:
        """
        Handle an out-of-range / invalid ultrasonic reading.

        Currently only warns. Uncomment trigger_abort() to enforce a hard stop.

        TODO (SAFETY): Set the valid range thresholds once the sensor
                       mounting geometry and planting depth are finalised.

        Args:
            raw_value: The invalid raw distance value from the ESP32.
        """
        self.log(f"⚠  Ultrasonic bad reading ({raw_value}) — check sensor!")
        # self.trigger_abort()   ← uncomment to hard-stop on bad distance

    # =========================================================================
    #  SENSOR VALUE UPDATES
    # =========================================================================

    def update_sensor_values(self, data: dict) -> None:
        """
        Push fresh sensor / actuator data into the dashboard cards.

        TODO (ESP32): Call this from _parse_serial_line() once the serial
                      protocol is agreed.  Expected keys mirror sensor_values:
                        drill_pct  — int 0–100
                        distance   — float or str, distance in cm
                        fire       — "SAFE" | "ALERT" | "0" | "1"
                        seeder     — "ON"  | "OFF"
                        legs       — "FORWARD" | "BACKWARD" | … | "STOPPED"

        Safety checks run inline for fire and ultrasonic readings.
        """
        unit_map: dict[str, str] = {"drill_pct": " %", "distance": " cm"}

        for key, value in data.items():
            if key not in self.sensor_values:
                continue

            self.sensor_values[key] = value

            # Update matching dashboard label
            if key in self._sensor_labels:
                display = str(value) + unit_map.get(key, "")
                self._sensor_labels[key].setText(display)

            # ── Inline safety checks ──────────────────────────────────────
            if key == "fire" and str(value) in ("1", "ALERT", "TRIGGERED"):
                self.failsafe_fire_sensor()

            if key == "distance":
                try:
                    dist = float(str(value))
                    if dist < 0 or dist > 400:          # sensor valid range
                        self.failsafe_bad_ultrasonic(value)
                except (ValueError, TypeError):
                    if str(value) not in ("--", ""):
                        self.failsafe_bad_ultrasonic(value)

    # =========================================================================
    #  MOVEMENT
    # =========================================================================

    def _send_move(self, command: str) -> None:
        """Send a MOVE command and mirror the direction in the leg status card."""
        self.send_command(command)
        direction = command.split(":")[-1] if ":" in command else command
        status    = "STOPPED" if direction == "STOP" else direction
        self.sensor_values["legs"] = status
        if "legs" in self._sensor_labels:
            self._sensor_labels["legs"].setText(status)

    # =========================================================================
    #  LOGGING
    # =========================================================================

    def log(self, message: str) -> None:
        ts  = datetime.now().strftime("%H:%M:%S")
        html = (
            f'<span style="color:{C["muted"]}">[{ts}]</span> '
            f'<span style="color:{C["text"]}">{message}</span>'
        )
        self.log_box.append(html)
        # Auto-scroll to newest entry
        sb = self.log_box.verticalScrollBar()
        sb.setValue(sb.maximum())

    # =========================================================================
    #  KEYBOARD SHORTCUTS
    # =========================================================================

    def keyPressEvent(self, event) -> None:  # type: ignore[override]
        k = event.key()
        if   k == Qt.Key_Up:     self._send_move("MOVE:FORWARD")
        elif k == Qt.Key_Down:   self._send_move("MOVE:BACKWARD")
        elif k == Qt.Key_Left:   self._send_move("MOVE:LEFT")
        elif k == Qt.Key_Right:  self._send_move("MOVE:RIGHT")
        elif k == Qt.Key_Space:  self._send_move("MOVE:STOP")
        elif k == Qt.Key_Escape: self.trigger_abort()
        elif k == Qt.Key_W:      self._set_state("WALKING",      "STATE:WALKING")
        elif k == Qt.Key_I:      self._set_state("IDLE",         "STATE:IDLE")
        elif k == Qt.Key_P:      self._set_state("PRE_PLANTING", "STATE:PRE_PLANTING")
        else:                    super().keyPressEvent(event)

    # =========================================================================
    #  STYLESHEET  — single source-of-truth for all visual styling
    # =========================================================================

    def _apply_stylesheet(self) -> None:
        self.setStyleSheet(f"""

            /* ════════════════════════════════════════════════════════════════
               BASE
            ════════════════════════════════════════════════════════════════ */
            QWidget {{
                background: {C['bg']};
                color: {C['text']};
                font-family: "Segoe UI", "SF Pro Text", "Helvetica Neue", sans-serif;
                font-size: 12px;
            }}

            /* ════════════════════════════════════════════════════════════════
               HEADER
            ════════════════════════════════════════════════════════════════ */
            QFrame#Header {{
                background: {C['surface']};
                border-bottom: 1px solid {C['border']};
            }}
            QLabel#HeaderTitle {{
                font-family: "Consolas", "Courier New", monospace;
                font-size: 13px;
                font-weight: bold;
                letter-spacing: 2px;
                color: {C['cyan']};
            }}
            QFrame#HDivider {{
                color: {C['dim']};
                max-width: 1px;
            }}

            /* Connection pill */
            QLabel#PillOn {{
                color: {C['green']};
                font-size: 11px;
                font-weight: bold;
                padding: 3px 10px;
                border: 1px solid {C['green']};
                border-radius: 10px;
            }}
            QLabel#PillOff {{
                color: {C['muted']};
                font-size: 11px;
                padding: 3px 10px;
                border: 1px solid {C['dim']};
                border-radius: 10px;
            }}

            /* ABORT button */
            QPushButton#AbortBtn {{
                background: {C['red']}1a;
                color: {C['red']};
                border: 1px solid {C['red']};
                border-radius: 6px;
                font-weight: bold;
                font-size: 13px;
                letter-spacing: 1px;
            }}
            QPushButton#AbortBtn:hover   {{ background: {C['red']}33; }}
            QPushButton#AbortBtn:pressed {{ background: {C['red']}55; }}

            /* ════════════════════════════════════════════════════════════════
               PANELS
            ════════════════════════════════════════════════════════════════ */
            QWidget#LeftPanel, QWidget#CenterPanel, QWidget#RightPanel {{
                background: {C['bg']};
            }}

            /* ════════════════════════════════════════════════════════════════
               GROUP BOXES
            ════════════════════════════════════════════════════════════════ */
            QGroupBox {{
                background: {C['card']};
                border: 1px solid {C['border']};
                border-radius: 8px;
                margin-top: 14px;
                padding: 12px 8px 8px 8px;
                font-family: "Consolas", "Courier New", monospace;
                font-size: 9px;
                font-weight: bold;
                letter-spacing: 1.5px;
                color: {C['muted']};
            }}
            QGroupBox::title {{
                subcontrol-origin: margin;
                subcontrol-position: top left;
                padding: 0 6px;
                left: 10px;
                top: 2px;
            }}

            /* ════════════════════════════════════════════════════════════════
               STATE BADGE
            ════════════════════════════════════════════════════════════════ */
            QFrame#StateBadge {{
                background: {C['card']};
                border: 1px solid {C['border']};
                border-radius: 8px;
            }}
            QLabel#BadgeSub {{
                font-family: "Consolas", "Courier New", monospace;
                font-size: 9px;
                letter-spacing: 2px;
                color: {C['muted']};
            }}
            QLabel#BadgeMain {{
                font-family: "Consolas", "Courier New", monospace;
                font-size: 30px;
                font-weight: bold;
                color: {C['cyan']};
            }}
            QLabel#StateDot {{
                font-size: 26px;
                color: {C['cyan']};
            }}

            /* ════════════════════════════════════════════════════════════════
               SENSOR CARDS
            ════════════════════════════════════════════════════════════════ */
            QFrame#SensorCard {{
                bacskground: {C['elevated']};
                border: 1px solid {C['border']};
                border-radius: 6px;
                min-height: 60px;
            }}
            QLabel#CardTitle {{
                font-family: "Consolas", "Courier New", monospace;
                font-size: 9px;
                letter-spacing: 1.5px;
                color: {C['muted']};
            }}
            QLabel#CardValue {{
                font-family: "Consolas", "Courier New", monospace;
                font-size: 17px;
                font-weight: bold;
                color: {C['text']};
            }}

            /* ════════════════════════════════════════════════════════════════
               BUTTONS — general fallback (all buttons not matched below)
            ════════════════════════════════════════════════════════════════ */
            QPushButton {{
                background: {C['elevated']};
                color: {C['text']};
                border: 1px solid {C['border']};
                border-radius: 6px;
                padding: 7px 12px;
                min-height: 30px;
            }}
            QPushButton:hover   {{ background: {C['card']}; border-color: {C['border_hi']}; }}
            QPushButton:pressed {{ background: {C['dim']}; }}

            /* Connect */
            QPushButton#ConnectBtn {{
                background: {C['green']}1a;
                color: {C['green']};
                border-color: {C['green']};
                font-weight: bold;
            }}
            QPushButton#ConnectBtn:hover {{ background: {C['green']}33; }}

            /* Disconnect */
            QPushButton#DisconnectBtn {{
                background: {C['amber']}1a;
                color: {C['amber']};
                border-color: {C['amber']};
                font-weight: bold;
            }}
            QPushButton#DisconnectBtn:hover {{ background: {C['amber']}33; }}

            /* State selector buttons */
            QPushButton#StateBtn {{
                font-family: "Consolas", "Courier New", monospace;
                font-size: 11px;
                letter-spacing: 1px;
                text-align: left;
                padding-left: 12px;
                background: {C['elevated']};
                border: 1px solid {C['border']};
            }}
            QPushButton#StateBtn:hover {{ border-color: {C['border_hi']}; }}

            /* D-pad movement */
            QPushButton#MoveBtn {{
                background: {C['elevated']};
                color: {C['cyan']};
                border: 1px solid {C['border']};
                border-radius: 8px;
                font-size: 18px;
                padding: 0;
            }}
            QPushButton#MoveBtn:hover   {{ background: {C['cyan']}1a; border-color: {C['cyan']}; }}
            QPushButton#MoveBtn:pressed {{ background: {C['cyan']}33; }}

            /* D-pad stop */
            QPushButton#StopBtn {{
                background: {C['elevated']};
                color: {C['amber']};
                border: 1px solid {C['border']};
                border-radius: 8px;
                font-size: 18px;
                padding: 0;
            }}
            QPushButton#StopBtn:hover   {{ background: {C['amber']}1a; border-color: {C['amber']}; }}
            QPushButton#StopBtn:pressed {{ background: {C['amber']}33; }}

            /* Actuator ON */
            QPushButton#ActOnBtn {{
                background: {C['green']}18;
                color: {C['green']};
                border: 1px solid {C['green']}55;
                font-weight: bold;
            }}
            QPushButton#ActOnBtn:hover  {{ background: {C['green']}30; border-color: {C['green']}; }}

            /* Actuator OFF */
            QPushButton#ActOffBtn {{
                background: {C['elevated']};
                color: {C['muted']};
                border: 1px solid {C['border']};
            }}
            QPushButton#ActOffBtn:hover {{ background: {C['card']}; color: {C['text']}; }}

            /* Small utility (refresh, clear) */
            QPushButton#SmallBtn {{
                background: {C['elevated']};
                color: {C['muted']};
                border: 1px solid {C['border']};
                border-radius: 6px;
                padding: 4px 8px;
                min-height: 24px;
                font-size: 11px;
            }}
            QPushButton#SmallBtn:hover {{ color: {C['text']}; border-color: {C['border_hi']}; }}

            /* ════════════════════════════════════════════════════════════════
               COMBO BOX
            ════════════════════════════════════════════════════════════════ */
            QComboBox {{
                background: {C['elevated']};
                color: {C['text']};
                border: 1px solid {C['border']};
                border-radius: 6px;
                padding: 4px 8px;
                min-height: 28px;
            }}
            QComboBox:hover   {{ border-color: {C['border_hi']}; }}
            QComboBox::drop-down {{ border: none; width: 20px; }}
            QComboBox QAbstractItemView {{
                background: {C['card']};
                color: {C['text']};
                selection-background-color: {C['border_hi']};
                border: 1px solid {C['border']};
            }}

            /* ════════════════════════════════════════════════════════════════
               LOG BOX
            ════════════════════════════════════════════════════════════════ */
            QTextEdit#LogBox {{
                background: {C['surface']};
                color: {C['text']};
                border: 1px solid {C['border']};
                border-radius: 6px;
                font-family: "Consolas", "Courier New", monospace;
                font-size: 11px;
                padding: 6px;
            }}

            /* ════════════════════════════════════════════════════════════════
               MISC LABELS
            ════════════════════════════════════════════════════════════════ */
            QLabel#Muted {{
                color: {C['muted']};
                font-size: 11px;
            }}
            QLabel#PanelTitle {{
                font-family: "Consolas", "Courier New", monospace;
                font-size: 10px;
                font-weight: bold;
                letter-spacing: 2px;
                color: {C['muted']};
            }}

            /* ════════════════════════════════════════════════════════════════
               SCROLLBARS
            ════════════════════════════════════════════════════════════════ */
            QScrollBar:vertical {{
                background: {C['surface']};
                width: 6px;
                border-radius: 3px;
                margin: 0;
            }}
            QScrollBar::handle:vertical {{
                background: {C['border_hi']};
                border-radius: 3px;
                min-height: 20px;
            }}
            QScrollBar::add-line:vertical,
            QScrollBar::sub-line:vertical {{ height: 0; }}

            QScrollBar:horizontal {{
                background: {C['surface']};
                height: 6px;
                border-radius: 3px;
            }}
            QScrollBar::handle:horizontal {{
                background: {C['border_hi']};
                border-radius: 3px;
                min-width: 20px;
            }}
            QScrollBar::add-line:horizontal,
            QScrollBar::sub-line:horizontal {{ width: 0; }}

            /* ════════════════════════════════════════════════════════════════
               SPLITTER
            ════════════════════════════════════════════════════════════════ */
            QSplitter::handle {{
                background: {C['border']};
            }}
        """)


# ─────────────────────────────────────────────────────────────────────────────
#  ENTRY POINT
# ─────────────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setApplicationName("Spider Robot Control")
    app.setApplicationVersion("2.0")
    window = ControlWindow()
    window.show()
    sys.exit(app.exec())