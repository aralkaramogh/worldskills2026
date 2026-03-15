/*
 * MOTOR DIAGNOSTIC TEST
 * Check if motor is actually spinning and encoder works
 * ESP32-S3 + Cytron DD10A + HD Hex Encoder
 */

#include <Arduino.h>

#define ENCODER_A_PIN 16
#define ENCODER_B_PIN 15
#define MOTOR_PWM_PIN 5
#define MOTOR_DIR_PIN 4

//#define COUNTS_PER_REV 560
#define COUNTS_PER_REV 2240 //(28 CPR × 20:1 gearbox x 4x quadrature)

volatile long encoderCount = 0;
unsigned long lastEncoderCount = 0;

void IRAM_ATTR encoderISR() {
  encoderCount++;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n===== MOTOR DIAGNOSTIC TEST =====");
  Serial.println("This test checks if motor and encoder work");
  Serial.println();
  
  // Encoder setup
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);
  
  // Motor setup
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  ledcSetup(0, 5000, 8);
  ledcAttachPin(MOTOR_PWM_PIN, 0);
  digitalWrite(MOTOR_DIR_PIN, HIGH);  // Forward
  
  Serial.println("Commands:");
  Serial.println("  PWM:0    - Stop motor");
  Serial.println("  PWM:50   - 50% power forward");
  Serial.println("  PWM:100  - 100% power forward");
  Serial.println("  PWM:255  - Full power");
  Serial.println("  REV:100  - Reverse at 100%");
  Serial.println("  STATUS   - Show encoder count");
  Serial.println();
}

String serialBuffer = "";

void loop() {
  // Print encoder count every 500ms
  static unsigned long lastPrint = 0;
  unsigned long now = millis();
  
  if (now - lastPrint >= 500) {
    lastPrint = now;
    
    long ticks = encoderCount;
    long rpm = (ticks * 60000) / (500 * COUNTS_PER_REV);
    
    Serial.printf("Encoder: %ld ticks | RPM: %ld | PWM: %d\n", 
      ticks, rpm, (int)(ledcRead(0)));
  }
  
  // Non-blocking serial
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        serialBuffer.trim();
        serialBuffer.toUpperCase();
        
        if (serialBuffer.startsWith("PWM:")) {
          int pwm = serialBuffer.substring(4).toInt();
          pwm = constrain(pwm, 0, 255);
          
          if (pwm == 0) {
            Serial.println("STOPPING");
            digitalWrite(MOTOR_DIR_PIN, HIGH);
            ledcWrite(0, 0);
          } else {
            Serial.printf("Forward at %d PWM\n", pwm);
            digitalWrite(MOTOR_DIR_PIN, HIGH);
            ledcWrite(0, pwm);
          }
        }
        else if (serialBuffer.startsWith("REV:")) {
          int pwm = serialBuffer.substring(4).toInt();
          pwm = constrain(pwm, 0, 255);
          
          Serial.printf("Reverse at %d PWM\n", pwm);
          digitalWrite(MOTOR_DIR_PIN, LOW);
          ledcWrite(0, pwm);
        }
        else if (serialBuffer == "STATUS") {
          Serial.printf("Encoder count: %ld\n", encoderCount);
          Serial.printf("Current PWM: %d\n", (int)(ledcRead(0)));
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
