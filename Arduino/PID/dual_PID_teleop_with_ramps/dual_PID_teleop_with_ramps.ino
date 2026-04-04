/*
 * =======================================================================================
 * SERIAL TEST FIRMWARE: Differential Drive AMR | ESP32-S3 | Cytron MDD10A
 * =======================================================================================
 * PURPOSE: Test Asymmetrical Slew Rate Limiter (Ramp) and PID logic via Serial Monitor 
 * before deploying micro-ROS.
 * * CONTROLS (Send via Serial Monitor at 115200 baud):
 * W = Forward    A = Left    S = Backward    D = Right    X = Stop
 * V = Toggle Verbose Logging (to watch the Ramp and PID in action)
 * =======================================================================================
 */

#include <Arduino.h>

// ================== PIN DEFINITIONS ==================
#define DIR_L    4
#define PWM_L    5
#define DIR_R    6
#define PWM_R    7
#define LED_PIN  2  // Onboard LED

#define ENC_L_A  18
#define ENC_L_B  17
#define ENC_R_A  16
#define ENC_R_B  15

// ================== HARDWARE CONFIG ==================
#define LEDC_FREQ  20000
#define LEDC_BITS  8
#define INVERT_LEFT  true   // Adjust based on your motor wiring
#define INVERT_RIGHT false  // Adjust based on your motor wiring
const float CPR = 560.0;    // Encoder counts per revolution

// ================== CONTROL LOOP ==================
const int SAMPLE_MS = 50;   // 50ms = 20Hz control loop
unsigned long prevPidTime = 0;
bool verbose = false;       // Toggle with 'V' in serial

// ================== TELEOP SPEEDS ==================
float teleopFwdRpm = 60.0;
float teleopTurnRpm = 40.0;

// ================== RAMP (SLEW RATE) LIMITS ==================
float accelRpmPerSec = 60.0;  // Gentle acceleration
float decelRpmPerSec = 200.0; // Sharp, fast braking

// RPM Variables
float desiredRpmL = 0.0; // The goal RPM set by keyboard commands
float desiredRpmR = 0.0;
float targetRpmL = 0.0;  // The current allowed RPM (stepped by the ramp)
float targetRpmR = 0.0;

// ================== PID VARIABLES ==================
float kP = 2.0;
float kI = 1.20;
float kD = 0.00;

float prevErrorL = 0, integralL = 0;
float prevErrorR = 0, integralR = 0;

// ================== ENCODER VARIABLES ==================
volatile long ticksL = 0;
volatile long ticksR = 0;
long lastL = 0;
long lastR = 0;

// ================================================================
// 1. INTERRUPT SERVICE ROUTINES
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
// 2. HARDWARE ACTUATION
// ================================================================
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
// 3. MATH & CONTROL FUNCTIONS
// ================================================================
float computeRamp(float currentTarget, float desiredTarget, float accelLimit, float decelLimit, float dt) {
  if (currentTarget == desiredTarget) return currentTarget;

  float maxStep;
  if ((currentTarget >= 0 && desiredTarget > currentTarget) || 
      (currentTarget <= 0 && desiredTarget < currentTarget)) {
    maxStep = accelLimit * dt; // Accelerating
  } else {
    maxStep = decelLimit * dt; // Braking / Reversing
  }

  if (desiredTarget > currentTarget) {
    currentTarget += maxStep;
    if (currentTarget > desiredTarget) currentTarget = desiredTarget;
  } else {
    currentTarget -= maxStep;
    if (currentTarget < desiredTarget) currentTarget = desiredTarget;
  }
  return currentTarget;
}

int computePID(float target, float current, float &prevError, float &integral, float dt) {
  float error = target - current;
  integral += error * dt;
  integral = constrain(integral, -100, 100); // Anti-windup
  float derivative = (error - prevError) / dt;
  prevError = error;
  float output = (kP * error) + (kI * integral) + (kD * derivative);
  return (int)constrain(output, -255, 255); 
}

