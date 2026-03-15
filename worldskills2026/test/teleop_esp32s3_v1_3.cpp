/**
 * ================================================================
 *  Diff Drive Teleop  |  ESP32-S3 DevKit  |  Cytron DD10A
 *  v1.3 — Serial toggle (V key) + Command Latching
 * ================================================================
 *
 *  Pins:
 *   Left  Motor : DIR=4   PWM=5
 *   Right Motor : DIR=6   PWM=7
 *
 *  Changes from v1.2:
 *   1. V key toggles ALL serial output on/off
 *   2. Motion commands latch — robot keeps moving after keypress
 *      until a new motion key or stop key is received.
 *      Watchdog is disabled in latch mode (motion is intentional).
 *
 *  Latch behaviour:
 *   Press W  → robot moves forward continuously
 *   Press D  → robot pivots right  (replaces W latch)
 *   Press X  → robot stops, latch cleared
 *   Press H  → reset, latch cleared
 *
 * ================================================================
 */

#include <Arduino.h>

// ── Core version (LEDC API changed in v3.x) ──────────────────
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
#define INVERT_LEFT  true
#define INVERT_RIGHT false

// ── Watchdog ─────────────────────────────────────────────────
// Only active when latch is CLEARED (no intentional motion).
// When a latch is held, watchdog is bypassed — motion is
// intentional and should not auto-stop.
#define WATCHDOG_ENABLE 1
#define WATCHDOG_MS     500

// ── Speed ────────────────────────────────────────────────────
int fwdSpeed  = 30;
int turnSpeed = 40;

const int FWD_CAP  = 80;
const int TURN_CAP = 80;

// ── Serial verbose toggle ─────────────────────────────────────
// false = silent (errors + watchdog only)
// true  = full motion feedback
// Toggle at runtime with V key
bool verbose = false;

// ── Latch state ───────────────────────────────────────────────
// Stores the last motion command char.
// '0' means no latch (stopped).
// Motion re-executes this command every LATCH_REPEAT_MS
// so speed changes (Q/Z/E/C) take effect immediately.
char latchedCmd   = '0';
#define LATCH_REPEAT_MS  100    // re-apply latched command every 100ms

// ── State ────────────────────────────────────────────────────
bool          running   = false;
unsigned long lastCmdMs = 0;
unsigned long lastLatchMs = 0;
String        cmdBuf    = "";

// ================================================================
//  LOG MACROS
//  LOG  — always prints (errors, watchdog, resets)
//  LOGV — only prints when verbose = true
// ================================================================
#define LOG(...)  Serial.printf(__VA_ARGS__)
#define LOGV(...) if (verbose) Serial.printf(__VA_ARGS__)

// ================================================================
//  PWM (core v2 + v3 compatible)
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
  ledcWrite(PWM_L, constrain(v, 0, 255));
#else
  ledcWrite(CH_L,  constrain(v, 0, 255));
#endif
}

void pwmR(int v) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PWM_R, constrain(v, 0, 255));
#else
  ledcWrite(CH_R,  constrain(v, 0, 255));
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
  if (INVERT_LEFT) fwd = !fwd;
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
//  MOTION COMMANDS
//  Each returns void and reads current speed globals,
//  so re-executing them after a speed change picks up new values.
// ================================================================

void cmdForward() {
  int sp = min(fwdSpeed, FWD_CAP);
  setMotors(sp, sp);
  LOGV("[FWD]   L=+%d  R=+%d\n", sp, sp);
}

void cmdBackward() {
  int sp = min(fwdSpeed, FWD_CAP);
  setMotors(-sp, -sp);
  LOGV("[BWD]   L=-%d  R=-%d\n", sp, sp);
}

void cmdTurnLeft() {
  int sp = min(turnSpeed, TURN_CAP);
  setMotors(-sp, sp);
  LOGV("[LEFT]  L=-%d  R=+%d\n", sp, sp);
}

void cmdTurnRight() {
  int sp = min(turnSpeed, TURN_CAP);
  setMotors(sp, -sp);
  LOGV("[RIGHT] L=+%d  R=-%d\n", sp, sp);
}

void cmdStop() {
  setMotors(0, 0);
  latchedCmd = '0';           // clear latch
  LOGV("[STOP]\n");
}

void cmdReset() {
  cmdStop();
  fwdSpeed  = 30;
  turnSpeed = 40;
  LOG("[RESET] fwdRpm=30  turnRpm=40\n");
}

void printStatus() {
  LOG("{ fwd:%d%% turn:%d%% fwd_cap:%d%% turn_cap:%d%%"
      " running:%s latch:%c verbose:%s }\n",
      fwdSpeed, turnSpeed, FWD_CAP, TURN_CAP,
      running ? "true" : "false",
      latchedCmd == '0' ? '-' : latchedCmd,
      verbose ? "on" : "off");
}

// ================================================================
//  EXECUTE MOTION BY CHAR
//  Used both by handleChar and the latch repeat loop.
// ================================================================

void executeMotionChar(char c) {
  switch (c) {
    case 'w': case 'W': case 'f': case 'F': cmdForward();   break;
    case 's': case 'S': case 'b': case 'B': cmdBackward();  break;
    case 'a': case 'A': case 'l': case 'L': cmdTurnLeft();  break;
    case 'd': case 'D': case 'r': case 'R': cmdTurnRight(); break;
    default: break;
  }
}

