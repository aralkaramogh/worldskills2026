/**
 * ================================================================
 *  Differential Drive Teleop Firmware
 *  Target  : ESP32-S3 DevKit
 *  Driver  : Cytron DD10A (single-channel × 2)
 *  Control : Serial (now) → ROS2 cmd_vel bridge (later)
 *  Version : 1.0
 * ================================================================
 *
 *  ── Wiring ──────────────────────────────────────────────────
 *
 *   Motor     │ Signal │ ESP32-S3 GPIO
 *  ───────────┼────────┼──────────────
 *   LEFT  (M1)│ DIR    │ GPIO 4
 *   LEFT  (M1)│ PWM    │ GPIO 5
 *   RIGHT (M2)│ DIR    │ GPIO 15
 *   RIGHT (M2)│ PWM    │ GPIO 16
 *
 *  ── Serial Commands (USB, 115200 baud) ──────────────────────
 *
 *   W / w    Forward
 *   S / s    Backward
 *   A / a    Turn Left  (pivot)
 *   D / d    Turn Right (pivot)
 *   X / x    Stop
 *   H / h    Reset (stop + restore default speeds)
 *   Q / q    Increase forward speed  (+5 %)
 *   Z / z    Decrease forward speed  (-5 %)
 *   E / e    Increase turning speed  (+5 %)
 *   C / c    Decrease turning speed  (-5 %)
 *   ?        Print current status
 *
 *  ── Structured Commands (newline-terminated) ────────────────
 *
 *   MOVE:<L>,<R>\n   Direct duty, L/R in -100..100
 *                    e.g.  MOVE:50,-50   → pivot right
 *   STOP\n           Stop motors
 *   RESET\n          Full reset
 *   STATUS\n         JSON status reply
 *
 *  ── ROS2 Integration (future) ───────────────────────────────
 *
 *   Option A  micro-ROS (on-chip):
 *     Add micro_ros_arduino, subscribe /cmd_vel
 *     (geometry_msgs/Twist), convert → MOVE command internally.
 *
 *   Option B  RPi serial bridge (simpler):
 *     Python ROS2 node on RPi subscribes /cmd_vel and writes
 *     "MOVE:<L>,<R>\n" over /dev/ttyUSB0.
 *
 *     Conversion formula (RPi side):
 *       WHEEL_RADIUS = 0.05          # metres
 *       TRACK_WIDTH  = 0.25          # metres (measure your bot)
 *       MAX_RAD_S    = 10.0          # max wheel angular velocity
 *       vl = (linear - angular * TRACK_WIDTH/2) / WHEEL_RADIUS
 *       vr = (linear + angular * TRACK_WIDTH/2) / WHEEL_RADIUS
 *       L  = clamp(vl / MAX_RAD_S * 100, -100, 100)
 *       R  = clamp(vr / MAX_RAD_S * 100, -100, 100)
 *       serial.write(f"MOVE:{L:.0f},{R:.0f}\n")
 *
 * ================================================================
 */

#include <Arduino.h>

// ── Build Options ────────────────────────────────────────────
#define WATCHDOG_ENABLE  1      // 1 = auto-stop if no cmd received
#define WATCHDOG_MS      500    // ms timeout before auto-stop

// ── Pin Definitions ──────────────────────────────────────────
constexpr int DIR_L  =  4;     // Left  motor direction
constexpr int PWM_L  =  5;     // Left  motor PWM
constexpr int DIR_R  = 15;     // Right motor direction
constexpr int PWM_R  = 16;     // Right motor PWM

constexpr int LED_PIN = 2;     // Built-in LED (active HIGH on S3-DevKit)

// ── LEDC (ESP32-S3 PWM) ──────────────────────────────────────
constexpr int LEDC_FREQ     = 20000;  // 20 kHz — inaudible, low ripple
constexpr int LEDC_BITS     = 8;      // 8-bit resolution → 0-255
constexpr int LEDC_CH_LEFT  = 0;
constexpr int LEDC_CH_RIGHT = 1;

// ── Motor Inversion ──────────────────────────────────────────
// One motor is mounted mirrored on a diff-drive chassis.
// Flip the one that spins backwards when both should go forward.
constexpr bool INVERT_LEFT  = false;
constexpr bool INVERT_RIGHT = true;

// ── Speed Settings ───────────────────────────────────────────
int fwdSpeed  = 100;   // % — user-adjustable via Q/Z
int turnSpeed = 100;   // % — user-adjustable via E/C

