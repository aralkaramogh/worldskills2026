/*
 * ================================================================
 *  RPM TELEOP + MONITORING (ESP32-S3)
 * ================================================================
 *  - Control input in RPM
 *  - Reads encoder pulses
 *  - Prints real-time RPM
 *  - Use this to observe base motor behavior before PID
 *
 *  COMMANDS:
 *    w/s → increase/decrease RPM
 *    x   → stop
 * ================================================================
 */

// ── MOTOR PINS ───────────────────────────────────────────────
#define DIR_L 4
#define PWM_L 5
#define DIR_R 6
#define PWM_R 7

// ── ENCODER PINS (CHANGE AS PER YOUR WIRING) ─────────────────
#define ENC_L 18
#define ENC_R 17

// ── ENCODER CONFIG ───────────────────────────────────────────
#define PPR 20   // pulses per revolution (adjust for your encoder)

// ── PWM ──────────────────────────────────────────────────────
#define PWM_FREQ 20000
#define PWM_BITS 8

// ── GLOBALS ──────────────────────────────────────────────────
volatile long countL = 0;
volatile long countR = 0;

int targetRPM = 50;   // user controlled
int pwmOut = 120;     // open loop (no PID)

// ── INTERRUPTS ───────────────────────────────────────────────
void IRAM_ATTR encL_ISR() { countL++; }
void IRAM_ATTR encR_ISR() { countR++; }

// ── PWM SETUP ────────────────────────────────────────────────
void setupPWM() {
  ledcAttach(PWM_L, PWM_FREQ, PWM_BITS);
  ledcAttach(PWM_R, PWM_FREQ, PWM_BITS);
}

// ── MOTOR DRIVE ──────────────────────────────────────────────
void setMotors(int pwm) {
  digitalWrite(DIR_L, LOW);
  digitalWrite(DIR_R, HIGH);

  ledcWrite(PWM_L, pwm);
  ledcWrite(PWM_R, pwm);
}

void stopMotors() {
  ledcWrite(PWM_L, 0);
  ledcWrite(PWM_R, 0);
}

// ── RPM CALCULATION ──────────────────────────────────────────
unsigned long lastTime = 0;

void computeRPM() {
  if (millis() - lastTime >= 500) {  // every 0.5 sec
    long cL = countL;
    long cR = countR;

    countL = 0;
    countR = 0;

    float rpmL = (cL * 120.0) / PPR; // 60s / 0.5s = 120
    float rpmR = (cR * 120.0) / PPR;

    Serial.printf("TargetRPM:%d  |  L:%.2f  R:%.2f\n", targetRPM, rpmL, rpmR);

    lastTime = millis();
  }
}

// ── SERIAL CONTROL ───────────────────────────────────────────
void handleSerial() {
  if (!Serial.available()) return;

  char c = Serial.read();

  switch (c) {
    case 'w': targetRPM += 10; break;
    case 's': targetRPM -= 10; break;
    case 'x': stopMotors(); break;
  }

  targetRPM = constrain(targetRPM, 0, 200);

  // crude mapping RPM → PWM (temporary, for testing only)
  pwmOut = map(targetRPM, 0, 200, 0, 255);

  setMotors(pwmOut);
}

// ── SETUP ────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(DIR_L, OUTPUT);
  pinMode(DIR_R, OUTPUT);

  pinMode(ENC_L, INPUT_PULLUP);
  pinMode(ENC_R, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_L), encL_ISR, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_R), encR_ISR, RISING);

  setupPWM();

  Serial.println("RPM Teleop Ready");
}

// ── LOOP ─────────────────────────────────────────────────────
void loop() {
  handleSerial();
  computeRPM();
}