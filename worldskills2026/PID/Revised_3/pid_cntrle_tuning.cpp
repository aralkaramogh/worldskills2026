/*
 * ESP32-S3 PID Velocity Controller with Quadrature Encoder
 * Designed for PlatformIO
 * 
 * This code reads a quadrature encoder, calculates velocity, and applies
 * PID control to maintain desired velocity. Data is sent to Serial Plotter.
 * 
 * Encoder: Quadrature AB phase incremental encoder
 * Motor Output: PWM pin
 * Serial Output: Desired Velocity, Current Velocity, PID Output (115200 baud)
 */

#include <Arduino.h>
#include <math.h>

// ==================== PIN CONFIGURATION ====================
#define ENCODER_A 16        // Quadrature Phase A
#define ENCODER_B 15        // Quadrature Phase B
#define MOTOR_PWM 5        // PWM output to motor driver (0-255)
#define MOTOR_DIR 4        // Motor direction pin (HIGH=forward, LOW=reverse)

// ==================== MOTOR SPECIFICATIONS ====================
// REV HD Hex Motor (REV-41-1291) with 20:1 Gearbox
// Base motor speed: 6000 RPM (no load)
// Output speed with 20:1: 300 RPM max
// Encoder: 28 counts/revolution at motor
// Effective encoder resolution at output: 28 * 20 = 560 counts/revolution

// ==================== PID PARAMETERS (TUNE THESE) ====================
// Recommended starting values for REV motor with moderate load
float Kp = 0.8;             // Proportional gain - increased for geared motor responsiveness
float Ki = 0.0;            // Integral gain - helps eliminate steady-state error
float Kd = 0.0;            // Derivative gain - reduces overshoot

float desiredVelocity = 150.0;  // RPM at output shaft (0-300 RPM max with 20:1 ratio)

// ==================== ENCODER CONFIGURATION ====================
#define MOTOR_PPR 28                    // Motor encoder: 28 counts/revolution
#define GEAR_RATIO 20                   // Gearbox reduction ratio: 20:1
#define OUTPUT_PPR (MOTOR_PPR * GEAR_RATIO)  // Effective PPR at output: 560
#define VELOCITY_SAMPLES 20             // Number of samples to average velocity

volatile long encoderCount = 0;
volatile long lastEncoderCount = 0;
long previousEncoderCount = 0;

float currentVelocity = 0.0;
float velocityArray[VELOCITY_SAMPLES] = {0};
int velocityIndex = 0;

// ==================== PID VARIABLES ====================
float pidError = 0;
float pidPrevError = 0;
float pidIntegral = 0;
float pidOutput = 0;
float pidOutputMax = 255;    // Max PWM value
float pidOutputMin = -255;   // Min PWM value (for reverse)

unsigned long lastTime = 0;
unsigned long currentTime = 0;
float deltaTime = 0;

// ==================== TIMING ====================
unsigned long lastVelocityCalcTime = 0;
unsigned long velocityCalcInterval = 100;  // Calculate velocity every 100ms

// ==================== SERIAL OUTPUT ====================
unsigned long lastSerialTime = 0;
unsigned long serialInterval = 50;  // Send to serial plotter every 50ms

// ==================== FUNCTION DECLARATIONS ====================
void encoderInterrupt();
void calculateVelocity();
void updatePID();
void applyMotorControl();
void stopMotor();
void sendSerialPlotterData();
void checkSerialInput();
void printHelp();

