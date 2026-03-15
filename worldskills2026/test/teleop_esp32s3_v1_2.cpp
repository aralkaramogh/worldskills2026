/**
 * ================================================================
 *  Diff Drive Teleop  |  ESP32-S3 DevKit  |  Cytron DD10A
 *  v1.2 — Immediate single-char response (PIO monitor compatible)
 * ================================================================
 *
 *  Pins:
 *   Left  Motor : DIR=4   PWM=5
 *   Right Motor : DIR=6  PWM=7
 *
 *  Works in PlatformIO monitor with NO special line-ending config.
 *  Single chars (W/S/A/D/X etc.) fire instantly on keypress.
 *  Multi-char commands (MOVE:50,-50) still need Enter to submit.
 *
 * ================================================================
 */

#include <Arduino.h>

// ── Core version detection (LEDC API changed in v3.x) ────────
#ifndef ESP_ARDUINO_VERSION_MAJOR
  #define ESP_ARDUINO_VERSION_MAJOR 2
#endif

// ── Pins ─────────────────────────────────────────────────────
#define DIR_L    4
#define PWM_L    5
#define DIR_R    6
#define PWM_R    7
#define LED_PIN  2

// ── LEDC ─────────────────────────────────────────────────────
#define LEDC_FREQ  20000
#define LEDC_BITS  8
#define CH_L       0
#define CH_R       1

// ── Motor inversion ──────────────────────────────────────────
// If a motor spins the wrong way, flip its flag to true
#define INVERT_LEFT  false
#define INVERT_RIGHT true

// ── Watchdog ─────────────────────────────────────────────────
#define WATCHDOG_ENABLE 1
#define WATCHDOG_MS     50    // ms — auto-stop if no cmd received (reduced for immediate stop)

// ── Speed ────────────────────────────────────────────────────
int fwdSpeed  = 30;
int turnSpeed = 40;

const int FWD_CAP  = 80;
const int TURN_CAP = 80;

// ── State ────────────────────────────────────────────────────
bool          running   = false;
unsigned long lastCmdMs = 0;
String        cmdBuf    = "";    // buffer for multi-char commands

// ================================================================
//  PWM  (works on core v2.x AND v3.x)
// ================================================================

void pwmSetup() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PWM_L, LEDC_FREQ, LEDC_BITS);
  ledcAttach(PWM_R, LEDC_FREQ, LEDC_BITS);
#else
  ledcSetup(CH_L, LEDC_FREQ, LEDC_BITS);
  ledcSetup(CH_R, LEDC_FREQ, LEDC_BITS);
  ledcAttachPin(PWM_L, CH_L);
  ledcAttachPin(PWM_R, CH_R);
#endif
}

void pwmL(int v) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PWM_L, v);
#else
  ledcWrite(CH_L, v);
#endif
}

void pwmR(int v) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PWM_R, v);
#else
  ledcWrite(CH_R, v);
#endif
}

// ================================================================
//  MOTORS
// ================================================================

int pctTo8bit(int pct) {
  return map(constrain(abs(pct), 0, 100), 0, 100, 0, 255);
}

void setLeftMotor(int pct) {
  pct = constrain(pct, -100, 100);
  bool fwd = (pct >= 0);
  if (INVERT_LEFT)  fwd = !fwd;
  digitalWrite(DIR_L, fwd ? HIGH : LOW);
  pwmL(pctTo8bit(pct));
}

void setRightMotor(int pct) {
  pct = constrain(pct, -100, 100);
  bool fwd = (pct >= 0);
  if (INVERT_RIGHT) fwd = !fwd;
  digitalWrite(DIR_R, fwd ? HIGH : LOW);
  pwmR(pctTo8bit(pct));
}

void setMotors(int l, int r) {
  setLeftMotor(l);
  setRightMotor(r);
  running = (l != 0 || r != 0);
  digitalWrite(LED_PIN, running ? HIGH : LOW);
}

// ================================================================
//  MOTION
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
    "{ fwd:%d%% turn:%d%% fwd_cap:%d%% turn_cap:%d%% running:%s }\n",
    fwdSpeed, turnSpeed, FWD_CAP, TURN_CAP,
    running ? "true" : "false"
  );
}

