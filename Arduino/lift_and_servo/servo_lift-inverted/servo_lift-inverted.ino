#include <Arduino.h>
#include <ESP32Servo.h>

// ---------------- PIN CONFIG ----------------
#define PWM_PIN 7
#define DIR_PIN 6

#define ENC_A 18
#define ENC_B 17

#define SERVO_PIN 15

// ---------------- SYSTEM PARAMETERS ----------------
float counts_per_mm = 1.38;
long max_counts = 414;

int pwmSpeed = 150;
int slowSpeed = 100;

// ---------------- SERVO PARAMETERS ----------------
Servo servo1;

int openPos  = 178;
int closePos = 130;
int releaseOffset = 2;

// ---------------- VARIABLES ----------------
volatile long encoderCount = 0;

long targetPosition = 0;
bool moving = false;

// ---------------- ENCODER ISR ----------------
void IRAM_ATTR handleEncoder() {
  int b = digitalRead(ENC_B);

  // ✅ MATCHED WITH CALIBRATION CODE
  if (b == HIGH)
    encoderCount--;
  else
    encoderCount++;
}

// ---------------- MOTOR CONTROL ----------------
void motorStop() {
  ledcWrite(PWM_PIN, 0);
  moving = false;
  Serial.println("STATUS:IDLE");
}

void motorUp(int speed) {
  digitalWrite(DIR_PIN, LOW);   // ✅ FIXED
  ledcWrite(PWM_PIN, speed);
  moving = true;
}

void motorDown(int speed) {
  digitalWrite(DIR_PIN, HIGH);  // ✅ FIXED
  ledcWrite(PWM_PIN, speed);
  moving = true;
}

// ---------------- MOVE ----------------
void moveToHeight(float height_mm) {

  // Clamp
  if (height_mm < 0) height_mm = 0;
  if (height_mm > (max_counts / counts_per_mm))
    height_mm = max_counts / counts_per_mm;

  targetPosition = height_mm * counts_per_mm;

  moving = true;

  Serial.println("STATUS:BUSY");
  Serial.print("MOVING TO:");
  Serial.println(height_mm);
}

// ---------------- GRIPPER ----------------
void openGripper() {
  servo1.attach(SERVO_PIN, 500, 2500);
  servo1.write(openPos);
  Serial.println("GRIPPER:OPEN");
}

void closeGripper() {
  servo1.attach(SERVO_PIN, 500, 2500);

  servo1.write(closePos);
  delay(300);

  servo1.write(closePos + releaseOffset);
  Serial.println("GRIPPER:CLOSED");
}

// ---------------- SETUP ----------------
void setup() {

  Serial.begin(115200);

  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENC_A, INPUT);
  pinMode(ENC_B, INPUT);

  attachInterrupt(digitalPinToInterrupt(ENC_A), handleEncoder, RISING);

  ledcAttach(PWM_PIN, 5000, 8);
  ledcWrite(PWM_PIN, 0);

  servo1.setPeriodHertz(50);
  servo1.attach(SERVO_PIN, 500, 2500);
  servo1.write(openPos);

  encoderCount = 0;

  Serial.println("STATUS:IDLE");
  Serial.println("SYSTEM READY");
}

// ---------------- LOOP ----------------
void loop() {

  // -------- SERIAL COMMANDS --------
  if (Serial.available()) {

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.startsWith("L")) {
      float height = cmd.substring(1).toFloat();
      moveToHeight(height);
    }

    else if (cmd == "S") {
      motorStop();
    }

    else if (cmd == "E") {
      motorStop();
      Serial.println("EMERGENCY_STOP");
    }

    else if (cmd == "P") {
      Serial.print("POS:");
      Serial.println(encoderCount);
    }

    else if (cmd == "B") {
      Serial.println(moving ? "STATUS:BUSY" : "STATUS:IDLE");
    }

    else if (cmd == "U") {
      moving = false;
      motorUp(pwmSpeed);
      Serial.println("STATUS:BUSY");
    }

    else if (cmd == "D") {
      moving = false;
      motorDown(pwmSpeed);
      Serial.println("STATUS:BUSY");
    }

    // -------- GRIPPER --------
    else if (cmd == "o") {
      openGripper();
    }

    else if (cmd == "c") {
      closeGripper();
    }
  }

  // -------- POSITION CONTROL --------
  if (moving) {

    long error = targetPosition - encoderCount;

    if (abs(error) < 5) {
      motorStop();
      Serial.println("STATUS:DONE");
    }
    else if (error > 0) {
      (abs(error) < 50) ? motorUp(slowSpeed) : motorUp(pwmSpeed);
    }
    else {
      (abs(error) < 50) ? motorDown(slowSpeed) : motorDown(pwmSpeed);
    }
  }
}