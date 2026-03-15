/**
 * ================================================================
 *  Diff Drive  |  ESP32-S3  |  Cytron DD10A  |  HD Hex + Encoder
 *  v3.0 — PID RPM Control + Odometry + ROS2 Serial Bridge
 * ================================================================
 *
 *  ── Motor Pins ───────────────────────────────────────────────
 *   Left  Motor : DIR=4   PWM=5
 *   Right Motor : DIR=6   PWM=7
 *
 *  ── Encoder Pins ─────────────────────────────────────────────
 *   Left  Encoder : A=15  B=16   ← already connected
 *   Right Encoder : A=17  B=18   ← connect same way as left
 *
 *   Each HD Hex encoder wire:
 *     Red/White  → ESP32 3.3V
 *     Black      → ESP32 GND
 *     A          → ENC_x_A pin
 *     B          → ENC_x_B pin
 *
 *  ── Robot Physical Parameters (MEASURE AND SET THESE) ────────
 *   WHEEL_RADIUS_M  : measure wheel outer radius in metres?
 *   TRACK_WIDTH_M   : measure left-to-right wheel centre distance
 *   GEAR_RATIO      : your Ultra Planetary ratio (e.g. 20 for 20:1)
 *   HD Hex raw CPR  : 28 counts/rev before gearbox
 *   Quadrature ×4   : 28 × 4 × GEAR_RATIO = ticks per output rev
 *
 *  ── Serial Protocol (ROS2 bridge on RPi talks this) ──────────
 *
 *   RPi → ESP32 commands:
 *     MOVE:<L_rpm>,<R_rpm>\n     e.g.  MOVE:60,-60
 *     STOP\n
 *     RESET\n
 *     STATUS\n
 *     PID:<Kp>,<Ki>,<Kd>\n       live gain tuning
 *
 *   ESP32 → RPi periodic odometry (every ODOM_PUBLISH_MS):
 *     {"x":0.00,"y":0.00,"th":0.000,
 *      "vl":0.00,"vr":0.00,"seq":0}\n
 *
 *     x, y   : position in metres
 *     th     : heading in radians (-π to π)
 *     vl, vr : actual wheel linear velocities (m/s)
 *     seq    : incrementing counter for packet loss detection
 *
 *  ── Keyboard Commands (single char, no Enter needed) ─────────
 *   W/S/A/D  Move    X  Stop    H  Reset
 *   Q/Z      Fwd RPM ±10       E/C  Turn RPM ±10
 *   V        Toggle verbose serial output
 *   ?        Print full STATUS JSON
 *
 *  ── ROS2 RPi Node (what to write on the Pi side) ─────────────
 *
 *   Node subscribes /cmd_vel (geometry_msgs/Twist):
 *     vl = msg.linear.x - msg.angular.z * TRACK_WIDTH/2
 *     vr = msg.linear.x + msg.angular.z * TRACK_WIDTH/2
 *     rpm_l = vl / WHEEL_RADIUS / (2*pi/60)
 *     rpm_r = vr / WHEEL_RADIUS / (2*pi/60)
 *     serial.write(f"MOVE:{rpm_l:.0f},{rpm_r:.0f}\n")
 *
 *   Node reads odom JSON, publishes:
 *     /odom  (nav_msgs/Odometry)
 *     /tf    (odom → base_link transform)
 *
 * ================================================================
 */

#include <Arduino.h>
#include <math.h>

// ── Core version (LEDC API v2 vs v3) ─────────────────────────
#ifndef ESP_ARDUINO_VERSION_MAJOR
  #define ESP_ARDUINO_VERSION_MAJOR 2
#endif

// ================================================================
//  HARDWARE CONFIG  ← edit to match your build
// ================================================================

// Motor driver pins
#define DIR_L    4
#define PWM_L    5
#define DIR_R    6
#define PWM_R    7
#define LED_PIN  2

// Encoder pins
#define ENC_L_A  16
#define ENC_L_B  15
#define ENC_R_A  17    // connect right encoder A here
#define ENC_R_B  18    // connect right encoder B here

// Motor inversion — one motor faces opposite direction on chassis
#define INVERT_LEFT   true
#define INVERT_RIGHT  false

// ── Physical robot parameters — MEASURE YOURS ────────────────
#define WHEEL_RADIUS_M   0.046f    // metres — e.g. 50mm radius wheel
#define TRACK_WIDTH_M    0.32f    // metres — wheel centre to wheel centre

