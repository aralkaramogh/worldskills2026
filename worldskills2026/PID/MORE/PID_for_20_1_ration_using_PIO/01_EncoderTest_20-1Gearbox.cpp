/*
 * ENCODER TEST - ESP32-S3 + Cytron MDD10 + Quadrature Encoder
 * PlatformIO .cpp version for src/main.cpp
 * 
 * Step 1: Verify encoder readings and RPM calculation
 * UPDATED FOR: 5:1 + 4:1 GEARBOX (20:1 total reduction)
 * 
 * Hardware:
 * - ESP32-S3
 * - Cytron MDD10 Motor Driver
 * - DC Motor with Quadrature Encoder (28 CPR at motor shaft)
 * - Encoder A: GPIO 4
 * - Encoder B: GPIO 5
 * - Motor PWM: GPIO 9
 * - Motor DIR: GPIO 8
 */

#include <Arduino.h>

// ===== CONFIGURATION FOR 20:1 GEARBOX =====
#define ENCODER_A_PIN 16
#define ENCODER_B_PIN 15
#define MOTOR_PWM_PIN 5
#define MOTOR_DIR_PIN 4

// GEARBOX CONFIGURATION
// Motor base: 28 CPR
// Gearbox: 5:1 × 4:1 = 20:1 total reduction
// Output CPR: 28 × 20 = 560 CPR
//#define COUNTS_PER_REV 560  // 28 × (5 × 4) for 20:1 gearbox

#define COUNTS_PER_REV 2240 //(28 CPR × 20:1 gearbox x 4x quadrature)
#define MOTOR_MAX_RPM 300   // 6000 / 20 = 300 RPM at output

// ===== GLOBAL VARIABLES =====
volatile long encoderCount = 0;
volatile long lastEncoderCount = 0;
long previousMillis = 0;
const long SAMPLE_INTERVAL = 100;  // 100ms sampling for RPM calculation

float currentRPM = 0;
float motorPWM = 0;

// ===== FUNCTION PROTOTYPES =====
void encoderISR();
void handleSerialInput();
void printHelp();

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n===== ENCODER & RPM TEST =====");
  Serial.println("Configuration: 20:1 Gearbox (5:1 + 4:1)");
  Serial.print("Motor CPR (base): 28");
  Serial.print(" × 20 reduction = ");
  Serial.print(COUNTS_PER_REV);
  Serial.println(" CPR at output");
  Serial.print("Max RPM: ");
  Serial.println(MOTOR_MAX_RPM);
  Serial.println();
  
  // Encoder setup - using interrupts for accurate counting
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  
  // Attach interrupts for quadrature decoding
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), encoderISR, CHANGE);
  
  // Motor control setup
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  
  // PWM setup for ESP32
  ledcSetup(0, 5000, 8);  // Channel 0, 5kHz, 8-bit (0-255)
  ledcAttachPin(MOTOR_PWM_PIN, 0);
  
  // Start motor at 50% speed
  digitalWrite(MOTOR_DIR_PIN, HIGH);  // Forward direction
  motorPWM = 128;
  ledcWrite(0, (int)motorPWM);
  
  Serial.println("Motor started at 50% speed (PWM = 128)");
  Serial.println("\nTime(ms)\tCounts\tDelta\tRPM\tPWM%");
  Serial.println("================================================");
}

// ===== MAIN LOOP =====
void loop() {
  unsigned long currentMillis = millis();
  
  // Update RPM every SAMPLE_INTERVAL
  if (currentMillis - previousMillis >= SAMPLE_INTERVAL) {
    previousMillis = currentMillis;
    
    // Calculate RPM
    long deltaCounts = encoderCount - lastEncoderCount;
    lastEncoderCount = encoderCount;
    
    // RPM = (counts/interval) * (60000ms / interval) / CPR
    currentRPM = (deltaCounts * 60000.0) / (SAMPLE_INTERVAL * COUNTS_PER_REV);
    
    // Print data
    Serial.print(currentMillis);
    Serial.print("\t");
    Serial.print(encoderCount);
    Serial.print("\t");
    Serial.print(deltaCounts);
    Serial.print("\t");
    Serial.print(currentRPM, 1);
    Serial.print("\t");
    Serial.println((motorPWM / 255.0) * 100, 1);
  }
  
  // Handle serial commands
  handleSerialInput();
  
  delay(10);
}

// ===== QUADRATURE ENCODER ISR =====
void encoderISR() {
  int a = digitalRead(ENCODER_A_PIN);
  int b = digitalRead(ENCODER_B_PIN);
  
  static int lastA = 0;
  static int lastB = 0;
  
  if (a != lastA) {
    if (a == b) {
      encoderCount++;  // Forward
    } else {
      encoderCount--;  // Reverse
    }
    lastA = a;
  } else if (b != lastB) {
    if (a != b) {
      encoderCount++;  // Forward
    } else {
      encoderCount--;  // Reverse
    }
    lastB = b;
  }
}

// ===== SERIAL COMMAND HANDLER =====
void handleSerialInput() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.startsWith("PWM:")) {
      int pwmValue = command.substring(4).toInt();
      pwmValue = constrain(pwmValue, 0, 255);
      motorPWM = pwmValue;
      ledcWrite(0, pwmValue);
      Serial.print("PWM set to: ");
      Serial.print(pwmValue);
      Serial.print(" (");
      Serial.print((pwmValue / 255.0) * 100, 1);
      Serial.println("%)");
    }
    else if (command.equals("STOP")) {
      motorPWM = 0;
      ledcWrite(0, 0);
      Serial.println("Motor stopped");
    }
    else if (command.equals("RESET")) {
      encoderCount = 0;
      lastEncoderCount = 0;
      Serial.println("Encoder count reset to 0");
    }
    else if (command.equals("FWD")) {
      digitalWrite(MOTOR_DIR_PIN, HIGH);
      Serial.println("Direction: Forward");
    }
    else if (command.equals("REV")) {
      digitalWrite(MOTOR_DIR_PIN, LOW);
      Serial.println("Direction: Reverse");
    }
    else if (command.equals("INFO")) {
      Serial.println("\n===== SYSTEM INFO =====");
      Serial.println("Configuration: 20:1 Gearbox (5:1 + 4:1)");
      Serial.print("Encoder CPR: ");
      Serial.println(COUNTS_PER_REV);
      Serial.print("Max RPM: ");
      Serial.println(MOTOR_MAX_RPM);
      Serial.print("Current RPM: ");
      Serial.println(currentRPM, 1);
      Serial.print("Current PWM: ");
      Serial.print(motorPWM);
      Serial.print(" (");
      Serial.print((motorPWM / 255.0) * 100, 1);
      Serial.println("%)");
      Serial.println("=======================\n");
    }
    else if (command.equals("HELP")) {
      printHelp();
    }
  }
}

void printHelp() {
  Serial.println("\n===== COMMANDS =====");
  Serial.println("PWM:0-255   - Set motor PWM (0-255)");
  Serial.println("FWD         - Forward direction");
  Serial.println("REV         - Reverse direction");
  Serial.println("STOP        - Stop motor");
  Serial.println("RESET       - Reset encoder count");
  Serial.println("INFO        - Show system info");
  Serial.println("HELP        - Show this help");
  Serial.println("====================");
  Serial.println("\nExpected RPM at different PWM values:");
  Serial.println("PWM 50   (~20%)  → ~60 RPM");
  Serial.println("PWM 128  (~50%)  → ~150 RPM");
  Serial.println("PWM 200  (~78%)  → ~234 RPM");
  Serial.println("PWM 255  (100%)  → ~300 RPM\n");
}
