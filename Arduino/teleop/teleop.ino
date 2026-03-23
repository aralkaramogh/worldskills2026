/*
 * ================================================================
 *  Diff Drive Teleop  |  ESP32-S3 DevKit  |  Cytron DD10A
 *  Compatible with ESP32 Arduino Core 3.x
 * ================================================================
 */

// ── Pins ─────────────────────────────────────────────────────
#define DIR_L    4
#define PWM_L    5
#define DIR_R    6
#define PWM_R    7
#define LED_PIN  2

// ── LEDC ─────────────────────────────────────────────────────
#define LEDC_FREQ  20000
#define LEDC_BITS  8

// ── Motor inversion ──────────────────────────────────────────
#define INVERT_LEFT  true
#define INVERT_RIGHT false

// ── Watchdog ─────────────────────────────────────────────────
#define WATCHDOG_ENABLE 1
#define WATCHDOG_MS     500

// ── Speed ────────────────────────────────────────────────────
int fwdSpeed  = 30;
int turnSpeed = 40;

const int FWD_CAP  = 80;
const int TURN_CAP = 80;

// ── Serial verbose toggle ─────────────────────────────────────
bool verbose = false;

// ── Latch state ───────────────────────────────────────────────
char latchedCmd   = '0';
#define LATCH_REPEAT_MS  100

// ── State ────────────────────────────────────────────────────
bool          running   = false;
unsigned long lastCmdMs = 0;
unsigned long lastLatchMs = 0;
String        cmdBuf    = "";

// ── Logging ──────────────────────────────────────────────────
#define LOG(...)  Serial.printf(__VA_ARGS__)
#define LOGV(...) if (verbose) Serial.printf(__VA_ARGS__)

// ================================================================
// PWM SETUP (ESP32 Core 3.x compatible)
// ================================================================
void pwmSetup() {
  // Attach each PWM pin with frequency and resolution
  ledcAttach(PWM_L, LEDC_FREQ, LEDC_BITS);
  ledcAttach(PWM_R, LEDC_FREQ, LEDC_BITS);
}

void pwmL(int v) {
  ledcWrite(PWM_L, constrain(v, 0, 255));
}

void pwmR(int v) {
  ledcWrite(PWM_R, constrain(v, 0, 255));
}

// ================================================================
// MOTORS
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
// MOTION COMMANDS
// ================================================================
void cmdForward() {
  int sp = min(fwdSpeed, FWD_CAP);
  setMotors(sp, sp);
  LOGV("[FWD]\n");
}

void cmdBackward() {
  int sp = min(fwdSpeed, FWD_CAP);
  setMotors(-sp, -sp);
  LOGV("[BWD]\n");
}

void cmdTurnLeft() {
  int sp = min(turnSpeed, TURN_CAP);
  setMotors(-sp, sp);
  LOGV("[LEFT]\n");
}

void cmdTurnRight() {
  int sp = min(turnSpeed, TURN_CAP);
  setMotors(sp, -sp);
  LOGV("[RIGHT]\n");
}

void cmdStop() {
  setMotors(0, 0);
  latchedCmd = '0';
  LOGV("[STOP]\n");
}

void cmdReset() {
  cmdStop();
  fwdSpeed  = 30;
  turnSpeed = 40;
  LOG("[RESET]\n");
}

void printStatus() {
  LOG("{ fwd:%d turn:%d running:%s latch:%c verbose:%s }\n",
      fwdSpeed, turnSpeed,
      running ? "true" : "false",
      latchedCmd == '0' ? '-' : latchedCmd,
      verbose ? "on" : "off");
}

// ================================================================
// COMMAND HANDLING
// ================================================================
void executeMotionChar(char c) {
  switch (c) {
    case 'w': case 'W': cmdForward(); break;
    case 's': case 'S': cmdBackward(); break;
    case 'a': case 'A': cmdTurnLeft(); break;
    case 'd': case 'D': cmdTurnRight(); break;
  }
}

bool isMotionChar(char c) {
  return (c=='w'||c=='W'||c=='s'||c=='S'||c=='a'||c=='A'||c=='d'||c=='D');
}

void handleChar(char c) {
  lastCmdMs = millis();

  if (isMotionChar(c)) {
    latchedCmd = c;
    lastLatchMs = millis();
    executeMotionChar(c);
    return;
  }

  switch (c) {
    case 'x': case 'X': cmdStop(); break;
    case 'h': case 'H': cmdReset(); break;

    case 'v': case 'V':
      verbose = !verbose;
      LOG("[VERBOSE] %s\n", verbose ? "ON" : "OFF");
      break;

    case 'q': fwdSpeed = min(fwdSpeed+5, FWD_CAP); break;
    case 'z': fwdSpeed = max(fwdSpeed-5, 5); break;
    case 'e': turnSpeed = min(turnSpeed+5, TURN_CAP); break;
    case 'c': turnSpeed = max(turnSpeed-5, 5); break;

    case '?': printStatus(); break;
  }
}

// ================================================================
// SERIAL
// ================================================================
void processSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    handleChar(c);
  }
}

// ================================================================
// LATCH
// ================================================================
void latchUpdate() {
  if (latchedCmd == '0') return;
  if (millis() - lastLatchMs < LATCH_REPEAT_MS) return;
  lastLatchMs = millis();
  executeMotionChar(latchedCmd);
}

// ================================================================
// SETUP
// ================================================================
void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(DIR_L, OUTPUT);
  pinMode(DIR_R, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  pwmSetup();

  LOG("Diff Drive Ready\n");
}

// ================================================================
// LOOP
// ================================================================
void loop() {
  processSerial();
  latchUpdate();

#if WATCHDOG_ENABLE
  if (latchedCmd == '0' && running && (millis() - lastCmdMs > WATCHDOG_MS)) {
    cmdStop();
  }
#endif
}