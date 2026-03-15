/**
 * ============================================================
 *  AMR Differential Drive Firmware
 *  raspi-esp-drive v1.0
 * ============================================================
 *
 *  MCU     : ESP32 N16R8 (16 MB Flash, 8 MB PSRAM)
 *  Driver  : Cytron MDD10 (dual-channel brushed DC driver)
 *  Motors  : REV HD Hex Motor + Ultra Planetary Gearbox (x2, front)
 *  Passive : 1× Omni wheel (rear caster)
 *  Control : Raspberry Pi 4 (8 GB) via UART
 *  Future  : ROS2 via micro-ROS or structured serial bridge
 *
 * ─── Robot Geometry ──────────────────────────────────────────
 *
 *        [LEFT MOTOR]   [RIGHT MOTOR]
 *              O───────────O       ← Front (Drive Axle)
 *              │           │
 *              │    AMR    │
 *              │           │
 *              └─────O─────┘       ← Rear (Passive Omni)
 *
 * ─── Cytron MDD10 Wiring ─────────────────────────────────────
 *
 *   MDD10 Pin │ ESP32 GPIO │ Function
 *   ──────────┼────────────┼──────────────────────────────────
 *   PWM1      │   GPIO 25  │ Left  Motor Speed
 *   DIR1      │   GPIO 26  │ Left  Motor Direction
 *   PWM2      │   GPIO 27  │ Right Motor Speed
 *   DIR2      │   GPIO 14  │ Right Motor Direction
 *
 * ─── UART Mapping ────────────────────────────────────────────
 *
 *   UART0  USB    (GPIO 1/3)   → Arduino IDE Serial Monitor / debug
 *   UART2  HW     (GPIO 16/17) → Raspberry Pi 4 (RX2=16, TX2=17)
 *   Baud: 115200 on both
 *
 *   The firmware listens on BOTH UARTs simultaneously. Commands
 *   arriving from either port are processed identically.
 *
 * ─── Serial Command Reference ────────────────────────────────
 *
 *  Single-char (case-insensitive):
 *   W / F        → Forward
 *   S / B        → Backward
 *   A / L        → Turn Left  (pivot)
 *   D / R        → Turn Right (pivot)
 *   X / Space    → Stop
 *   H            → Reset (stop + restore default speeds)
 *   Q            → Increase forward/backward speed (+5 %)
 *   Z            → Decrease forward/backward speed (-5 %)
 *   E            → Increase turning speed (+5 %)
 *   C            → Decrease turning speed (-5 %)
 *   ?            → Print current status
 *
 *  Structured commands (newline-terminated, ROS2-bridge ready):
 *   MOVE:<L>,<R>\n   → Direct duty cycle, L/R in -100..100
 *                       e.g.  "MOVE:50,-50\n"  → pivot right at 50 %
 *   STOP\n           → Stop all motors
 *   RESET\n          → Full reset
 *   STATUS\n         → JSON status reply
 *
 * ─── Safety ──────────────────────────────────────────────────
 *
 *   WATCHDOG_MS  : If no command is received within this window
 *                  the robot stops automatically. Set 0 to disable.
 *   FORWARD_BACKWARD_CAP / TURNING_CAP : hard speed ceilings
 *                  (change in code; not adjustable via serial).
 *
 * ─── PWM (LEDC) ──────────────────────────────────────────────
 *
 *   ESP32 does NOT use analogWrite(). The LEDC peripheral is used
 *   instead (ledcSetup / ledcAttachPin / ledcWrite).
 *   Frequency: 20 kHz (inaudible, low ripple for DC motors)
 *   Resolution: 8-bit → duty 0-255
 *
 * ============================================================
 */

#include <Arduino.h>

// ─── Build-time Options ───────────────────────────────────────
#define ENABLE_WATCHDOG   1        // 1 = auto-stop on timeout
#define WATCHDOG_MS       500      // ms without command → stop
#define ENABLE_RAMPING    0        // 1 = soft speed ramping (future)
// ─────────────────────────────────────────────────────────────

// ─── LED ──────────────────────────────────────────────────────
const int LED_PIN = 2;             // Built-in LED on ESP32 DevKit