// ── Encoder resolution ────────────────────────────────────────
#define ENC_CPR       28          // HD Hex raw counts per motor rev
#define GEAR_RATIO    20          // Ultra Planetary ratio (3,4,5,16,20...)
#define TICKS_PER_REV (ENC_CPR * 4 * GEAR_RATIO)   // 2240 for 20:1

// ── Speed limits ─────────────────────────────────────────────
#define MAX_RPM       150
#define DEFAULT_FWD_RPM   60
#define DEFAULT_TURN_RPM  40

// ── Timing ───────────────────────────────────────────────────
#define PID_INTERVAL_MS    50     // PID runs at 20 Hz
#define ODOM_PUBLISH_MS    50     // odometry sent to RPi at 20 Hz

// ── Watchdog ─────────────────────────────────────────────────
#define WATCHDOG_ENABLE    1
#define WATCHDOG_MS        500    // ms — stop if no cmd received

// ── PID gains (tune with PID:<Kp>,<Ki>,<Kd> command) ─────────
float KP = 1.5f;
float KI = 0.08f;
float KD = 0.02f;
#define INTEGRAL_LIMIT  80.0f    // anti-windup clamp

// ================================================================
//  LEDC PWM (v2 + v3 compatible)
// ================================================================

#define LEDC_FREQ   20000
#define LEDC_BITS   8
#define CH_L        0
#define CH_R        1

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
//  ENCODERS  (ISR — quadrature X4 decoding)
// ================================================================

volatile long encTicksL = 0;    // cumulative, used for odometry
volatile long encTicksR = 0;

void IRAM_ATTR isrLA() { encTicksL += (digitalRead(ENC_L_A) == digitalRead(ENC_L_B)) ? -1 : 1; }
void IRAM_ATTR isrLB() { encTicksL += (digitalRead(ENC_L_A) != digitalRead(ENC_L_B)) ? -1 : 1; }
void IRAM_ATTR isrRA() { encTicksR += (digitalRead(ENC_R_A) == digitalRead(ENC_R_B)) ? -1 : 1; }
void IRAM_ATTR isrRB() { encTicksR += (digitalRead(ENC_R_A) != digitalRead(ENC_R_B)) ? -1 : 1; }

// ================================================================
//  PID  (one instance per motor)
// ================================================================

struct MotorPID {
  float setpoint  = 0.0f;   // target RPM
  float integral  = 0.0f;
  float prevError = 0.0f;
  int   dutyOut   = 0;       // last duty 0-255

  void reset() {
    setpoint = 0; integral = 0; prevError = 0; dutyOut = 0;
  }

  // Returns duty 0-255. Direction is handled separately via DIR pin.
  int compute(float actualRpm, float dt) {
    if (setpoint == 0.0f) {
      integral = 0; prevError = 0;
      return 0;
    }
    float error = setpoint - actualRpm;

    integral += error * dt;
    integral  = constrain(integral, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);

    // Derivative on measurement (not error) — prevents derivative kick
    float deriv = (error - prevError) / dt;
    prevError = error;

    float out  = KP * error + KI * integral + KD * deriv;
    int   duty = (int)(fabs(out) / (float)MAX_RPM * 255.0f);
    duty = constrain(duty, 0, 255);

    dutyOut = duty;
    return duty;
  }
};

MotorPID pidL, pidR;

// ================================================================
//  MOTORS  (raw driver)
// ================================================================

void driveLeft(int duty, float sp) {
  bool fwd = (sp >= 0);
  if (INVERT_LEFT) fwd = !fwd;
  digitalWrite(DIR_L, fwd ? HIGH : LOW);
  pwmL(duty);
}

void driveRight(int duty, float sp) {
  bool fwd = (sp >= 0);
  if (INVERT_RIGHT) fwd = !fwd;
  digitalWrite(DIR_R, fwd ? HIGH : LOW);
  pwmR(duty);
}

void hardStop() {
  pwmL(0); pwmR(0);
  digitalWrite(DIR_L, LOW);
  digitalWrite(DIR_R, LOW);
  pidL.reset(); pidR.reset();
  digitalWrite(LED_PIN, LOW);
}

