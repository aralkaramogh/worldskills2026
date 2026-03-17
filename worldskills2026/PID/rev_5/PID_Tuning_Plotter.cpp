/*
RPM:150           (set target RPM to 150)
kP:0.8            (set P gain)
kI:0.0            (set I gain)
kD:0.0            (set D gain)
STOP              (stop motor)
RESET             (reset encoder and timer)
*/

#include <Arduino.h>

// ===== PINS =====
#define ENCODER_A_PIN 16
#define ENCODER_B_PIN 15
#define MOTOR_PWM_PIN 5
#define MOTOR_DIR_PIN 4

// ===== GEARBOX =====
#define COUNTS_PER_REV 560
#define MOTOR_MAX_RPM 300
#define SAMPLE_INTERVAL 100

// ===== ENCODER =====
volatile long encoderCount = 0;
volatile long lastEncoderCount = 0;

// ===== PID GAINS =====
float kP = 5.0f;
float kI = 0.0f;
float kD = 0.0f;

// ===== PID STATE =====
float setpointRPM = 0.0f;
float currentRPM = 0.0f;
float error = 0.0f;
float lastError = 0.0f;
float integral = 0.0f;
float motorPWM = 0.0f;

// ===== TIMING =====
unsigned long previousMillis = 0;
unsigned long startMillis = 0;

// ===== SERIAL BUFFER =====
String serialBuffer = "";

// ===== ISR =====
void IRAM_ATTR encoderISR() {
  int a = digitalRead(ENCODER_A_PIN);
  int b = digitalRead(ENCODER_B_PIN);
  
  static int lastA = 0;
  static int lastB = 0;
  
  if (a != lastA) {
    if (a == b) {
      encoderCount++;
    } else {
      encoderCount--;
    }
    lastA = a;
  } else if (b != lastB) {
    if (a != b) {
      encoderCount++;
    } else {
      encoderCount--;
    }
    lastB = b;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  
  // Encoder setup
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), encoderISR, CHANGE);
  
  // Motor setup
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  ledcSetup(0, 5000, 8);
  ledcAttachPin(MOTOR_PWM_PIN, 0);
  digitalWrite(MOTOR_DIR_PIN, HIGH);
  
  previousMillis = millis();
  startMillis = millis();
  
  // Plotter header
  Serial.println("Time_s\tSetpoint_RPM\tActual_RPM\tError_RPM\tPWM_%\tkP\tkI\tkD");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // PID update every SAMPLE_INTERVAL
  if (currentMillis - previousMillis >= SAMPLE_INTERVAL) {
    float dt = (currentMillis - previousMillis) / 1000.0f;
    previousMillis = currentMillis;
    
    // Calculate RPM
    long deltaCounts = encoderCount - lastEncoderCount;
    lastEncoderCount = encoderCount;
    currentRPM = (deltaCounts * 60000.0f) / (SAMPLE_INTERVAL * COUNTS_PER_REV);
    
    // PID calculation
    error = setpointRPM - currentRPM;
    integral += error * dt;
    integral = constrain(integral, -100.0f, 100.0f);
    
    float derivative = (error - lastError) / dt;
    lastError = error;
    
    motorPWM = (kP * error) + (kI * integral) + (kD * derivative);
    motorPWM = constrain(motorPWM, -255.0f, 255.0f);
    
    // Apply to motor
    if (motorPWM >= 0) {
      digitalWrite(MOTOR_DIR_PIN, HIGH);
      ledcWrite(0, (int)motorPWM);
    } else {
      digitalWrite(MOTOR_DIR_PIN, LOW);
      ledcWrite(0, (int)(-motorPWM));
    }
    
    // Print for plotter
    float elapsedTime = (currentMillis - startMillis) / 1000.0f;
    Serial.print(elapsedTime, 2);
    Serial.print("\t");
    Serial.print(setpointRPM, 1);
    Serial.print("\t");
    Serial.print(currentRPM, 1);
    Serial.print("\t");
    Serial.print(error, 1);
    Serial.print("\t");
    Serial.print((fabs(motorPWM) / 255.0f) * 100.0f, 1);
    Serial.print("\t");
    Serial.print(kP, 4);
    Serial.print("\t");
    Serial.print(kI, 4);
    Serial.print("\t");
    Serial.println(kD, 4);
  }
  
  // Handle serial commands
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