constexpr int FWD_CAP  = 80;   // hard ceiling, not serial-adjustable
constexpr int TURN_CAP = 80;

// ── State ────────────────────────────────────────────────────
bool          running       = false;
unsigned long lastCmdMs     = 0;
String        cmdBuf        = "";

// ================================================================
//  MOTOR CONTROL
// ================================================================

int clampDuty(int d) { return constrain(d, -100, 100); }

int dutyToLedc(int d) {
  return map(constrain(abs(d), 0, 100), 0, 100, 0, 255);
}

void setLeftMotor(int duty) {
  duty = clampDuty(duty);
  bool fwd = (duty >= 0);
  if (INVERT_LEFT) fwd = !fwd;
  digitalWrite(DIR_L, fwd ? HIGH : LOW);
  ledcWrite(LEDC_CH_LEFT, dutyToLedc(duty));
}

void setRightMotor(int duty) {
  duty = clampDuty(duty);
  bool fwd = (duty >= 0);
  if (INVERT_RIGHT) fwd = !fwd;
  digitalWrite(DIR_R, fwd ? HIGH : LOW);
  ledcWrite(LEDC_CH_RIGHT, dutyToLedc(duty));
}

void setMotors(int l, int r) {
  setLeftMotor(l);
  setRightMotor(r);
  running = (l != 0 || r != 0);
  digitalWrite(LED_PIN, running ? HIGH : LOW);
}

// ================================================================
//  MOTION COMMANDS
// ================================================================

void cmdForward() {
  int sp = min(fwdSpeed, FWD_CAP);
  setMotors(sp, sp);
  Serial.printf("[FWD]  L=%d R=%d\n", sp, sp);
}

void cmdBackward() {
  int sp = min(fwdSpeed, FWD_CAP);
  setMotors(-sp, -sp);
  Serial.printf("[BWD]  L=%d R=%d\n", -sp, -sp);
}

void cmdTurnLeft() {
  int sp = min(turnSpeed, TURN_CAP);
  setMotors(-sp, sp);
  Serial.printf("[LEFT] L=%d R=%d\n", -sp, sp);
}

void cmdTurnRight() {
  int sp = min(turnSpeed, TURN_CAP);
  setMotors(sp, -sp);
  Serial.printf("[RIGHT] L=%d R=%d\n", sp, -sp);
}

void cmdStop() {
  setMotors(0, 0);
  Serial.println("[STOP]");
}

void cmdReset() {
  cmdStop();
  fwdSpeed  = 30;
  turnSpeed = 40;
  Serial.println("[RESET] fwdSpeed=30 turnSpeed=40");
}

void printStatus() {
  Serial.printf(
    "{\"fwd_speed\":%d,\"turn_speed\":%d,"
    "\"fwd_cap\":%d,\"turn_cap\":%d,"
    "\"running\":%s,\"watchdog_ms\":%d}\n",
    fwdSpeed, turnSpeed,
    FWD_CAP, TURN_CAP,
    running ? "true" : "false",
    WATCHDOG_MS
  );
}

// ================================================================
//  COMMAND PARSERS
// ================================================================

void handleChar(char c) {
  lastCmdMs = millis();

  switch (c) {
    // ── Motion ───────────────────────────────────────────────
    case 'w': case 'W': case 'f': case 'F': cmdForward();   break;
    case 's': case 'S': case 'b': case 'B': cmdBackward();  break;
    case 'a': case 'A': case 'l': case 'L': cmdTurnLeft();  break;
    case 'd': case 'D': case 'r': case 'R': cmdTurnRight(); break;

    // ── Stop / Reset ─────────────────────────────────────────
    case 'x': case 'X': case ' ': cmdStop();  break;
    case 'h': case 'H':           cmdReset(); break;

    // ── Forward Speed ────────────────────────────────────────
    case 'q': case 'Q':
      fwdSpeed = min(fwdSpeed + 5, FWD_CAP);
      Serial.printf("[SPD] fwdSpeed=%d\n", fwdSpeed);
      break;
    case 'z': case 'Z':
      fwdSpeed = max(fwdSpeed - 5, 5);
      Serial.printf("[SPD] fwdSpeed=%d\n", fwdSpeed);
      break;

    // ── Turn Speed ───────────────────────────────────────────
    case 'e': case 'E':
      turnSpeed = min(turnSpeed + 5, TURN_CAP);
      Serial.printf("[SPD] turnSpeed=%d\n", turnSpeed);
      break;
    case 'c': case 'C':
      turnSpeed = max(turnSpeed - 5, 5);
      Serial.printf("[SPD] turnSpeed=%d\n", turnSpeed);
      break;

    // ── Status ───────────────────────────────────────────────
    case '?': printStatus(); break;

    default: break;
  }
}