void setTargetRpm(float l, float r) {
  pidL.setpoint = constrain(l, -MAX_RPM, MAX_RPM);
  pidR.setpoint = constrain(r, -MAX_RPM, MAX_RPM);
  if (l != 0 || r != 0) digitalWrite(LED_PIN, HIGH);
  else                   digitalWrite(LED_PIN, LOW);
}

// ================================================================
//  ODOMETRY
//  Integrates wheel ticks into 2D pose (x, y, heading).
//  Run at PID_INTERVAL_MS cadence — same loop as PID.
// ================================================================

struct Odometry {
  float x   = 0.0f;   // metres
  float y   = 0.0f;   // metres
  float th  = 0.0f;   // radians
  float vl  = 0.0f;   // left  wheel m/s (actual)
  float vr  = 0.0f;   // right wheel m/s (actual)
  long  seq = 0;
} odom;

long prevOdomTicksL = 0;
long prevOdomTicksR = 0;

void odomUpdate(float dt) {
  // Snapshot ticks atomically
  noInterrupts();
  long curL = encTicksL;
  long curR = encTicksR;
  interrupts();

  long dL = curL - prevOdomTicksL;
  long dR = curR - prevOdomTicksR;
  prevOdomTicksL = curL;
  prevOdomTicksR = curR;

  // Convert ticks → metres
  float distL = (dL / (float)TICKS_PER_REV) * (2.0f * M_PI * WHEEL_RADIUS_M);
  float distR = (dR / (float)TICKS_PER_REV) * (2.0f * M_PI * WHEEL_RADIUS_M);

  // Wheel velocities m/s
  odom.vl = distL / dt;
  odom.vr = distR / dt;

  // Differential drive kinematics
  float distCentre = (distL + distR) / 2.0f;
  float dTheta     = (distR - distL) / TRACK_WIDTH_M;

  odom.x  += distCentre * cosf(odom.th + dTheta / 2.0f);
  odom.y  += distCentre * sinf(odom.th + dTheta / 2.0f);
  odom.th += dTheta;

  // Wrap heading to -π .. π
  while (odom.th >  M_PI) odom.th -= 2.0f * M_PI;
  while (odom.th < -M_PI) odom.th += 2.0f * M_PI;
}

// ================================================================
//  SERIAL OUTPUT
// ================================================================

bool verbose = false;

#define LOG(...)  Serial.printf(__VA_ARGS__)
#define LOGV(...) if (verbose) Serial.printf(__VA_ARGS__)

// Called every ODOM_PUBLISH_MS — RPi parses this for /odom topic
void publishOdom() {
  odom.seq++;
  Serial.printf(
    "{\"x\":%.4f,\"y\":%.4f,\"th\":%.4f,"
    "\"vl\":%.4f,\"vr\":%.4f,\"seq\":%ld}\n",
    odom.x, odom.y, odom.th,
    odom.vl, odom.vr, odom.seq
  );
}

void printStatus() {
  noInterrupts();
  long tL = encTicksL;
  long tR = encTicksR;
  interrupts();

  // Actual RPM (computed in pidUpdate)
  Serial.printf(
    "{\"sp_l\":%.0f,\"sp_r\":%.0f,"
    "\"duty_l\":%d,\"duty_r\":%d,"
    "\"ticks_l\":%ld,\"ticks_r\":%ld,"
    "\"x\":%.4f,\"y\":%.4f,\"th\":%.4f,"
    "\"kp\":%.3f,\"ki\":%.3f,\"kd\":%.3f,"
    "\"verbose\":%s}\n",
    pidL.setpoint, pidR.setpoint,
    pidL.dutyOut, pidR.dutyOut,
    tL, tR,
    odom.x, odom.y, odom.th,
    KP, KI, KD,
    verbose ? "true" : "false"
  );
}

// ================================================================
//  PID + ODOM UPDATE LOOP  (called in main loop, rate-limited)
// ================================================================

unsigned long lastPidMs  = 0;
unsigned long lastOdomMs = 0;
float actualRpmL = 0, actualRpmR = 0;
long  prevPidTicksL = 0, prevPidTicksR = 0;