// ==================== SETUP ====================
void setup() {
  // Serial communication
  Serial.begin(115200);  // High baud rate for ESP32
  delay(1000);
  
  Serial.println("\n=== ESP32-S3 PID Motor Controller ===");
  Serial.println("Motor: REV HD Hex Motor (REV-41-1291)");
  Serial.println("Gearbox: 20:1 Reduction");
  Serial.println("Max Output RPM: 300");
  Serial.println("Encoder: 28 CPR (560 CPR effective at output)");
  Serial.println("");
  Serial.println("Pin Configuration:");
  Serial.println("  Encoder A: GPIO " + String(ENCODER_A));
  Serial.println("  Encoder B: GPIO " + String(ENCODER_B));
  Serial.println("  Motor PWM: GPIO " + String(MOTOR_PWM));
  Serial.println("  Motor Dir: GPIO " + String(MOTOR_DIR));
  
  // Pin configuration
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  pinMode(MOTOR_PWM, OUTPUT);
  pinMode(MOTOR_DIR, OUTPUT);
  
  // Attach interrupts for quadrature encoder
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), encoderInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B), encoderInterrupt, CHANGE);
  
  // Motor PWM frequency (25kHz is good for most motors)
  ledcSetup(0, 25000, 8);  // Channel 0, 25kHz frequency, 8-bit resolution
  ledcAttachPin(MOTOR_PWM, 0);
  
  // Stop motor initially
  stopMotor();
  
  lastTime = millis();
  lastVelocityCalcTime = millis();
  lastSerialTime = millis();
  
  delay(2000);
  Serial.println("\n✓ System Ready!");
  Serial.println("Velocity is displayed in RPM at the output shaft");
  Serial.println("Maximum safe velocity: 300 RPM");
  Serial.println("Recommended test velocity: 100-200 RPM");
  Serial.println("\nTuning Tips:");
  Serial.println("  - Start with Kp=0.8, Ki=0.15, Kd=0.12");
  Serial.println("  - Increase Kp if response is slow");
  Serial.println("  - Increase Kd if response oscillates");
  Serial.println("  - Test at multiple RPM values: 50, 100, 150, 200\n");
}

// ==================== MAIN LOOP ====================
void loop() {
  currentTime = millis();
  deltaTime = (currentTime - lastTime) / 1000.0;  // Convert to seconds
  
  if (deltaTime > 0.05) {  // Update every 50ms minimum
    lastTime = currentTime;
    
    // Calculate velocity every 100ms
    if (currentTime - lastVelocityCalcTime >= velocityCalcInterval) {
      calculateVelocity();
      lastVelocityCalcTime = currentTime;
    }
    
    // Update PID controller
    updatePID();
    
    // Apply motor control
    applyMotorControl();
    
    // Send data to Serial Plotter
    if (currentTime - lastSerialTime >= serialInterval) {
      sendSerialPlotterData();
      lastSerialTime = currentTime;
    }
  }
  
  // Check for serial input to adjust parameters
  checkSerialInput();
}

// ==================== ENCODER INTERRUPT HANDLER ====================
void encoderInterrupt() {
  // Read current state
  bool aState = digitalRead(ENCODER_A);
  bool bState = digitalRead(ENCODER_B);
  
  // Quadrature decoding (2x resolution)
  static bool lastAState = false;
  static bool lastBState = false;
  
  if (aState != lastAState) {
    if (aState == bState) {
      encoderCount++;  // Forward
    } else {
      encoderCount--;  // Reverse
    }
    lastAState = aState;
  }
  
  if (bState != lastBState) {
    if (bState != aState) {
      encoderCount++;  // Forward
    } else {
      encoderCount--;  // Reverse
    }
    lastBState = bState;
  }
}

// ==================== VELOCITY CALCULATION ====================
void calculateVelocity() {
  // Get change in encoder count
  long deltaCount = encoderCount - previousEncoderCount;
  previousEncoderCount = encoderCount;
  
  // Calculate velocity in RPM at output shaft
  // Formula: (deltaCount / OUTPUT_PPR) * (60,000 / velocityCalcInterval) = RPM
  // deltaCount = encoder pulses in interval
  // OUTPUT_PPR = 560 (28 motor counts * 20 gear ratio)
  // 60,000 = 60 seconds * 1000 milliseconds
  // velocityCalcInterval = 100ms
  float revolutions = (float)deltaCount / OUTPUT_PPR;
  float rawVelocity = revolutions * (60000.0 / velocityCalcInterval);  // RPM at output shaft
  
  // Apply moving average filter for smooth velocity
  velocityArray[velocityIndex] = rawVelocity;
  velocityIndex = (velocityIndex + 1) % VELOCITY_SAMPLES;
  
  // Calculate average
  float sum = 0;
  for (int i = 0; i < VELOCITY_SAMPLES; i++) {
    sum += velocityArray[i];
  }
  currentVelocity = sum / VELOCITY_SAMPLES;
}

