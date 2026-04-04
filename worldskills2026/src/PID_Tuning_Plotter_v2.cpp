
#include <Arduino.h>

// ================== PINS ==================
#define ENCODER_A_PIN 18
#define ENCODER_B_PIN 17
#define MOTOR_PWM_PIN 16
#define MOTOR_DIR_PIN 15

// ================== MOTOR ==================
#define COUNTS_PER_REV 560
#define MOTOR_MAX_RPM 300

// ================== CONTROL ==================
#define SAMPLE_INTERVAL 100      // ms (control loop)
#define PWM_FREQ 20000           // 20 kHz PWM
#define PWM_CHANNEL 0
#define PWM_RESOLUTION 8
#define MIN_PWM 40               // overcome gearbox friction

// ================== ENCODER ==================
volatile long encoderCount = 0;
long lastEncoderCount = 0;

// ================== PID GAINS ==================
float kP = 0.8f;
float kI = 0.0f;
float kD = 0.0f;

// ================== PID STATE ==================
float setpointRPM = 0.0f;
float currentRPM = 0.0f;

float error = 0.0f;
float lastError = 0.0f;
float integral = 0.0f;

float motorPWM = 0.0f;

// ================== TIMING ==================
unsigned long previousMillis = 0;
unsigned long startMillis = 0;

// ================== SERIAL ==================
String serialBuffer = "";

// ================== ENCODER ISR ==================
void IRAM_ATTR encoderISR() {

  int a = digitalRead(ENCODER_A_PIN);
  int b = digitalRead(ENCODER_B_PIN);

  if (a == b)
    encoderCount++;
  else
    encoderCount--;
}

// ================== SETUP ==================
void setup() {

  Serial.begin(115200);
  delay(500);

  // Encoder
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);

  // Motor
  pinMode(MOTOR_DIR_PIN, OUTPUT);

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR_PWM_PIN, PWM_CHANNEL);

  digitalWrite(MOTOR_DIR_PIN, HIGH);

  previousMillis = millis();
  startMillis = millis();

  Serial.println("Time_s\tSetpoint_RPM\tActual_RPM\tError_RPM\tPWM_%\tkP\tkI\tkD");
}

// ================== LOOP ==================
void loop() {

  unsigned long currentMillis = millis();

  // ===== CONTROL LOOP =====
  if (currentMillis - previousMillis >= SAMPLE_INTERVAL) {

    float dt = (currentMillis - previousMillis) / 1000.0f;
    previousMillis = currentMillis;

    // ---------- RPM calculation ----------
    long deltaCounts = encoderCount - lastEncoderCount;
    lastEncoderCount = encoderCount;

    currentRPM = (deltaCounts * 60.0f) / (COUNTS_PER_REV * dt);

    // ---------- PID ----------
    error = setpointRPM - currentRPM;
    Serial.print("Error: "); Serial.println(error);

    integral += error * dt;
    integral = constrain(integral, -100.0f, 100.0f);

    float derivative = (error - lastError) / dt;
    lastError = error;

    motorPWM = (kP * error) + (kI * integral) + (kD * derivative);
    motorPWM = constrain(motorPWM, -255.0f, 255.0f);

    // ---------- Apply to motor ----------
    int pwm = abs(motorPWM);

    if (pwm > 0 && pwm < MIN_PWM)
      pwm = MIN_PWM;

    if (motorPWM >= 0) {
      digitalWrite(MOTOR_DIR_PIN, HIGH);
      ledcWrite(PWM_CHANNEL, pwm);
    }
    else {
      digitalWrite(MOTOR_DIR_PIN, LOW);
      ledcWrite(PWM_CHANNEL, pwm);
    }

    // ---------- Serial Plot ----------
    float elapsedTime = (currentMillis - startMillis) / 1000.0f;

    Serial.print(elapsedTime, 2);
    Serial.print("\t");

    Serial.print(setpointRPM, 1);
    Serial.print("\t");

    Serial.print(currentRPM, 1);
    Serial.print("\t");

    Serial.print(error, 1);
    Serial.print("\t");

    Serial.print((pwm / 255.0f) * 100.0f, 1);
    Serial.print("\t");

    Serial.print(kP, 4);
    Serial.print("\t");

    Serial.print(kI, 4);
    Serial.print("\t");

    Serial.println(kD, 4);
  }

  // ===== SERIAL COMMAND HANDLER =====
  while (Serial.available()) {

    char c = Serial.read();

    if (c == '\n' || c == '\r') {

      if (serialBuffer.length() > 0) {

        serialBuffer.trim();
        serialBuffer.toUpperCase();

        if (serialBuffer.startsWith("RPM:")) {
          setpointRPM = serialBuffer.substring(4).toFloat();
        }

        else if (serialBuffer.startsWith("KP:")) {
          kP = serialBuffer.substring(3).toFloat();
        }

        else if (serialBuffer.startsWith("KI:")) {
          kI = serialBuffer.substring(3).toFloat();
        }

        else if (serialBuffer.startsWith("KD:")) {
          kD = serialBuffer.substring(3).toFloat();
        }

        else if (serialBuffer == "STOP") {
          setpointRPM = 0;
          integral = 0;
        }

        else if (serialBuffer == "RESET") {
          encoderCount = 0;
          lastEncoderCount = 0;
          integral = 0;
          lastError = 0;
          startMillis = millis();
        }

        serialBuffer = "";
      }
    }

    else if (serialBuffer.length() < 32) {
      serialBuffer += c;
    }
  }

  delay(1);
}