void pidUpdate() {
  unsigned long now     = millis();
  unsigned long elapsed = now - lastPidMs;
  if (elapsed < PID_INTERVAL_MS) return;
  lastPidMs = now;

  float dt = elapsed / 1000.0f;

  // ── RPM calculation ──────────────────────────────────────
  noInterrupts();
  long curL = encTicksL;
  long curR = encTicksR;
  interrupts();

  long dL = curL - prevPidTicksL;
  long dR = curR - prevPidTicksR;
  prevPidTicksL = curL;
  prevPidTicksR = curR;

  actualRpmL = (dL / (float)TICKS_PER_REV) / dt * 60.0f;
  actualRpmR = (dR / (float)TICKS_PER_REV) / dt * 60.0f;

  // ── PID compute ──────────────────────────────────────────
  int dutyL = pidL.compute(actualRpmL, dt);
  int dutyR = pidR.compute(actualRpmR, dt);

  if (pidL.setpoint == 0 && pidR.setpoint == 0) {
    pwmL(0); pwmR(0);
  } else {
    driveLeft (dutyL, pidL.setpoint);
    driveRight(dutyR, pidR.setpoint);
  }

  // ── Odometry update ──────────────────────────────────────
  odomUpdate(dt);

  LOGV("[PID] L sp=%.0f act=%.1f duty=%d | R sp=%.0f act=%.1f duty=%d\n",
       pidL.setpoint, actualRpmL, dutyL,
       pidR.setpoint, actualRpmR, dutyR);
}

void odomPublishLoop() {
  if (millis() - lastOdomMs < ODOM_PUBLISH_MS) return;
  lastOdomMs = millis();
  publishOdom();
}

// ================================================================
//  COMMANDS
// ================================================================

int fwdRpm  = DEFAULT_FWD_RPM;
int turnRpm = DEFAULT_TURN_RPM;
unsigned long lastCmdMs = 0;

// ── Latch ────────────────────────────────────────────────────
char latchedCmd = '0';
unsigned long lastLatchMs = 0;
#define LATCH_REPEAT_MS  100

void executeMotionChar(char c);   // forward declaration

void cmdStop() {
  setTargetRpm(0, 0);
  latchedCmd = '0';
  LOGV("[STOP]\n");
}

void cmdReset() {
  hardStop();
  fwdRpm  = DEFAULT_FWD_RPM;
  turnRpm = DEFAULT_TURN_RPM;
  latchedCmd = '0';
  odom = {};
  LOG("[RESET]\n");
}

void cmdForward()   { setTargetRpm( fwdRpm,  fwdRpm);  LOGV("[FWD]  %d RPM\n",  fwdRpm);  }
void cmdBackward()  { setTargetRpm(-fwdRpm, -fwdRpm);  LOGV("[BWD]  %d RPM\n",  fwdRpm);  }
void cmdTurnLeft()  { setTargetRpm(-turnRpm, turnRpm);  LOGV("[LEFT] %d RPM\n", turnRpm);  }
void cmdTurnRight() { setTargetRpm( turnRpm,-turnRpm);  LOGV("[RIGHT] %d RPM\n", turnRpm); }

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
    default: return false;
  }
}

void latchUpdate() {
  if (latchedCmd == '0') return;
  if (millis() - lastLatchMs < LATCH_REPEAT_MS) return;
  lastLatchMs = millis();
  executeMotionChar(latchedCmd);
}

// ================================================================
//  SINGLE CHAR DISPATCH
// ================================================================

void handleChar(char c) {
  lastCmdMs = millis();

  if (isMotionChar(c)) {
    latchedCmd  = c;
    lastLatchMs = millis();
    executeMotionChar(c);
    return;
  }

  switch (c) {
    case 'x': case 'X': case ' ': cmdStop();  break;
    case 'h': case 'H':           cmdReset(); break;

    case 'v': case 'V':
      verbose = !verbose;
      LOG("[VERBOSE] %s\n", verbose ? "ON" : "OFF");
      break;

    case 'q': case 'Q':
      fwdRpm = min(fwdRpm + 10, MAX_RPM);
      LOG("[SPD] fwdRpm=%d\n", fwdRpm); break;
    case 'z': case 'Z':
      fwdRpm = max(fwdRpm - 10, 5);
      LOG("[SPD] fwdRpm=%d\n", fwdRpm); break;
    case 'e': case 'E':
      turnRpm = min(turnRpm + 10, MAX_RPM);
      LOG("[SPD] turnRpm=%d\n", turnRpm); break;
    case 'c': case 'C':
      turnRpm = max(turnRpm - 10, 5);
      LOG("[SPD] turnRpm=%d\n", turnRpm); break;

    case '?': printStatus(); break;
    default:  break;
  }
}

