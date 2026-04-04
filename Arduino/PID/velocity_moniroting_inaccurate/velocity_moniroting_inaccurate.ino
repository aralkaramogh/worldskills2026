/*
 * ================================================================
 *  VELOCITY TELEOP + MONITORING (ESP32-S3)
 * ================================================================
 *  - Control in m/s
 *  - Converts encoder RPM → velocity
 *  - Straight motion testing for PID tuning
 * ================================================================
 */

// ── MOTOR PINS ───────────────────────────────────────────────
#define DIR_L 4
#define PWM_L 5
#define DIR_R 6
#define PWM_R 7

// ── ENCODER ──────────────────────────────────────────────────
#define ENC_L 18
#define ENC_R 17
#define PPR 20

// ── WHEEL PARAMETERS ─────────────────────────────────────────
#define WHEEL_RADIUS 0.4575   // meters (CHANGE THIS)

// ── PWM ──────────────────────────────────────────────────────
#define PWM_FREQ 20000
#define PWM_BITS 8

volatile long countL = 0;
volatile long countR = 0;

float targetVel = 0.5; // m/s
int pwmOut = 120;

// ── ISR ──────────────────────────────────────────────────────
void IRAM_ATTR encL_ISR() { countL++; }
void IRAM_ATTR encR_ISR() { countR++; }

// ── PWM SETUP ────────────────────────────────────────────────
void setupPWM() {
  ledcAttach(PWM_L, PWM_FREQ, PWM_BITS);
  ledcAttach(PWM_R, PWM_FREQ, PWM_BITS);
}

// ── MOTOR ────────────────────────────────────────────────────
void setMotors(int pwm) {
  digitalWrite(DIR_L, LOW);
  digitalWrite(DIR_R, HIGH);

  ledcWrite(PWM_L, pwm);
  ledcWrite(PWM_R, pwm);
}

// ── VELOCITY CALCULATION ─────────────────────────────────────
unsigned long lastTime = 0;

void computeVelocity() {
  if (millis() - lastTime >= 500) {

    long cL = countL;
    long cR = countR;

    countL = 0;
    countR = 0;

    float rpmL = (cL * 120.0) / PPR;
    float rpmR = (cR * 120.0) / PPR;

    float velL = (2 * PI * WHEEL_RADIUS * rpmL) / 60.0;
    float velR = (2 * PI * WHEEL_RADIUS * rpmR) / 60.0;

    Serial.printf("TargetVel:%.2f m/s | L:%.2f  R:%.2f\n", targetVel, velL, velR);

    lastTime = millis();
  }
}

// ── SERIAL CONTROL ───────────────────────────────────────────
void handleSerial() {
  if (!Serial.available()) return;

  char c = Serial.read();

  switch (c) {
    case 'w': targetVel += 0.1; break;
    case 's': targetVel -= 0.1; break;
    case 'x': pwmOut = 0; break;
  }

  targetVel = constrain(targetVel, 0.0, 2.0);

  // rough mapping velocity → PWM (temporary)
  pwmOut = map(targetVel * 100, 0, 200, 0, 255);

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

  Serial.println("Velocity Teleop Ready");
}

// ── LOOP ─────────────────────────────────────────────────────
void loop() {
  handleSerial();
  computeVelocity();
}