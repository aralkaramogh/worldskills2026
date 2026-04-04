/*
 * ================================================================
 * Diff Drive Teleop + PID Closed Loop | ESP32-S3 | Cytron DD10A
 * ================================================================
 */

// ── Pins ─────────────────────────────────────────────────────
#define DIR_L    4
#define PWM_L    5
#define DIR_R    6
#define PWM_R    7
#define LED_PIN  2

#define ENC_L_A  18
#define ENC_L_B  17
#define ENC_R_A  16
#define ENC_R_B  15

// ── LEDC ─────────────────────────────────────────────────────
#define LEDC_FREQ  20000
#define LEDC_BITS  8

// ── Motor & Hardware Config ──────────────────────────────────
#define INVERT_LEFT  true
#define INVERT_RIGHT false
#define CPR 560             // Encoder counts per revolution
#define SAMPLE_MS 100       // Control loop interval (100ms)

// ── Watchdog ─────────────────────────────────────────────────
#define WATCHDOG_ENABLE 1
#define WATCHDOG_MS     500

// ── Target Speeds (Now in RPM, not PWM %) ────────────────────
float targetFwdRPM  = 60.0; // Base forward speed in RPM
float targetTurnRPM = 40.0; // Base turning speed in RPM

const float MAX_RPM = 150.0; // Cap to prevent asking for impossible speeds

// ── Encoders ─────────────────────────────────────────────────
volatile long ticksL = 0;
volatile long ticksR = 0;
long lastL = 0;
long lastR = 0;

// ── PID Variables ────────────────────────────────────────────
float kP = 2.0;
float kI = 1.20;
float kD = 0.0;

float targetRpmL = 0;
float targetRpmR = 0;

float prevErrorL = 0, integralL = 0;
float prevErrorR = 0, integralR = 0;

// ── State & Timing ───────────────────────────────────────────
bool running = false;
unsigned long lastCmdMs = 0;
unsigned long lastLatchMs = 0;
unsigned long prevPidTime = 0;
char latchedCmd = '0';
bool verbose = false;

#define LATCH_REPEAT_MS  100
#define LOG(...)  Serial.printf(__VA_ARGS__)
#define LOGV(...) if (verbose) Serial.printf(__VA_ARGS__)

// ================================================================
// INTERRUPT SERVICE ROUTINES (ISRs)
// ================================================================
void IRAM_ATTR encL_ISR() {
  if (digitalRead(ENC_L_A) == digitalRead(ENC_L_B)) ticksL--;
  else ticksL++;
}

void IRAM_ATTR encR_ISR() {
  if (digitalRead(ENC_R_A) == digitalRead(ENC_R_B)) ticksR++;
  else ticksR--;
}

// ================================================================
// MOTOR ACTUATION (Accepts PID output: -255 to 255)
// ================================================================
void pwmSetup() {
  ledcAttach(PWM_L, LEDC_FREQ, LEDC_BITS);
  ledcAttach(PWM_R, LEDC_FREQ, LEDC_BITS);
}

void setLeftMotorHardware(int pidOutput) {
  pidOutput = constrain(pidOutput, -255, 255);
  bool fwd = (pidOutput >= 0);
  if (INVERT_LEFT) fwd = !fwd;
  digitalWrite(DIR_L, fwd ? HIGH : LOW);
  ledcWrite(PWM_L, abs(pidOutput));
}

void setRightMotorHardware(int pidOutput) {
  pidOutput = constrain(pidOutput, -255, 255);
  bool fwd = (pidOutput >= 0);
  if (INVERT_RIGHT) fwd = !fwd;
  digitalWrite(DIR_R, fwd ? HIGH : LOW);
  ledcWrite(PWM_R, abs(pidOutput));
}

// ================================================================
// PID COMPUTATION FUNCTION
// ================================================================
int computePID(float target, float current, float &prevError, float &integral, float dt) {
  float error = target - current;
  
  integral += error * dt;
  integral = constrain(integral, -100, 100); // Anti-windup
  
  float derivative = (error - prevError) / dt;
  prevError = error;
  
  float output = (kP * error) + (kI * integral) + (kD * derivative);
  return (int)output; 
}

// ================================================================
// MOTION COMMANDS (Setting Target RPMs)
// ================================================================
void setTargets(float l, float r) {
  targetRpmL = l;
  targetRpmR = r;
  running = (l != 0 || r != 0);
  digitalWrite(LED_PIN, running ? HIGH : LOW);
}