// ================================================================
//  STRUCTURED COMMAND PARSER
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

  // MOVE:<L_rpm>,<R_rpm>
  if (upper.startsWith("MOVE:")) {
    String args  = line.substring(5);
    int    comma = args.indexOf(',');
    if (comma < 0) { LOG("[ERR] MOVE:<L>,<R>\n"); return; }
    float L = constrain(args.substring(0, comma).toFloat(), -MAX_RPM, MAX_RPM);
    float R = constrain(args.substring(comma + 1).toFloat(), -MAX_RPM, MAX_RPM);
    setTargetRpm(L, R);
    latchedCmd = '0';    // MOVE from RPi does not latch
    LOGV("[MOVE] L=%.0f R=%.0f RPM\n", L, R);
    return;
  }

  // PID:<Kp>,<Ki>,<Kd>
  if (upper.startsWith("PID:")) {
    String args = line.substring(4);
    int c1 = args.indexOf(',');
    int c2 = args.lastIndexOf(',');
    if (c1 < 0 || c1 == c2) { LOG("[ERR] PID:<Kp>,<Ki>,<Kd>\n"); return; }
    KP = args.substring(0, c1).toFloat();
    KI = args.substring(c1 + 1, c2).toFloat();
    KD = args.substring(c2 + 1).toFloat();
    pidL.integral = 0; pidR.integral = 0;
    LOG("[PID] Kp=%.3f Ki=%.3f Kd=%.3f\n", KP, KI, KD);
    return;
  }

  // RESETODOM  — zero pose without stopping motors
  if (upper == "RESETODOM") {
    odom = {};
    LOG("[ODOM] Reset\n");
    return;
  }

  LOG("[WARN] Unknown: %s\n", line.c_str());
}

// ================================================================
//  SERIAL READER
// ================================================================

String cmdBuf = "";

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
//  SETUP
// ================================================================

void setup() {
  Serial.begin(115200);
  delay(400);

  // Motor pins
  pinMode(DIR_L,   OUTPUT); digitalWrite(DIR_L,   LOW);
  pinMode(DIR_R,   OUTPUT); digitalWrite(DIR_R,   LOW);
  pinMode(LED_PIN, OUTPUT); digitalWrite(LED_PIN, LOW);

  // PWM
  pwmSetup();
  pwmL(0); pwmR(0);

  // Encoder pins + interrupts
  pinMode(ENC_L_A, INPUT_PULLUP);
  pinMode(ENC_L_B, INPUT_PULLUP);
  pinMode(ENC_R_A, INPUT_PULLUP);
  pinMode(ENC_R_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_L_A), isrLA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_L_B), isrLB, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_R_A), isrRA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_R_B), isrRB, CHANGE);

  lastCmdMs   = millis();
  lastPidMs   = millis();
  lastOdomMs  = millis();
  lastLatchMs = millis();

  LOG("\n=================================================\n");
  LOG("  Diff Drive v3.0 | ESP32-S3 | PID + Odometry\n");
  LOG("=================================================\n");
  LOG("TICKS_PER_REV=%d  MAX_RPM=%d\n", TICKS_PER_REV, MAX_RPM);
  LOG("WHEEL_R=%.4fm  TRACK=%.4fm\n", WHEEL_RADIUS_M, TRACK_WIDTH_M);
  LOG("PID Kp=%.3f Ki=%.3f Kd=%.3f\n", KP, KI, KD);
  LOG("Odom published every %dms as JSON on Serial\n", ODOM_PUBLISH_MS);
  LOG("-------------------------------------------------\n");
  LOG("Keys: W/S/A/D move  X stop  H reset  V verbose  ? status\n");
  LOG("Cmds: MOVE:<L>,<R>  PID:<Kp>,<Ki>,<Kd>  RESETODOM\n");
  LOG("Verbose OFF. Press V to enable PID debug output.\n\n");
}

// ================================================================
//  LOOP
// ================================================================

void loop() {
  processSerial();
  latchUpdate();
  pidUpdate();          // also calls odomUpdate()
  odomPublishLoop();    // sends JSON to RPi

#if WATCHDOG_ENABLE
  // Watchdog only when no latch held
  if (latchedCmd == '0' &&
      (pidL.setpoint != 0 || pidR.setpoint != 0) &&
      (millis() - lastCmdMs > WATCHDOG_MS)) {
    LOG("[WDG] timeout → STOP\n");
    cmdStop();
  }
#endif
}