// ─── Cytron MDD10 Pins ───────────────────────────────────────
const int MOTOR_PWM_L  = 4;      // Left  Motor PWM  → MDD10 PWM1
const int MOTOR_DIR_L  = 5;      // Left  Motor DIR  → MDD10 DIR1
const int MOTOR_PWM_R  = 15;      // Right Motor PWM  → MDD10 PWM2
const int MOTOR_DIR_R  = 16;      // Right Motor DIR  → MDD10 DIR2

// ─── LEDC (ESP32 PWM) ─────────────────────────────────────────
const int LEDC_FREQ      = 20000; // 20 kHz
const int LEDC_RES_BITS  = 8;     // 8-bit → 0-255
const int LEDC_CH_LEFT   = 0;     // LEDC channel for left motor
const int LEDC_CH_RIGHT  = 1;     // LEDC channel for right motor

// ─── UART2 (Raspberry Pi) ─────────────────────────────────────
const int PI_RX_PIN = 16;         // ESP32 RX2 ← RPi TX
const int PI_TX_PIN = 17;         // ESP32 TX2 → RPi RX
const long SERIAL_BAUD = 115200;

// ─── Motor Inversion ──────────────────────────────────────────
// Because the two motors face opposite directions on the chassis,
// one side's "forward" is physically reversed relative to the other.
// Set to true for the motor whose DIR logic must be flipped.
const bool INVERT_LEFT  = false;
const bool INVERT_RIGHT = true;   // Right motor mounted mirrored

// ─── Speed Variables ─────────────────────────────────────────
// User-adjustable via serial (Q/Z, E/C).
int forwardBackwardSpeed = 100;    // % (default)
int turningSpeed         = 100;    // % (default)

// Hard safety caps — not adjustable via serial.
// Increase cautiously; HD Hex + gearbox can generate high torque.
const int FORWARD_BACKWARD_CAP = 60;   // max % for fwd/back
const int TURNING_CAP          = 80;   // max % for pivot turns

// ─── Watchdog ─────────────────────────────────────────────────
unsigned long lastCommandMs = 0;
bool motorsRunning = false;

// ─── String Buffer (structured commands) ─────────────────────
String cmdBuffer = "";

// ═════════════════════════════════════════════════════════════
//  HELPERS
// ═════════════════════════════════════════════════════════════

/** Clamp value to -100..100 */
int limitDuty(int duty) {
  return constrain(duty, -100, 100);
}

/** Map -100..100 duty percent → 0..255 LEDC duty */
int dutyToLedc(int dutyPercent) {
  int mag = constrain(abs(dutyPercent), 0, 100);
  return map(mag, 0, 100, 0, 255);
}

// ═════════════════════════════════════════════════════════════
//  MOTOR SETTERS
// ═════════════════════════════════════════════════════════════

/**
 * Drive left motor.
 * @param duty  -100 (full reverse) .. 100 (full forward)
 */
void setLeftMotor(int duty) {
  duty = limitDuty(duty);
  bool fwd = (duty >= 0);
  if (INVERT_LEFT) fwd = !fwd;
  digitalWrite(MOTOR_DIR_L, fwd ? HIGH : LOW);
  ledcWrite(LEDC_CH_LEFT, dutyToLedc(duty));
}

/**
 * Drive right motor.
 * @param duty  -100 (full reverse) .. 100 (full forward)
 */
void setRightMotor(int duty) {
  duty = limitDuty(duty);
  bool fwd = (duty >= 0);
  if (INVERT_RIGHT) fwd = !fwd;
  digitalWrite(MOTOR_DIR_R, fwd ? HIGH : LOW);
  ledcWrite(LEDC_CH_RIGHT, dutyToLedc(duty));
}

/**
 * Drive both motors simultaneously.
 * @param leftDuty   -100..100
 * @param rightDuty  -100..100
 */
void setMotors(int leftDuty, int rightDuty) {
  setLeftMotor(leftDuty);
  setRightMotor(rightDuty);
}

// ═════════════════════════════════════════════════════════════
//  MOTION COMMANDS
// ═════════════════════════════════════════════════════════════

void driveForward(int speedPct) {
  speedPct = constrain(abs(speedPct), 0, forwardBackwardSpeed);
  speedPct = min(speedPct, FORWARD_BACKWARD_CAP);
  setMotors(speedPct, speedPct);
  motorsRunning = true;
  Serial.print("[CMD] Forward  | duty: "); Serial.println(speedPct);
  Serial2.print("[CMD] Forward  | duty: "); Serial2.println(speedPct);
}