bool isMotionChar(char c) {
  switch (c) {
    case 'w': case 'W': case 'f': case 'F':
    case 's': case 'S': case 'b': case 'B':
    case 'a': case 'A': case 'l': case 'L':
    case 'd': case 'D': case 'r': case 'R':
      return true;
    default:
      return false;
  }
}

// ================================================================
//  SINGLE CHAR DISPATCH
// ================================================================

void handleChar(char c) {
  lastCmdMs = millis();

  if (isMotionChar(c)) {
    latchedCmd = c;             // latch this command
    lastLatchMs = millis();
    executeMotionChar(c);
    return;
  }

  switch (c) {
    case 'x': case 'X': case ' ': cmdStop();  break;
    case 'h': case 'H':           cmdReset(); break;

    case 'v': case 'V':
      verbose = !verbose;
      // Always print this confirmation regardless of verbose state
      LOG("[VERBOSE] %s\n", verbose ? "ON" : "OFF");
      break;

    case 'q': case 'Q':
      fwdSpeed = min(fwdSpeed + 5, FWD_CAP);
      LOG("[SPD] fwdSpeed=%d%%\n", fwdSpeed);
      break;
    case 'z': case 'Z':
      fwdSpeed = max(fwdSpeed - 5, 5);
      LOG("[SPD] fwdSpeed=%d%%\n", fwdSpeed);
      break;
    case 'e': case 'E':
      turnSpeed = min(turnSpeed + 5, TURN_CAP);
      LOG("[SPD] turnSpeed=%d%%\n", turnSpeed);
      break;
    case 'c': case 'C':
      turnSpeed = max(turnSpeed - 5, 5);
      LOG("[SPD] turnSpeed=%d%%\n", turnSpeed);
      break;

    case '?': printStatus(); break;
    default:  break;
  }
}

// ================================================================
//  STRUCTURED COMMAND PARSER  (MOVE / STOP / RESET / STATUS)
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
    if (comma < 0) { LOG("[ERR] Usage: MOVE:<L>,<R>\n"); return; }
    int L = constrain(args.substring(0, comma).toInt(), -FWD_CAP, FWD_CAP);
    int R = constrain(args.substring(comma + 1).toInt(), -FWD_CAP, FWD_CAP);
    setMotors(L, R);
    // MOVE clears latch — direct duty control, not a latched key
    latchedCmd = '0';
    LOGV("[MOVE] L=%d  R=%d\n", L, R);
    return;
  }

  LOG("[WARN] Unknown: %s\n", line.c_str());
}

// ================================================================
//  SERIAL READER
// ================================================================

bool isKnownSingleChar(char c) {
  const char known[] = "wWfFsSbBaAlLdDrRxXhHvVqQzZeEcC ?";
  for (size_t i = 0; known[i]; i++) if (c == known[i]) return true;
  return false;
}

void processSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (cmdBuf.length() > 0) { handleLine(cmdBuf); cmdBuf = ""; }
    } else if (cmdBuf.length() == 0 && isKnownSingleChar(c)) {
      handleChar(c);
    } else {
      if (cmdBuf.length() < 64) cmdBuf += c;
      else cmdBuf = "";
    }
  }
}

// ================================================================
//  LATCH LOOP
//  Re-applies the latched command every LATCH_REPEAT_MS.
//  This ensures speed changes (Q/Z/E/C) take effect immediately
//  even while the robot is already moving.
// ================================================================

void latchUpdate() {
  if (latchedCmd == '0') return;
  if (millis() - lastLatchMs < LATCH_REPEAT_MS) return;
  lastLatchMs = millis();
  executeMotionChar(latchedCmd);
}

// ================================================================
//  SETUP
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

  lastCmdMs  = millis();
  lastLatchMs = millis();

  LOG("\n╔══════════════════════════════════════════╗\n");
  LOG("║  Diff Drive Teleop  |  ESP32-S3 + DD10A  ║\n");
  LOG("║  v1.3  latch + serial toggle              ║\n");
  LOG("╚══════════════════════════════════════════╝\n");
  LOG("Core v%d | PWM %dHz | WDG %dms\n",
      ESP_ARDUINO_VERSION_MAJOR, LEDC_FREQ, WATCHDOG_MS);
  LOG("Fwd:%d%% Turn:%d%% (caps %d%%/%d%%)\n",
      fwdSpeed, turnSpeed, FWD_CAP, TURN_CAP);
  LOG("V=verbose toggle | ?=status | W/S/A/D=move | X=stop\n");
  LOG("Q/Z=fwd speed  E/C=turn speed\n");
  LOG("MOVE:<L>,<R> (needs Enter)\n");
  LOG("Verbose is OFF by default. Press V to enable.\n\n");
}

// ================================================================
//  LOOP
// ================================================================

void loop() {
  processSerial();

  latchUpdate();

#if WATCHDOG_ENABLE
  // Watchdog only fires when there is NO latch active.
  // If a latch is held, motion is intentional — don't stop it.
  if (latchedCmd == '0' && running && (millis() - lastCmdMs > WATCHDOG_MS)) {
    LOG("[WDG] timeout → STOP\n");
    cmdStop();
  }
#endif
}
