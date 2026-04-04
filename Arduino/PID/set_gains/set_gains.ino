/*
 * ================================================================
 *  PID SPEED CONTROL (2 MOTOR - TUNING MODE)
 * ================================================================
 *
 *  PURPOSE:
 *  - Tune PID gains (Kp, Ki, Kd)
 *  - Achieve stable speed control
 *
 *  CONTROL STRATEGY:
 *  - Uses average RPM of both motors
 *  - Same PWM applied to both motors
 *'
 *  SERIAL COMMANDS:
 *   RPM:100   → Set speed
 *   KP:1.2    → Set Kp
 *   KI:0.0    → Set Ki
 *   KD:0.2    → Set Kd
 *
 * ================================================================
 */

// ================== PIN CONFIG ==================
#define ENC_L_A 18
#define ENC_L_B 17
#define ENC_R_A 16
#define ENC_R_B 15

#define PWM_L 5
#define DIR_L 4
#define PWM_R 7
#define DIR_R 6

// ================== MOTOR CONFIG ==================
#define CPR 560             // Encoder counts per revolution
#define SAMPLE_MS 100       // Control loop interval

// ================== ENCODER ==================
volatile long ticksL = 0;
volatile long ticksR = 0;

long lastL = 0;
long lastR = 0;

// ================== PID VARIABLES ==================
float kP = 2.80;
float kI = 1.60;
float kD = 0.02;

float setRPM = 0;

float error = 0;
float prevError = 0;
float integral = 0;

// ================== TIMING ==================
unsigned long prevTime = 0;

// ================== ISR ==================
void IRAM_ATTR encL_ISR() {
  if (digitalRead(ENC_L_A) == digitalRead(ENC_L_B)) ticksL--;
  else ticksL++;
}

void IRAM_ATTR encR_ISR() {
  if (digitalRead(ENC_R_A) == digitalRead(ENC_R_B)) ticksR++;
  else ticksR--;
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  pinMode(ENC_L_A, INPUT_PULLUP);
  pinMode(ENC_L_B, INPUT_PULLUP);
  pinMode(ENC_R_A, INPUT_PULLUP);
  pinMode(ENC_R_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_L_A), encL_ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_R_A), encR_ISR, CHANGE);

  pinMode(DIR_L, OUTPUT);
  pinMode(DIR_R, OUTPUT);

  ledcAttach(PWM_L, 20000, 8);
  ledcAttach(PWM_R, 20000, 8);

  prevTime = millis();

  Serial.println("SetRPM\tActualRPM\tError\tPWM");
}

// ================== LOOP ==================
void loop() {

  // ---------- CONTROL LOOP ----------
  if (millis() - prevTime >= SAMPLE_MS) {

    float dt = SAMPLE_MS / 1000.0;
    prevTime = millis();

    // Calculate RPM
    long dL = ticksL - lastL;
    long dR = ticksR - lastR;

    lastL = ticksL;
    lastR = ticksR;

    float rpmL = (dL * 60.0) / (CPR * dt);
    float rpmR = (dR * 60.0) / (CPR * dt);

    float avgRPM = (rpmL + rpmR) / 2.0;

    // ---------- PID ----------
    error = setRPM - avgRPM;

    integral += error * dt;
    integral = constrain(integral, -100, 100);

    float derivative = (error - prevError) / dt;
    prevError = error;

    float output = kP * error + kI * integral + kD * derivative;

    int pwm = constrain(abs(output), 0, 255);

    // Apply to motors
    digitalWrite(DIR_L, output < 0);
    digitalWrite(DIR_R, output >= 0);

    ledcWrite(PWM_L, pwm);
    ledcWrite(PWM_R, pwm);

    // ---------- Debug ----------
  Serial.print("SET RPM: ");
Serial.print(setRPM);

Serial.print(" | ACT RPM: ");
Serial.print(avgRPM);

Serial.print(" | ERROR: ");
Serial.print(error);

Serial.print(" | PWM: ");
Serial.print(pwm);

Serial.print(" | KP: ");
Serial.print(kP);

Serial.print(" | KI : ");
Serial.print(kI);

Serial.print(" | KD: ");
Serial.println(kD);
  }

  // ---------- SERIAL CONTROL ----------
  if (Serial.available()) {

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    if (cmd.startsWith("RPM:")) setRPM = cmd.substring(4).toFloat();
    if (cmd.startsWith("KP:")) kP = cmd.substring(3).toFloat();
    if (cmd.startsWith("KI:")) kI = cmd.substring(3).toFloat();
    if (cmd.startsWith("KD:")) kD = cmd.substring(3).toFloat();
  }
}