void driveBackward(int speedPct) {
  speedPct = constrain(abs(speedPct), 0, forwardBackwardSpeed);
  speedPct = min(speedPct, FORWARD_BACKWARD_CAP);
  setMotors(-speedPct, -speedPct);
  motorsRunning = true;
  Serial.print("[CMD] Backward | duty: "); Serial.println(speedPct);
  Serial2.print("[CMD] Backward | duty: "); Serial2.println(speedPct);
}

void pivotRight(int speedPct) {
  speedPct = constrain(abs(speedPct), 0, turningSpeed);
  speedPct = min(speedPct, TURNING_CAP);
  // Left fwd, Right rev → pivots clockwise (right)
  setMotors(speedPct, -speedPct);
  motorsRunning = true;
  Serial.print("[CMD] Right    | duty: "); Serial.println(speedPct);
  Serial2.print("[CMD] Right    | duty: "); Serial2.println(speedPct);
}

void pivotLeft(int speedPct) {
  speedPct = constrain(abs(speedPct), 0, turningSpeed);
  speedPct = min(speedPct, TURNING_CAP);
  // Left rev, Right fwd → pivots counter-clockwise (left)
  setMotors(-speedPct, speedPct);
  motorsRunning = true;
  Serial.print("[CMD] Left     | duty: "); Serial.println(speedPct);
  Serial2.print("[CMD] Left     | duty: "); Serial2.println(speedPct);
}

void stopAll() {
  setMotors(0, 0);
  motorsRunning = false;
  digitalWrite(LED_PIN, LOW);
  Serial.println("[CMD] STOP");
  Serial2.println("[CMD] STOP");
}

void resetControl() {
  stopAll();
  forwardBackwardSpeed = 20;
  turningSpeed         = 30;
  Serial.println("[RESET] Speeds restored to defaults (fwd=20, turn=30)");
  Serial2.println("[RESET] Speeds restored to defaults (fwd=20, turn=30)");
}

// ─── Status JSON ──────────────────────────────────────────────
void printStatus(Stream &port) {
  port.print("{\"fwd_speed\":");  port.print(forwardBackwardSpeed);
  port.print(",\"turn_speed\":"); port.print(turningSpeed);
  port.print(",\"fwd_cap\":");    port.print(FORWARD_BACKWARD_CAP);
  port.print(",\"turn_cap\":");   port.print(TURNING_CAP);
  port.print(",\"running\":");    port.print(motorsRunning ? "true" : "false");
  port.print(",\"watchdog_ms\":"); port.print(WATCHDOG_MS);
  port.println("}");
}

// ═════════════════════════════════════════════════════════════
//  COMMAND PARSERS
// ═════════════════════════════════════════════════════════════

/**
 * Handle single-character commands.
 * Called for every received byte that is not part of a structured cmd.
 */
void handleCharCommand(char cmd) {
  lastCommandMs = millis();

  switch (cmd) {
    // ── Movement ───────────────────────────────────────────
    case 'w': case 'W': case 'f': case 'F':
      digitalWrite(LED_PIN, HIGH);
      driveForward(forwardBackwardSpeed);
      break;

    case 's': case 'S': case 'b': case 'B':
      digitalWrite(LED_PIN, HIGH);
      driveBackward(forwardBackwardSpeed);
      break;

    case 'a': case 'A': case 'l': case 'L':
      digitalWrite(LED_PIN, HIGH);
      pivotLeft(turningSpeed);
      break;

    case 'd': case 'D': case 'r': case 'R':
      digitalWrite(LED_PIN, HIGH);
      pivotRight(turningSpeed);
      break;

    // ── Stop / Reset ───────────────────────────────────────
    case 'x': case 'X': case ' ':
      stopAll();
      break;

    case 'h': case 'H':
      resetControl();
      break;

    // ── Forward/Backward Speed ─────────────────────────────
    case 'q': case 'Q':
      forwardBackwardSpeed = min(forwardBackwardSpeed + 5, FORWARD_BACKWARD_CAP);
      Serial.print("[SPEED] Fwd/Back: "); Serial.println(forwardBackwardSpeed);
      Serial2.print("[SPEED] Fwd/Back: "); Serial2.println(forwardBackwardSpeed);
      break;

    case 'z': case 'Z':
      forwardBackwardSpeed = max(forwardBackwardSpeed - 5, 5);
      Serial.print("[SPEED] Fwd/Back: "); Serial.println(forwardBackwardSpeed);
      Serial2.print("[SPEED] Fwd/Back: "); Serial2.println(forwardBackwardSpeed);
      break;

    // ── Turning Speed ──────────────────────────────────────
    case 'e': case 'E':
      turningSpeed = min(turningSpeed + 5, TURNING_CAP);
      Serial.print("[SPEED] Turn: "); Serial.println(turningSpeed);
      Serial2.print("[SPEED] Turn: "); Serial2.println(turningSpeed);
      break;

    case 'c': case 'C':
      turningSpeed = max(turningSpeed - 5, 5);
      Serial.print("[SPEED] Turn: "); Serial.println(turningSpeed);
      Serial2.print("[SPEED] Turn: "); Serial2.println(turningSpeed);
      break;

    // ── Status ─────────────────────────────────────────────
    case '?':
      printStatus(Serial);
      printStatus(Serial2);
      break;

    default:
      break; // Unknown char — ignore silently
  }
}

