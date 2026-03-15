/**
 * ================================================================
 *  PID Motor Tuner - GUI Compatible Firmware
 *  ESP32-S3 + Cytron DD10A + HD Hex Encoder (20:1 gearbox)
 *  Compatible with: pid_tuner_gui.py
 * ================================================================
 *
 *  This firmware is designed to work with the Python GUI tuner.
 *  GUI sends commands, firmware responds with CSV data.
 *
 *  MOTOR PINS:
 *   Left  Motor : DIR=4   PWM=5
 *   Right Motor : DIR=6   PWM=7
 *
 *  ENCODER PINS:
 *   Left  Encoder : A=15  B=16
 *   Right Encoder : A=17  B=18
 *
 *  SERIAL OUTPUT (CSV Format for GUI):
 *   Time(s), Setpoint(RPM), Actual(RPM), Error(RPM), PWM, kP, kI, kD
 *   0.10, 300.0, 145.5, 154.5, 128.3, 0.030, 0.001, 0.008
 *
 *  COMMANDS FROM GUI (via Serial):
 *   SQUARE          - Square wave test (0 → 300 → 0 RPM)
 *   SINE            - Sine wave test
 *   RAMP            - Ramp test
 *   STOP            - Stop immediately
 *   kP:0.035        - Set P gain
 *   kI:0.002        - Set I gain
 *   kD:0.008        - Set D gain
 *   RESET           - Reset everything
 *   STATUS          - Print status info
 *
 * ================================================================
 */

#include <Arduino.h>
#include <math.h>

// ================================================================
//  HARDWARE CONFIG
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
#define ENC_R_A  17
#define ENC_R_B  18

// Motor inversion
#define INVERT_LEFT   true
#define INVERT_RIGHT  false

// ================================================================
//  PHYSICAL PARAMETERS (20:1 Gearbox)
// ================================================================

#define WHEEL_RADIUS_M    0.05f      // 50mm wheel
#define TRACK_WIDTH_M     0.25f      // 250mm wheelbase

// Encoder: HD Hex (28 CPR) × 4 quadrature × 20 gear ratio = 2240 ticks/rev
#define ENC_CPR           28
#define GEAR_RATIO        20
#define TICKS_PER_REV     (ENC_CPR * 4 * GEAR_RATIO)   // 2240

#define MAX_RPM           300        // 6000 / 20
#define DEFAULT_FWD_RPM   60
#define DEFAULT_TURN_RPM  40

// ================================================================
//  TIMING & CONTROL
// ================================================================

#define PID_INTERVAL_MS    50        // 20 Hz PID loop
#define CSV_PUBLISH_MS     50        // CSV output every 50ms (20 Hz)
#define WATCHDOG_ENABLE    1
#define WATCHDOG_MS        500

// ================================================================
//  LEDC PWM (v2 + v3 compatible)
// ================================================================

#define LEDC_FREQ   20000
#define LEDC_BITS   8
#define CH_L        0
#define CH_R        1

// ================================================================
//  PID GAINS (User Adjustable)
// ================================================================

float KP = 0.02f;    // Proportional
float KI = 0.00f;    // Integral
float KD = 0.00f;    // Derivative
#define INTEGRAL_LIMIT  100.0f

// ================================================================
//  ENCODER DATA (Volatile for ISR)
// ================================================================

volatile long encTicksL = 0;
volatile long encTicksR = 0;

void IRAM_ATTR isrLA() { 
  encTicksL += (digitalRead(ENC_L_A) == digitalRead(ENC_L_B)) ? -1 : 1; 
}
void IRAM_ATTR isrLB() { 
  encTicksL += (digitalRead(ENC_L_A) != digitalRead(ENC_L_B)) ? -1 : 1; 
}
void IRAM_ATTR isrRA() { 
  encTicksR += (digitalRead(ENC_R_A) == digitalRead(ENC_R_B)) ? -1 : 1; 
}
void IRAM_ATTR isrRB() { 
  encTicksR += (digitalRead(ENC_R_A) != digitalRead(ENC_R_B)) ? -1 : 1; 
}

// ================================================================
//  PID CONTROLLER STRUCT
// ================================================================

struct MotorPID {
  float setpoint  = 0.0f;
  float integral  = 0.0f;
  float prevError = 0.0f;
  int   dutyOut   = 0;

  void reset() {
    setpoint = 0; integral = 0; prevError = 0; dutyOut = 0;
  }

  int compute(float actualRpm, float dt) {
    if (setpoint == 0.0f) {
      integral = 0; prevError = 0;
      return 0;
    }
    
    float error = setpoint - actualRpm;
    integral += error * dt;
    integral = constrain(integral, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);

    float deriv = (error - prevError) / dt;
    prevError = error;

    float out = KP * error + KI * integral + KD * deriv;
    int duty = (int)(fabs(out) / (float)MAX_RPM * 255.0f);
    duty = constrain(duty, 0, 255);

    dutyOut = duty;
    return duty;
  }
};