/**
 * Parse structured newline-terminated command.
 *   MOVE:<L>,<R>  |  STOP  |  RESET  |  STATUS
 */
void handleLine(String &line) {
  line.trim();
  if (line.length() == 0) return;
  lastCmdMs = millis();

  if (line.equalsIgnoreCase("STOP"))   { cmdStop();    return; }
  if (line.equalsIgnoreCase("RESET"))  { cmdReset();   return; }
  if (line.equalsIgnoreCase("STATUS")) { printStatus(); return; }

  if (line.startsWith("MOVE:") || line.startsWith("move:")) {
    String args = line.substring(5);
    int comma = args.indexOf(',');
    if (comma < 0) {
      Serial.println("[ERR] Usage: MOVE:<L>,<R>");
      return;
    }
    int L = constrain(args.substring(0, comma).toInt(), -100, 100);
    int R = constrain(args.substring(comma + 1).toInt(), -100, 100);
    // Enforce hard caps
    L = constrain(L, -FWD_CAP, FWD_CAP);
    R = constrain(R, -FWD_CAP, FWD_CAP);
    setMotors(L, R);
    Serial.printf("[MOVE] L=%d R=%d\n", L, R);
    return;
  }

  Serial.printf("[WARN] Unknown: %s\n", line.c_str());
}

void processSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\n' || c == '\r') {
      if (cmdBuf.length() > 1) {
        handleLine(cmdBuf);       // multi-char → structured
      } else if (cmdBuf.length() == 1) {
        handleChar(cmdBuf[0]);    // single char with newline
      }
      cmdBuf = "";
    } else {
      if (cmdBuf.length() < 64) cmdBuf += c;
      else cmdBuf = "";           // overflow guard
    }
  }
}

// ================================================================
//  SETUP
// ================================================================

void setup() {
  Serial.begin(115200);
  delay(300);

  // Direction pins
  pinMode(DIR_L,  OUTPUT); digitalWrite(DIR_L,  LOW);
  pinMode(DIR_R,  OUTPUT); digitalWrite(DIR_R,  LOW);
  pinMode(LED_PIN, OUTPUT); digitalWrite(LED_PIN, LOW);

  // LEDC PWM
  ledcSetup(LEDC_CH_LEFT,  LEDC_FREQ, LEDC_BITS);
  ledcSetup(LEDC_CH_RIGHT, LEDC_FREQ, LEDC_BITS);
  ledcAttachPin(PWM_L, LEDC_CH_LEFT);
  ledcAttachPin(PWM_R, LEDC_CH_RIGHT);
  ledcWrite(LEDC_CH_LEFT,  0);
  ledcWrite(LEDC_CH_RIGHT, 0);

  lastCmdMs = millis();

  Serial.println();
  Serial.println("╔══════════════════════════════════════════╗");
  Serial.println("║  Diff Drive Teleop  |  ESP32-S3 + DD10A  ║");
  Serial.println("╚══════════════════════════════════════════╝");
  Serial.println("Single-char : W S A D  X(stop)  H(reset)  ?(status)");
  Serial.println("Speed keys  : Q/Z (fwd)   E/C (turn)");
  Serial.println("Structured  : MOVE:<L>,<R>  STOP  RESET  STATUS");
  Serial.printf ("Fwd speed   : %d%%  (cap %d%%)\n", fwdSpeed,  FWD_CAP);
  Serial.printf ("Turn speed  : %d%%  (cap %d%%)\n", turnSpeed, TURN_CAP);
#if WATCHDOG_ENABLE
  Serial.printf ("Watchdog    : %d ms\n", WATCHDOG_MS);
#else
  Serial.println("Watchdog    : DISABLED");
#endif
  Serial.println("Ready.\n");
}

// ================================================================
//  LOOP
// ================================================================

void loop() {
  processSerial();

#if WATCHDOG_ENABLE
  if (running && (millis() - lastCmdMs > WATCHDOG_MS)) {
    Serial.println("[WDG] timeout → STOP");
    cmdStop();
  }
#endif
}