/**
 * Parse and execute structured command (newline-terminated string).
 *
 * Supported:
 *   MOVE:<left>,<right>   e.g. "MOVE:60,-60"
 *   STOP
 *   RESET
 *   STATUS
 */
void handleStructuredCommand(String &line) {
  line.trim();
  if (line.length() == 0) return;

  lastCommandMs = millis();

  if (line.equalsIgnoreCase("STOP")) {
    stopAll();
    return;
  }

  if (line.equalsIgnoreCase("RESET")) {
    resetControl();
    return;
  }

  if (line.equalsIgnoreCase("STATUS")) {
    printStatus(Serial);
    printStatus(Serial2);
    return;
  }

  // MOVE:<L>,<R>
  if (line.startsWith("MOVE:") || line.startsWith("move:")) {
    String args = line.substring(5); // after "MOVE:"
    int commaIdx = args.indexOf(',');
    if (commaIdx < 0) {
      Serial.println("[ERR] MOVE: expected MOVE:<L>,<R>");
      return;
    }
    int leftDuty  = constrain(args.substring(0, commaIdx).toInt(), -100, 100);
    int rightDuty = constrain(args.substring(commaIdx + 1).toInt(), -100, 100);

    // Enforce caps (absolute values vs caps)
    leftDuty  = constrain(leftDuty,  -FORWARD_BACKWARD_CAP, FORWARD_BACKWARD_CAP);
    rightDuty = constrain(rightDuty, -FORWARD_BACKWARD_CAP, FORWARD_BACKWARD_CAP);

    setMotors(leftDuty, rightDuty);
    motorsRunning = (leftDuty != 0 || rightDuty != 0);
    if (motorsRunning) digitalWrite(LED_PIN, HIGH);
    else               digitalWrite(LED_PIN, LOW);

    Serial.print("[MOVE] L="); Serial.print(leftDuty);
    Serial.print(" R=");       Serial.println(rightDuty);
    Serial2.print("[MOVE] L="); Serial2.print(leftDuty);
    Serial2.print(" R=");       Serial2.println(rightDuty);
    return;
  }

  // Unknown structured command
  Serial.print("[WARN] Unknown cmd: "); Serial.println(line);
}

/**
 * Read available bytes from a Stream, accumulate into cmdBuffer.
 * Dispatch structured command on '\n'; single chars immediately.
 */
void processStream(Stream &port) {
  while (port.available() > 0) {
    char c = (char)port.read();

    if (c == '\n' || c == '\r') {
      if (cmdBuffer.length() > 1) {
        // Multi-char → structured command
        handleStructuredCommand(cmdBuffer);
      } else if (cmdBuffer.length() == 1) {
        // Single char with newline → single-char command
        handleCharCommand(cmdBuffer[0]);
      }
      cmdBuffer = "";
    } else {
      cmdBuffer += c;
      // Safety: prevent unbounded growth
      if (cmdBuffer.length() > 64) cmdBuffer = "";
    }
  }
}

// ═════════════════════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════════════════════