// ================================================================
//  COMMAND DISPATCH  (single char — fires immediately)
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
      Serial.printf("[SPD] fwdSpeed=%d%%\n", fwdSpeed); break;
    case 'z': case 'Z':
      fwdSpeed = max(fwdSpeed - 5, 5);
      Serial.printf("[SPD] fwdSpeed=%d%%\n", fwdSpeed); break;
    case 'e': case 'E':
      turnSpeed = min(turnSpeed + 5, TURN_CAP);
      Serial.printf("[SPD] turnSpeed=%d%%\n", turnSpeed); break;
    case 'c': case 'C':
      turnSpeed = max(turnSpeed - 5, 5);
      Serial.printf("[SPD] turnSpeed=%d%%\n", turnSpeed); break;

    case '?': printStatus(); break;
    default:  break;                          // ignore unknown
  }
}

// ================================================================
//  STRUCTURED COMMAND  (MOVE:<L>,<R>  — needs Enter)
// ================================================================

void handleLine(String &line) {
  line.trim();
  if (line.length() == 0) return;
  lastCmdMs = millis();

  if (line.equalsIgnoreCase("STOP"))   { cmdStop();     return; }
  if (line.equalsIgnoreCase("RESET"))  { cmdReset();    return; }
  if (line.equalsIgnoreCase("STATUS")) { printStatus(); return; }

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

  Serial.printf("[WARN] Unknown: %s\n", line.c_str());
}

// ================================================================
//  SERIAL READER
//  Key logic:
//   - If incoming char is a known single-char command → fire NOW
//   - If it looks like a multi-char command → buffer until newline
// ================================================================

bool isKnownSingleChar(char c) {
  const char known[] = "wWfFsSbBaAlLdDrRxXhHqQzZeEcC ?";
  for (size_t i = 0; i < sizeof(known) - 1; i++) {
    if (c == known[i]) return true;
  }
  return false;
}

void processSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\n' || c == '\r') {
      // Flush whatever is buffered as a structured command
      if (cmdBuf.length() > 0) {
        handleLine(cmdBuf);
        cmdBuf = "";
      }
    } else if (cmdBuf.length() == 0 && isKnownSingleChar(c)) {
      // Buffer is empty and this is a known single-char key → instant fire
      handleChar(c);
    } else {
      // Building a multi-char command (e.g. MOVE:50,-50)
      if (cmdBuf.length() < 64) cmdBuf += c;
      else                      cmdBuf = "";   // overflow guard
    }
  }
}

// ================================================================
//  SETUP & LOOP
// ================================================================

void setup() {
  Serial.begin(115200);
  delay(400);

  pinMode(DIR_L,   OUTPUT); digitalWrite(DIR_L,   LOW);
  pinMode(DIR_R,   OUTPUT); digitalWrite(DIR_R,   LOW);
  pinMode(LED_PIN, OUTPUT); digitalWrite(LED_PIN, LOW);

  pwmSetup();
  pwmL(0);
  pwmR(0);

  lastCmdMs = millis();

  Serial.println();
  Serial.println("╔══════════════════════════════════════════╗");
  Serial.println("║  Diff Drive Teleop  |  ESP32-S3 + DD10A  ║");
  Serial.println("║  v1.2  instant keypress, no newline req  ║");
  Serial.println("╚══════════════════════════════════════════╝");
  Serial.printf ("Core v%d.x  |  PWM %d Hz  |  WDG %d ms\n",
                 ESP_ARDUINO_VERSION_MAJOR, LEDC_FREQ, WATCHDOG_MS);
  Serial.printf ("Fwd: %d%% (cap %d%%)   Turn: %d%% (cap %d%%)\n",
                 fwdSpeed, FWD_CAP, turnSpeed, TURN_CAP);
  Serial.println("W/S/A/D = move   X = stop   H = reset   ? = status");
  Serial.println("Q/Z = fwd speed   E/C = turn speed");
  Serial.println("Structured (needs Enter): MOVE:<L>,<R>  e.g. MOVE:40,-40");
  Serial.println("Ready — press a key\n");
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
