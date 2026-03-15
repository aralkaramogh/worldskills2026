/**
 * ================================================================
 *  Diff Drive Teleop  |  ESP32-S3 DevKit  |  Cytron DD10A
 *  v1.1 — Fixed LEDC for Arduino core v2.x AND v3.x
 * ================================================================
 *
 *  Pins:
 *   Left  Motor : DIR=4   PWM=5
 *   Right Motor : DIR=15  PWM=16
 *
 *  Serial Monitor: set to "Newline" or "Both NL & CR", 115200 baud
 *
 *  Commands:
 *   W/F  Forward    S/B  Backward
 *   A/L  Turn Left  D/R  Turn Right
 *   X    Stop       H    Reset
 *   Q/Z  Fwd speed ±5%   E/C  Turn speed ±5%
 *   ?    Status
 *   MOVE:<L>,<R>  e.g. MOVE:50,-50
 * ================================================================
 */

#include <Arduino.h>

// ── Detect core version ──────────────────────────────────────
// Arduino ESP32 core v3.x removed ledcSetup/ledcAttachPin.
// We detect at compile time and use the right API.
#ifndef ESP_ARDUINO_VERSION_MAJOR
  #define ESP_ARDUINO_VERSION_MAJOR 2
#endif

// ── Pins ─────────────────────────────────────────────────────
#define DIR_L   4
#define PWM_L   5
#define DIR_R   15
#define PWM_R   16
#define LED_PIN 2

// ── LEDC config ──────────────────────────────────────────────
#define LEDC_FREQ   20000   // 20 kHz
#define LEDC_BITS   8       // 0-255
#define CH_L        0
#define CH_R        1

// ── Motor inversion ──────────────────────────────────────────
// On a diff-drive chassis, motors face opposite directions.
// If one motor spins the WRONG way, flip its INVERT flag.
#define INVERT_LEFT  false
#define INVERT_RIGHT true

// ── Watchdog ─────────────────────────────────────────────────
#define WATCHDOG_ENABLE 1
#define WATCHDOG_MS     500

// ── Speed settings ───────────────────────────────────────────
int fwdSpeed  = 30;
int turnSpeed = 40;

const int FWD_CAP  = 80;
const int TURN_CAP = 80;

// ── State ────────────────────────────────────────────────────
bool          running   = false;
unsigned long lastCmdMs = 0;
String        cmdBuf    = "";

// ================================================================
//  PWM ABSTRACTION  (works on core v2.x AND v3.x)
// ================================================================

void pwmSetup() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  // Core v3.x API — attach pin directly, no channel needed
  ledcAttach(PWM_L, LEDC_FREQ, LEDC_BITS);
  ledcAttach(PWM_R, LEDC_FREQ, LEDC_BITS);
#else
  // Core v2.x API
  ledcSetup(CH_L, LEDC_FREQ, LEDC_BITS);
  ledcSetup(CH_R, LEDC_FREQ, LEDC_BITS);
  ledcAttachPin(PWM_L, CH_L);
  ledcAttachPin(PWM_R, CH_R);
#endif
}

void pwmWriteLeft(int duty8bit) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PWM_L, duty8bit);
#else
  ledcWrite(CH_L, duty8bit);
#endif
}

void pwmWriteRight(int duty8bit) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PWM_R, duty8bit);
#else
  ledcWrite(CH_R, duty8bit);
#endif
}

// ================================================================
//  MOTOR CONTROL
// ================================================================

int dutyTo8bit(int pct) {
  return map(constrain(abs(pct), 0, 100), 0, 100, 0, 255);
}

void setLeftMotor(int pct) {
  pct = constrain(pct, -100, 100);
  bool fwd = (pct >= 0);
  if (INVERT_LEFT) fwd = !fwd;
  digitalWrite(DIR_L, fwd ? HIGH : LOW);
  pwmWriteLeft(dutyTo8bit(pct));
}

void setRightMotor(int pct) {
  pct = constrain(pct, -100, 100);
  bool fwd = (pct >= 0);
  if (INVERT_RIGHT) fwd = !fwd;
  digitalWrite(DIR_R, fwd ? HIGH : LOW);
  pwmWriteRight(dutyTo8bit(pct));
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
  Serial.printf("[FWD]   L=+%d  R=+%d\n", sp, sp);
}

void cmdBackward() {
  int sp = min(fwdSpeed, FWD_CAP);
  setMotors(-sp, -sp);
  Serial.printf("[BWD]   L=-%d  R=-%d\n", sp, sp);
}

void cmdTurnLeft() {
  int sp = min(turnSpeed, TURN_CAP);
  setMotors(-sp, sp);
  Serial.printf("[LEFT]  L=-%d  R=+%d\n", sp, sp);
}

void cmdTurnRight() {
  int sp = min(turnSpeed, TURN_CAP);
  setMotors(sp, -sp);
  Serial.printf("[RIGHT] L=+%d  R=-%d\n", sp, sp);
}

void cmdStop() {
  setMotors(0, 0);
  Serial.println("[STOP]");
}

void cmdReset() {
  cmdStop();
  fwdSpeed  = 30;
  turnSpeed = 40;
  Serial.println("[RESET] fwdSpeed=30  turnSpeed=40");
}