// ==================== PID CONTROLLER ====================
void updatePID() {
  // Calculate error
  pidError = desiredVelocity - currentVelocity;
  
  // Proportional term
  float pTerm = Kp * pidError;
  
  // Integral term (with anti-windup)
  pidIntegral += pidError * deltaTime;
  if (pidIntegral > pidOutputMax) pidIntegral = pidOutputMax;
  if (pidIntegral < pidOutputMin) pidIntegral = pidOutputMin;
  float iTerm = Ki * pidIntegral;
  
  // Derivative term
  float dTerm = 0;
  if (deltaTime > 0) {
    dTerm = Kd * (pidError - pidPrevError) / deltaTime;
  }
  pidPrevError = pidError;
  
  // Combine terms
  pidOutput = pTerm + iTerm + dTerm;
  
  // Constrain output
  pidOutput = constrain(pidOutput, pidOutputMin, pidOutputMax);
}

// ==================== MOTOR CONTROL ====================
void applyMotorControl() {
  if (pidOutput >= 0) {
    // Forward direction
    digitalWrite(MOTOR_DIR, HIGH);
    ledcWrite(0, (int)pidOutput);
  } else {
    // Reverse direction
    digitalWrite(MOTOR_DIR, LOW);
    ledcWrite(0, (int)(-pidOutput));
  }
}

void stopMotor() {
  digitalWrite(MOTOR_DIR, LOW);
  ledcWrite(0, 0);
}

// ==================== SERIAL PLOTTER OUTPUT ====================
void sendSerialPlotterData() {
  // Format for Arduino Serial Plotter:
  // desiredVelocity,currentVelocity,pidOutput
  // This creates three independent plots
  
  Serial.print(desiredVelocity);
  Serial.print(",");
  Serial.print(currentVelocity);
  Serial.print(",");
  Serial.println(pidOutput);
}

// ==================== SERIAL MONITOR CONTROL ====================
void checkSerialInput() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.startsWith("Kp=")) {
      Kp = input.substring(3).toFloat();
      Serial.println("Kp updated to: " + String(Kp, 4));
    }
    else if (input.startsWith("Ki=")) {
      Ki = input.substring(3).toFloat();
      Serial.println("Ki updated to: " + String(Ki, 4));
    }
    else if (input.startsWith("Kd=")) {
      Kd = input.substring(3).toFloat();
      Serial.println("Kd updated to: " + String(Kd, 4));
    }
    else if (input.startsWith("v=")) {
      desiredVelocity = input.substring(2).toFloat();
      Serial.println("Desired Velocity updated to: " + String(desiredVelocity, 2));
    }
    else if (input == "stop") {
      stopMotor();
      Serial.println("Motor stopped");
    }
    else if (input == "status") {
      Serial.println("\n=== REV HD Hex Motor (20:1) Status ===");
      Serial.println("PID Parameters:");
      Serial.println("  Kp: " + String(Kp, 4));
      Serial.println("  Ki: " + String(Ki, 4));
      Serial.println("  Kd: " + String(Kd, 4));
      Serial.println("");
      Serial.println("Motor Control:");
      Serial.println("  Desired Velocity: " + String(desiredVelocity, 1) + " RPM");
      Serial.println("  Current Velocity: " + String(currentVelocity, 1) + " RPM");
      Serial.println("  PID Error: " + String(pidError, 1) + " RPM");
      Serial.println("  PID Output: " + String(pidOutput, 0) + " (PWM 0-255)");
      Serial.println("");
      Serial.println("Encoder Data:");
      Serial.println("  Encoder Count: " + String(encoderCount));
      Serial.println("  Output PPR: " + String(OUTPUT_PPR));
      Serial.println("  Delta Count (last interval): " + String(encoderCount - lastEncoderCount));
      Serial.println("====\n");
      lastEncoderCount = encoderCount;
    }
    else {
      printHelp();
    }
  }
}

void printHelp() {
  Serial.println("\n=== Commands ===");
  Serial.println("Kp=value  - Set Kp (e.g., Kp=0.8)");
  Serial.println("Ki=value  - Set Ki (e.g., Ki=0.1)");
  Serial.println("Kd=value  - Set Kd (e.g., Kd=0.05)");
  Serial.println("v=value   - Set desired velocity (e.g., v=50)");
  Serial.println("stop      - Stop motor");
  Serial.println("status    - Show current parameters and values");
  Serial.println("help      - Show this message\n");
}