void cmdForward() {
  setTargets(targetFwdRPM, targetFwdRPM);
  LOGV("[FWD RPM: %.1f]\n", targetFwdRPM);
}

void cmdBackward() {
  setTargets(-targetFwdRPM, -targetFwdRPM);
  LOGV("[BWD RPM: %.1f]\n", -targetFwdRPM);
}

void cmdTurnLeft() {
  setTargets(-targetTurnRPM, targetTurnRPM);
  LOGV("[LEFT RPM]\n");
}

void cmdTurnRight() {
  setTargets(targetTurnRPM, -targetTurnRPM);
  LOGV("[RIGHT RPM]\n");
}

void cmdStop() {
  setTargets(0, 0);
  latchedCmd = '0';
  
  // Reset PID history so it doesn't jump when starting again
  integralL = 0; prevErrorL = 0;
  integralR = 0; prevErrorR = 0;
  
  // Instantly stop hardware
  setLeftMotorHardware(0);
  setRightMotorHardware(0);
  LOGV("[STOP]\n");
}

void cmdReset() {
  cmdStop();
  targetFwdRPM  = 60.0;
  targetTurnRPM = 40.0;
  LOG("[RESET]\n");
}

void printStatus() {
  LOG("{ fwdRPM:%.1f turnRPM:%.1f running:%s latch:%c verbose:%s }\n",
      targetFwdRPM, targetTurnRPM,
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

    // Adjust RPM limits instead of PWM limits
    case 'q': targetFwdRPM = min(targetFwdRPM + 5, MAX_RPM); break;
    case 'z': targetFwdRPM = max(targetFwdRPM - 5, 5.0f); break;
    case 'e': targetTurnRPM = min(targetTurnRPM + 5, MAX_RPM); break;
    case 'c': targetTurnRPM = max(targetTurnRPM - 5, 5.0f); break;

    case '?': printStatus(); break;
  }
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

  // Setup Encoders
  pinMode(ENC_L_A, INPUT_PULLUP);
  pinMode(ENC_L_B, INPUT_PULLUP);
  pinMode(ENC_R_A, INPUT_PULLUP);
  pinMode(ENC_R_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_L_A), encL_ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_R_A), encR_ISR, CHANGE);

  pwmSetup();
  prevPidTime = millis();

  LOG("Diff Drive + PID Ready. Target Fwd RPM: %.1f\n", targetFwdRPM);
}

// ================================================================
// LOOP
// ================================================================
void loop() {
  // 1. Check Serial
  while (Serial.available()) {
    handleChar(Serial.read());
  }

  // 2. Latch Update
  if (latchedCmd != '0' && (millis() - lastLatchMs >= LATCH_REPEAT_MS)) {
    lastLatchMs = millis();
    executeMotionChar(latchedCmd);
  }

  // 3. Watchdog
#if WATCHDOG_ENABLE
  if (latchedCmd == '0' && running && (millis() - lastCmdMs > WATCHDOG_MS)) {
    cmdStop();
  }
#endif

  // 4. PID Control Loop (Runs every 100ms)
  unsigned long currentTime = millis();
  if (currentTime - prevPidTime >= SAMPLE_MS) {
    float dt = SAMPLE_MS / 1000.0;
    prevPidTime = currentTime;

    // Calculate actual RPM
    long dL = ticksL - lastL;
    long dR = ticksR - lastR;
    lastL = ticksL;
    lastR = ticksR;

    float currentRpmL = (dL * 60.0) / (CPR * dt);
    float currentRpmR = (dR * 60.0) / (CPR * dt);

    // Compute PID (only if running to prevent jitter at standstill)
    if (running) {
      int outputL = computePID(targetRpmL, currentRpmL, prevErrorL, integralL, dt);
      int outputR = computePID(targetRpmR, currentRpmR, prevErrorR, integralR, dt);

      setLeftMotorHardware(outputL);
      setRightMotorHardware(outputR);

      if (verbose) {
         LOG("TL:%.1f TR:%.1f | AL:%.1f AR:%.1f | PL:%d PR:%d\n", 
             targetRpmL, targetRpmR, currentRpmL, currentRpmR, outputL, outputR);
      }
    }
  }
}