void printStatus() {
  Serial.printf(
    "{ fwd_speed:%d  turn_speed:%d  fwd_cap:%d  turn_cap:%d"
    "  running:%s  watchdog_ms:%d }\n",
    fwdSpeed, turnSpeed, FWD_CAP, TURN_CAP,
    running ? "true" : "false", WATCHDOG_MS
  );
}

// ================================================================
//  COMMAND HANDLING
// ================================================================

void handleChar(char c) {
  lastCmdMs = millis();
  switch (c) {
    case 'w': case 'W': case 'f': case 'F': cmdForward();   break;
    case 's': case 'S': case 'b': case 'B': cmdBackward();  break;
    case 'a': case 'A': case 'l': case 'L': cmdTurnLeft();  break;
    case 'd': case 'D': case 'r': case 'R': cmdTurnRight(); break;
    case 'x': case 'X': case ' ':           cmdStop();      break;
    case 'h': case 'H':                     cmdReset();      break;
    case 'q': case 'Q':
      fwdSpeed = min(fwdSpeed + 5, FWD_CAP);
      Serial.printf("[SPD] fwdSpeed=%d\n", fwdSpeed); break;
    case 'z': case 'Z':
      fwdSpeed = max(fwdSpeed - 5, 5);
      Serial.printf("[SPD] fwdSpeed=%d\n", fwdSpeed); break;
    case 'e': case 'E':
      turnSpeed = min(turnSpeed + 5, TURN_CAP);
      Serial.printf("[SPD] turnSpeed=%d\n", turnSpeed); break;
    case 'c': case 'C':
      turnSpeed = max(turnSpeed - 5, 5);
      Serial.printf("[SPD] turnSpeed=%d\n", turnSpeed); break;
    case '?': printStatus(); break;
    default:  break;
  }
}

void handleLine(String &line) {
  line.trim();
  if (line.length() == 0) return;
  lastCmdMs = millis();

  // Single-char line (Serial Monitor sent just the char + newline)
  if (line.length() == 1) { handleChar(line[0]); return; }

  if (line.equalsIgnoreCase("STOP"))   { cmdStop();     return; }
  if (line.equalsIgnoreCase("RESET"))  { cmdReset();    return; }
  if (line.equalsIgnoreCase("STATUS")) { printStatus(); return; }

  // MOVE:<L>,<R>
  String upper = line;
  upper.toUpperCase();
  if (upper.startsWith("MOVE:")) {
    String args  = line.substring(5);
    int    comma = args.indexOf(',');
    if (comma < 0) { Serial.println("[ERR] Usage: MOVE:<L>,<R>"); return; }
    int L = constrain(args.substring(0, comma).toInt(), -FWD_CAP, FWD_CAP);
    int R = constrain(args.substring(comma + 1).toInt(), -FWD_CAP, FWD_CAP);
    setMotors(L, R);
    Serial.printf("[MOVE] L=%d  R=%d\n", L, R);
    return;
  }

  Serial.printf("[WARN] Unknown cmd: %s\n", line.c_str());
}

void processSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      handleLine(cmdBuf);
      cmdBuf = "";
    } else {
      if (cmdBuf.length() < 64) cmdBuf += c;
      else cmdBuf = "";
    }
  }
}

// ================================================================
//  SETUP & LOOP
// ================================================================

void setup() {
  Serial.begin(115200);
  delay(400);

  pinMode(DIR_L,  OUTPUT); digitalWrite(DIR_L,  LOW);
  pinMode(DIR_R,  OUTPUT); digitalWrite(DIR_R,  LOW);
  pinMode(LED_PIN, OUTPUT); digitalWrite(LED_PIN, LOW);

  pwmSetup();
  pwmWriteLeft(0);
  pwmWriteRight(0);

  lastCmdMs = millis();

  // ── Print core version so you can verify ────────────────
  Serial.println();
  Serial.println("╔══════════════════════════════════════════╗");
  Serial.println("║  Diff Drive Teleop  |  ESP32-S3 + DD10A  ║");
  Serial.println("║  v1.1  core-safe LEDC                    ║");
  Serial.println("╚══════════════════════════════════════════╝");
  Serial.printf ("ESP32 Arduino core : v%d.x\n", ESP_ARDUINO_VERSION_MAJOR);
  Serial.printf ("Fwd speed  : %d%% (cap %d%%)\n", fwdSpeed,  FWD_CAP);
  Serial.printf ("Turn speed : %d%% (cap %d%%)\n", turnSpeed, TURN_CAP);
  Serial.printf ("Watchdog   : %s (%d ms)\n",
                 WATCHDOG_ENABLE ? "ON" : "OFF", WATCHDOG_MS);
  Serial.println();
  Serial.println("!! Serial Monitor → set line ending to 'Newline' !!");
  Serial.println("W/S/A/D = move   X = stop   H = reset   ? = status");
  Serial.println("MOVE:<L>,<R>  e.g.  MOVE:40,-40");
  Serial.println("Ready.\n");
}

void loop() {
  processSerial();

#if WATCHDOG_ENABLE
  if (running && (millis() - lastCmdMs > WATCHDOG_MS)) {
    Serial.println("[WDG] timeout → STOP");
    cmdStop();
  }
#endif
}