void setup() {
  // Debug serial (USB)
  Serial.begin(SERIAL_BAUD);
  delay(200);

  // Raspberry Pi serial (UART2)
  Serial2.begin(SERIAL_BAUD, SERIAL_8N1, PI_RX_PIN, PI_TX_PIN);

  // ── LED ────────────────────────────────────────────────────
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // ── Motor Direction Pins ───────────────────────────────────
  pinMode(MOTOR_DIR_L, OUTPUT);
  pinMode(MOTOR_DIR_R, OUTPUT);
  digitalWrite(MOTOR_DIR_L, LOW);
  digitalWrite(MOTOR_DIR_R, LOW);

  // ── LEDC PWM Setup ────────────────────────────────────────
  ledcSetup(LEDC_CH_LEFT,  LEDC_FREQ, LEDC_RES_BITS);
  ledcSetup(LEDC_CH_RIGHT, LEDC_FREQ, LEDC_RES_BITS);
  ledcAttachPin(MOTOR_PWM_L, LEDC_CH_LEFT);
  ledcAttachPin(MOTOR_PWM_R, LEDC_CH_RIGHT);
  ledcWrite(LEDC_CH_LEFT,  0);
  ledcWrite(LEDC_CH_RIGHT, 0);

  lastCommandMs = millis();

  // ── Banner ─────────────────────────────────────────────────
  Serial.println();
  Serial.println("╔══════════════════════════════════════════╗");
  Serial.println("║  AMR Drive  |  ESP32 N16R8 + MDD10       ║");
  Serial.println("║  raspi-esp-drive v1.0                     ║");
  Serial.println("╚══════════════════════════════════════════╝");
  Serial.println("Commands: W/S/A/D  X(stop)  H(reset)  ?(status)");
  Serial.println("Structured: MOVE:<L>,<R>  STOP  RESET  STATUS");
  Serial.print("Fwd/Back Speed: "); Serial.print(forwardBackwardSpeed);
  Serial.print("% (cap "); Serial.print(FORWARD_BACKWARD_CAP); Serial.println("%)");
  Serial.print("Turn Speed:     "); Serial.print(turningSpeed);
  Serial.print("% (cap "); Serial.print(TURNING_CAP); Serial.println("%)");
#if ENABLE_WATCHDOG
  Serial.print("Watchdog: "); Serial.print(WATCHDOG_MS); Serial.println(" ms");
#else
  Serial.println("Watchdog: DISABLED");
#endif
  Serial.println("Ready.");
  Serial.println();

  // Mirror banner to RPi port
  Serial2.println("{\"status\":\"ready\",\"firmware\":\"raspi-esp-drive-v1.0\"}");
}

// ═════════════════════════════════════════════════════════════
//  LOOP
// ═════════════════════════════════════════════════════════════

void loop() {
  // ── Process USB Serial (debug / direct) ───────────────────
  processStream(Serial);

  // ── Process Raspberry Pi Serial ───────────────────────────
  processStream(Serial2);

  // ── Watchdog ──────────────────────────────────────────────
#if ENABLE_WATCHDOG
  if (motorsRunning && (millis() - lastCommandMs > WATCHDOG_MS)) {
    Serial.println("[WDG] Timeout → STOP");
    Serial2.println("[WDG] Timeout → STOP");
    stopAll();
  }
#endif
}

// ═════════════════════════════════════════════════════════════
//  ROS2 BRIDGE NOTES (for Raspberry Pi side)
// ═════════════════════════════════════════════════════════════
//
//  Option A — micro-ROS (recommended long-term):
//    Add micro_ros_arduino library, replace Serial2 with a micro-ROS
//    transport, subscribe to /cmd_vel (geometry_msgs/Twist), convert
//    linear.x / angular.z → differential wheel duties, send MOVE: cmd.
//
//  Option B — ROS2 serial bridge (simpler, works now):
//    On RPi, run a Python ROS2 node that subscribes /cmd_vel and
//    writes "MOVE:<L>,<R>\n" over /dev/ttyS0 (or USB).
//
//    Differential drive conversion (RPi side, pseudocode):
//      wheel_radius = 0.048           # metres (HD Hex ≈ 96 mm dia)
//      track_width  = 0.30            # metres, measure your chassis
//      v_left  = (linear - angular * track_width/2) / wheel_radius
//      v_right = (linear + angular * track_width/2) / wheel_radius
//      # Normalise to -100..100 based on max rad/s
//      L_pct = clamp(v_left  / max_rad_per_s * 100, -100, 100)
//      R_pct = clamp(v_right / max_rad_per_s * 100, -100, 100)
//      serial.write(f"MOVE:{L_pct:.0f},{R_pct:.0f}\n")
//
// ═════════════════════════════════════════════════════════════