MotorPID pidL, pidR;

// ================================================================
//  STATE VARIABLES
// ================================================================

unsigned long startTimeMs = 0;
unsigned long lastPidMs = 0;
unsigned long lastCsvMs = 0;
unsigned long lastCmdMs = 0;

float actualRpmL = 0.0f;
float actualRpmR = 0.0f;
long prevPidTicksL = 0, prevPidTicksR = 0;

// Test waveform
String testWaveform = "NONE";
float waveSetpoint = 0.0f;
#define WAVE_FREQ_HZ   0.2f      // 0.2 Hz = 5 second period
#define WAVE_AMP_RPM   150.0f    // ±150 RPM
#define WAVE_CENTER    150.0f    // Center at 150 RPM

// Serial input
String cmdBuffer = "";

// ================================================================
//  PWM SETUP (v2/v3 compatible)
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
  ledcWrite(CH_L, constrain(v, 0, 255));
#endif
}

void pwmR(int v) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PWM_R, constrain(v, 0, 255));
#else
  ledcWrite(CH_R, constrain(v, 0, 255));
#endif
}

// ================================================================
//  MOTOR CONTROL
// ================================================================

void driveLeft(int duty, float setpointRpm) {
  bool fwd = (setpointRpm >= 0);
  if (INVERT_LEFT) fwd = !fwd;
  digitalWrite(DIR_L, fwd ? HIGH : LOW);
  pwmL(duty);
}

void driveRight(int duty, float setpointRpm) {
  bool fwd = (setpointRpm >= 0);
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
//  TEST WAVEFORMS
// ================================================================

float getWaveSetpoint(unsigned long elapsedMs) {
  float elapsed = elapsedMs / 1000.0f;  // seconds
  
  if (testWaveform == "SQUARE") {
    // 0.2 Hz = 5 second period = 2.5 sec per half
    float phase = fmod(elapsed * WAVE_FREQ_HZ, 1.0f);
    return (phase < 0.5f) ? (WAVE_CENTER + WAVE_AMP_RPM) : (WAVE_CENTER - WAVE_AMP_RPM);
  }
  
  else if (testWaveform == "SINE") {
    // Smooth sine wave 0-300 RPM
    float sine = sin(2.0f * M_PI * elapsed * WAVE_FREQ_HZ);
    return WAVE_CENTER + WAVE_AMP_RPM * sine;
  }
  
  else if (testWaveform == "RAMP") {
    // Ramp from 0 to 300 RPM in 5 seconds, hold
    float phase = elapsed * WAVE_FREQ_HZ;
    if (phase < 0.5f) {
      return 300.0f * (phase * 2.0f);  // 0 to 300 in first 2.5 sec
    } else {
      return 300.0f;  // Hold at 300
    }
  }
  
  return 0.0f;
}

// ================================================================
//  COMMAND PARSING
// ================================================================

void handleCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  
  if (cmd == "STOP") {
    hardStop();
    testWaveform = "NONE";
  }
  
  else if (cmd == "SQUARE") {
    testWaveform = "SQUARE";
    startTimeMs = millis();
  }
  
  else if (cmd == "SINE") {
    testWaveform = "SINE";
    startTimeMs = millis();
  }
  
  else if (cmd == "RAMP") {
    testWaveform = "RAMP";
    startTimeMs = millis();
  }
  
  else if (cmd == "RESET") {
    hardStop();
    testWaveform = "NONE";
    KP = 0.02f;
    KI = 0.00f;
    KD = 0.00f;
  }
  
  else if (cmd == "STATUS") {
    Serial.printf("[STATUS] kP=%.4f kI=%.4f kD=%.4f\n", KP, KI, KD);
  }
  
  else if (cmd.startsWith("KP:")) {
    KP = cmd.substring(3).toFloat();
  }
  
  else if (cmd.startsWith("KI:")) {
    KI = cmd.substring(3).toFloat();
  }
  
  else if (cmd.startsWith("KD:")) {
    KD = cmd.substring(3).toFloat();
  }
}

// ================================================================
//  SERIAL COMMUNICATION
// ================================================================

void processSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (cmdBuffer.length() > 0) {
        handleCommand(cmdBuffer);
        cmdBuffer = "";
      }
    } else {
      if (cmdBuffer.length() < 64) {
        cmdBuffer += c;
      }
    }
  }
}

