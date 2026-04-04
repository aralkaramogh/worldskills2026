/*
 * ================================================================
 *  SERIAL TELEOP + PID + ACCELERATION RAMP (RPM CONTROL)
 * ================================================================

 🎮 CONTROLS (FINAL)
Movement
w → forward
s → backward
a → left
d → right
x → stop
Speed tuning
q / z → forward speed ±
e / c → turning speed ±

⚠️ FINAL TUNING ADVICE (VERY IMPORTANT)
If robot jerks → reduce MAX_RAMP_STEP
If slow response → increase Kp
If oscillation → reduce Ki
If noisy → reduce Kd
 */

// ── PINS ─────────────────────────────────────────────────────
#define DIR_L 4
#define PWM_L 5
#define DIR_R 6
#define PWM_R 7

#define ENC_L_A 18
#define ENC_L_B 17
#define ENC_R_A 16
#define ENC_R_B 15

// ── CONFIG ───────────────────────────────────────────────────
#define PPR 20
#define CONTROL_INTERVAL 100   // ms

// ── RAMP CONFIG ──────────────────────────────────────────────
#define RAMP_INTERVAL 20       // ms
#define MAX_RAMP_STEP 5.0      // RPM step per update

// ── SPEED LIMITS ─────────────────────────────────────────────
#define FWD_MIN   10
#define FWD_MAX   150
#define TURN_MIN  10
#define TURN_MAX  150

// ── PID GAINS ────────────────────────────────────────────────
float Kp = 2.80;
float Ki = 1.60;
float Kd = 0.02;

// ── STATE ────────────────────────────────────────────────────
volatile long ticksL = 0;
volatile long ticksR = 0;

float currentRPM_L = 0;
float currentRPM_R = 0;

// PID states
float errL=0, prevErrL=0, integralL=0;
float errR=0, prevErrR=0, integralR=0;

// ── TELEOP ───────────────────────────────────────────────────
int fwdSpeed  = 50;   // RPM
int turnSpeed = 40;

char latchedCmd = '0';

// ── RAMP VARIABLES ───────────────────────────────────────────
float cmdRPM_L = 0, cmdRPM_R = 0;
float rampRPM_L = 0, rampRPM_R = 0;

unsigned long lastRampMs = 0;
unsigned long lastControl = 0;

// ── PWM ──────────────────────────────────────────────────────
void pwmSetup() {
  ledcAttach(PWM_L, 20000, 8);
  ledcAttach(PWM_R, 20000, 8);
}

void setMotorRaw(int l, int r) {
  digitalWrite(DIR_L, l >= 0);
  digitalWrite(DIR_R, r >= 0);

  ledcWrite(PWM_L, constrain(abs(l), 0, 255));
  ledcWrite(PWM_R, constrain(abs(r), 0, 255));
}

// ── QUADRATURE ISR ───────────────────────────────────────────
void IRAM_ATTR isrL() {
  if (digitalRead(ENC_L_B)) ticksL++;
  else ticksL--;
}

void IRAM_ATTR isrR() {
  if (digitalRead(ENC_R_B)) ticksR--;
  else ticksR++;
}

// ── RPM CALC ─────────────────────────────────────────────────
void updateRPM() {
  static long prevL=0, prevR=0;

  long currL = ticksL;
  long currR = ticksR;

  long dL = currL - prevL;
  long dR = currR - prevR;

  prevL = currL;
  prevR = currR;

  currentRPM_L = (dL * 600.0) / PPR;
  currentRPM_R = (dR * 600.0) / PPR;
}

// ── PID ──────────────────────────────────────────────────────
int computePID(float target, float current, float &err, float &prevErr, float &integral) {
  err = target - current;

  integral += err;
  integral = constrain(integral, -200, 200); // anti-windup

  float derivative = err - prevErr;
  prevErr = err;

  float out = Kp*err + Ki*integral + Kd*derivative;

  return constrain((int)out, -255, 255);
}

// ── RAMP ─────────────────────────────────────────────────────
float applyRamp(float target, float current) {
  float diff = target - current;

  if (abs(diff) <= MAX_RAMP_STEP) return target;

  if (diff > 0) return current + MAX_RAMP_STEP;
  else          return current - MAX_RAMP_STEP;
}