// ================================================================
// 4. SERIAL COMMAND HANDLER
// ================================================================
void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    
    switch (c) {
      case 'w': case 'W': 
        desiredRpmL = teleopFwdRpm; desiredRpmR = teleopFwdRpm; 
        Serial.println("CMD: FORWARD"); break;
      case 's': case 'S': 
        desiredRpmL = -teleopFwdRpm; desiredRpmR = -teleopFwdRpm; 
        Serial.println("CMD: BACKWARD"); break;
      case 'a': case 'A': 
        desiredRpmL = -teleopTurnRpm; desiredRpmR = teleopTurnRpm; 
        Serial.println("CMD: LEFT"); break;
      case 'd': case 'D': 
        desiredRpmL = teleopTurnRpm; desiredRpmR = -teleopTurnRpm; 
        Serial.println("CMD: RIGHT"); break;
      case 'x': case 'X': 
        desiredRpmL = 0.0; desiredRpmR = 0.0; 
        Serial.println("CMD: STOP (Ramping down)"); break;
      case 'v': case 'V':
        verbose = !verbose;
        Serial.print("VERBOSE LOGGING: "); Serial.println(verbose ? "ON" : "OFF");
        break;
    }
  }
}

// ================================================================
// 5. SETUP
// ================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("--- AMR SERIAL TEST FIRMWARE ---");
  Serial.println("Send W, A, S, D to move. Send X to stop. Send V to toggle telemetry.");

  pinMode(DIR_L, OUTPUT);
  pinMode(DIR_R, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  pinMode(ENC_L_A, INPUT_PULLUP);
  pinMode(ENC_L_B, INPUT_PULLUP);
  pinMode(ENC_R_A, INPUT_PULLUP);
  pinMode(ENC_R_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_L_A), encL_ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_R_A), encR_ISR, CHANGE);
  
  ledcAttach(PWM_L, LEDC_FREQ, LEDC_BITS);
  ledcAttach(PWM_R, LEDC_FREQ, LEDC_BITS);

  prevPidTime = millis();
}

// ================================================================
// 6. MAIN LOOP
// ================================================================
void loop() {
  // 1. Process Keyboard Commands
  handleSerial();
  
  // 2. Control Loop
  unsigned long currentTime = millis();
  if (currentTime - prevPidTime >= SAMPLE_MS) {
    float dt = SAMPLE_MS / 1000.0;
    prevPidTime = currentTime;

    // --- APPLY RAMP ---
    targetRpmL = computeRamp(targetRpmL, desiredRpmL, accelRpmPerSec, decelRpmPerSec, dt);
    targetRpmR = computeRamp(targetRpmR, desiredRpmR, accelRpmPerSec, decelRpmPerSec, dt);

    // --- FULL STOP EFFICIENCY CHECK ---
    if (desiredRpmL == 0 && desiredRpmR == 0 && targetRpmL == 0 && targetRpmR == 0) {
      setLeftMotorHardware(0);
      setRightMotorHardware(0);
      integralL = 0; prevErrorL = 0;
      integralR = 0; prevErrorR = 0;
      digitalWrite(LED_PIN, LOW);
    } 
    // --- COMPUTE & APPLY PID ---
    else {
      digitalWrite(LED_PIN, HIGH);
      
      long dL = ticksL - lastL;
      long dR = ticksR - lastR;
      lastL = ticksL;
      lastR = ticksR;

      float currentRpmL = (dL * 60.0) / (CPR * dt);
      float currentRpmR = (dR * 60.0) / (CPR * dt);

      int outputL = computePID(targetRpmL, currentRpmL, prevErrorL, integralL, dt);
      int outputR = computePID(targetRpmR, currentRpmR, prevErrorR, integralR, dt);

      setLeftMotorHardware(outputL);
      setRightMotorHardware(outputR);

      // Print telemetry if Verbose mode is on
      if (verbose) {
        Serial.printf("DesL:%6.1f | TargL:%6.1f | ActL:%6.1f || DesR:%6.1f | TargR:%6.1f | ActR:%6.1f\n", 
            desiredRpmL, targetRpmL, currentRpmL, desiredRpmR, targetRpmR, currentRpmR);
      }
    }
  }
}