void publishCsv(unsigned long elapsedMs, float setpoint, float actualL, float error, int duty) {
  float time_s = elapsedMs / 1000.0f;
  
  // CSV format: Time, Setpoint, Actual, Error, PWM, kP, kI, kD
  Serial.printf(
    "%.2f,%.1f,%.1f,%.1f,%d,%.4f,%.4f,%.4f\n",
    time_s,
    setpoint,
    actualL,
    error,
    (int)((duty / 255.0f) * 100.0f),  // PWM as %
    KP, KI, KD
  );
}

// ================================================================
//  PID & WAVEFORM UPDATE
// ================================================================

void pidUpdate() {
  unsigned long now = millis();
  if (now - lastPidMs < PID_INTERVAL_MS) return;
  
  float dt = (now - lastPidMs) / 1000.0f;
  lastPidMs = now;

  // Get current RPM from encoder
  noInterrupts();
  long curL = encTicksL;
  long curR = encTicksR;
  interrupts();

  long dL = curL - prevPidTicksL;
  long dR = curR - prevPidTicksR;
  prevPidTicksL = curL;
  prevPidTicksR = curR;

  // Convert ticks to RPM
  actualRpmL = (dL / (float)TICKS_PER_REV) / dt * 60.0f;
  actualRpmR = (dR / (float)TICKS_PER_REV) / dt * 60.0f;

  // Get test waveform setpoint (only left motor for single motor test)
  if (testWaveform != "NONE") {
    unsigned long elapsed = millis() - startTimeMs;
    waveSetpoint = getWaveSetpoint(elapsed);
    setTargetRpm(waveSetpoint, waveSetpoint);
  }

  // PID compute
  int dutyL = pidL.compute(actualRpmL, dt);
  int dutyR = pidR.compute(actualRpmR, dt);

  // Drive motors
  if (pidL.setpoint == 0 && pidR.setpoint == 0) {
    pwmL(0); pwmR(0);
  } else {
    driveLeft(dutyL, pidL.setpoint);
    driveRight(dutyR, pidR.setpoint);
  }

  // Publish CSV data
  if (millis() - lastCsvMs >= CSV_PUBLISH_MS) {
    lastCsvMs = millis();
    unsigned long elapsed = millis() - startTimeMs;
    float error = pidL.setpoint - actualRpmL;
    publishCsv(elapsed, pidL.setpoint, actualRpmL, error, dutyL);
  }
}

// ================================================================
//  WATCHDOG
// ================================================================

void checkWatchdog() {
#if WATCHDOG_ENABLE
  if ((pidL.setpoint != 0 || pidR.setpoint != 0) && 
      (millis() - lastCmdMs > WATCHDOG_MS)) {
    hardStop();
    testWaveform = "NONE";
  }
#endif
}

// ================================================================
//  SETUP
// ================================================================

void setup() {
  Serial.begin(115200);
  delay(500);

  // Initialize pins
  pinMode(DIR_L,   OUTPUT); digitalWrite(DIR_L,   LOW);
  pinMode(DIR_R,   OUTPUT); digitalWrite(DIR_R,   LOW);
  pinMode(LED_PIN, OUTPUT); digitalWrite(LED_PIN, LOW);

  // Initialize encoders
  pinMode(ENC_L_A, INPUT_PULLUP);
  pinMode(ENC_L_B, INPUT_PULLUP);
  pinMode(ENC_R_A, INPUT_PULLUP);
  pinMode(ENC_R_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_L_A), isrLA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_L_B), isrLB, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_R_A), isrRA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_R_B), isrRB, CHANGE);

  // Initialize PWM
  pwmSetup();
  pwmL(0);
  pwmR(0);

  // Initialize timing
  lastPidMs = millis();
  lastCsvMs = millis();
  lastCmdMs = millis();

  // Print header
  Serial.println("\n=================================================================");
  Serial.println("  PID Motor Tuner - GUI Compatible Firmware v1.0");
  Serial.println("  ESP32-S3 + Cytron DD10A + 20:1 Gearbox");
  Serial.println("=================================================================");
  Serial.printf("TICKS_PER_REV: %d\n", TICKS_PER_REV);
  Serial.printf("MAX_RPM: %d\n", MAX_RPM);
  Serial.println("\nCSV Header (for GUI parsing):");
  Serial.println("Time(s),Setpoint(RPM),Actual(RPM),Error(RPM),PWM(%),kP,kI,kD");
  Serial.println("\nReady for GUI commands...");
  Serial.println("Send: SQUARE, SINE, RAMP, STOP, kP:0.03, kI:0.001, kD:0.008");
  Serial.println("=================================================================\n");
}

// ================================================================
//  MAIN LOOP
// ================================================================

void loop() {
  processSerial();
  pidUpdate();
  checkWatchdog();
  delay(1);  // Small delay to prevent watchdog timeout
}