// ── TELEOP COMMANDS ──────────────────────────────────────────
void executeCmd(char c) {
  switch(c) {
    case 'w':
      cmdRPM_L =  fwdSpeed;
      cmdRPM_R =  fwdSpeed;
      break;

    case 's':
      cmdRPM_L = -fwdSpeed;
      cmdRPM_R = -fwdSpeed;
      break;

    case 'a':
      cmdRPM_L = -turnSpeed;
      cmdRPM_R =  turnSpeed;
      break;

    case 'd':
      cmdRPM_L =  turnSpeed;
      cmdRPM_R = -turnSpeed;
      break;

    case 'x':
      cmdRPM_L = 0;
      cmdRPM_R = 0;
      break;
  }
}

// ── SERIAL CONTROL ───────────────────────────────────────────
void handleSerial() {
  if (!Serial.available()) return;

  char c = Serial.read();

  // Motion
  if (c=='w'||c=='s'||c=='a'||c=='d') {
    latchedCmd = c;
    executeCmd(c);
  }

  if (c=='x') {
    latchedCmd = '0';
    executeCmd('x');
  }

  // Forward speed
  if (c=='q') {
    fwdSpeed = constrain(fwdSpeed + 5, FWD_MIN, FWD_MAX);
    Serial.printf("[FWD ↑] %d RPM\n", fwdSpeed);
  }

  if (c=='z') {
    fwdSpeed = constrain(fwdSpeed - 5, FWD_MIN, FWD_MAX);
    Serial.printf("[FWD ↓] %d RPM\n", fwdSpeed);
  }

  // Turn speed
  if (c=='e') {
    turnSpeed = constrain(turnSpeed + 5, TURN_MIN, TURN_MAX);
    Serial.printf("[TURN ↑] %d RPM\n", turnSpeed);
  }

  if (c=='c') {
    turnSpeed = constrain(turnSpeed - 5, TURN_MIN, TURN_MAX);
    Serial.printf("[TURN ↓] %d RPM\n", turnSpeed);
  }

  // Reapply latched command
  if (latchedCmd != '0') {
    executeCmd(latchedCmd);
  }
}

// ── SETUP ────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(DIR_L, OUTPUT);
  pinMode(DIR_R, OUTPUT);

  pinMode(ENC_L_A, INPUT_PULLUP);
  pinMode(ENC_L_B, INPUT_PULLUP);
  pinMode(ENC_R_A, INPUT_PULLUP);
  pinMode(ENC_R_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_L_A), isrL, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_R_A), isrR, RISING);

  pwmSetup();

  Serial.println("PID + Ramp Teleop Ready");
}

// ── LOOP ─────────────────────────────────────────────────────
void loop() {
  handleSerial();

  // ── RAMP UPDATE ───────────────────────────────────────────
  if (millis() - lastRampMs >= RAMP_INTERVAL) {
    lastRampMs = millis();

    rampRPM_L = applyRamp(cmdRPM_L, rampRPM_L);
    rampRPM_R = applyRamp(cmdRPM_R, rampRPM_R);
  }

  // ── PID CONTROL ───────────────────────────────────────────
  #define KP_STEP 0.1
#define KI_STEP 0.05
#define KD_STEP 0.01

  if (millis() - lastControl >= CONTROL_INTERVAL) {
    lastControl = millis();

    updateRPM();

    int pwmL = computePID(rampRPM_L, currentRPM_L, errL, prevErrL, integralL);
    int pwmR = computePID(rampRPM_R, currentRPM_R, errR, prevErrR, integralR);

    setMotorRaw(pwmL, pwmR);

    

//    Serial.printf("CMD: %.1f %.1f | RAMP: %.1f %.1f | ACT: %.1f %.1f\n",
    Serial.printf("CMD: %.1f %.1f");

      cmdRPM_L, cmdRPM_R,
      rampRPM_L, rampRPM_R,
      currentRPM_L, currentRPM_R);
  }


      void printPID() {
  Serial.printf("PID => Kp: %.2f | Ki: %.2f | Kd: %.2f\n", Kp, Ki, Kd);
}
  
