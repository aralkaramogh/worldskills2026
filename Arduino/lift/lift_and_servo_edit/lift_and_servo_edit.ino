/*
===========================================================
ESP32 LIFT + GRIPPER CONTROLLER (ROS2-READY INTERFACE)
===========================================================

AUTHOR: (Your Name)
ROLE: Low-Level Actuator Controller (for ROS2 integration)

-----------------------------------------------------------
📌 SYSTEM ROLE (IMPORTANT FOR ROS DEVELOPER)
-----------------------------------------------------------

This ESP32 acts as a:
→ Deterministic actuator controller
→ Executes lift + gripper commands reliably

It DOES NOT handle:
✘ Vision (YOLO)
✘ Planning
✘ Navigation

It ONLY executes:
✔ Height commands (mm)
✔ Gripper commands (open/close)

-----------------------------------------------------------
📡 COMMUNICATION PROTOCOL (SERIAL)
-----------------------------------------------------------

All commands are ASCII-based, newline terminated.

---------------- INPUT COMMANDS ----------------

Lxxx   → Move lift to height in mm
         Example: L120

U      → Manual UP
D      → Manual DOWN
S      → Stop motion
E      → EMERGENCY STOP (highest priority)

o      → Open gripper
c      → Close gripper (with micro-release)

P      → Request position
B      → Request BUSY/IDLE status

---------------- OUTPUT RESPONSES ----------------

STATUS:BUSY
STATUS:IDLE
STATUS:DONE

POS:<encoder_count>

GRIPPER:OPEN
GRIPPER:CLOSED

ERROR:OUT_OF_RANGE
EMERGENCY_STOP

-----------------------------------------------------------
🤖 ROS2 INTEGRATION EXPECTATION
-----------------------------------------------------------

ROS side should:

1. Send:
   Lxxx (height in mm)

2. Wait for:
   STATUS:DONE

3. Then send:
   c (close gripper)

4. For placement:
   send Lxxx → wait DONE → send o

-----------------------------------------------------------
⚠️ SAFETY CONTRACT
-----------------------------------------------------------

✔ Input values are clamped internally
✔ Motion stops automatically near target
✔ Emergency stop available via 'E'
✔ No homing required (manual zero at boot)

-----------------------------------------------------------
📏 CALIBRATION NOTES
-----------------------------------------------------------

counts_per_mm = encoder_counts / actual_height_mm

Example:
1400 counts / 300 mm = 4.66 counts/mm

-----------------------------------------------------------
📌 STARTUP PROCEDURE
-----------------------------------------------------------

1. Manually bring lift to bottom
2. Reset ESP32
3. System assumes:
   encoderCount = 0 (bottom reference)

===========================================================
*/

#include <Arduino.h>
#include <ESP32Servo.h>

// ---------------- PIN CONFIG ----------------
#define PWM_PIN 7
#define DIR_PIN 6

#define ENC_A 18
#define ENC_B 17

#define SERVO_PIN 15


// ---------------- SYSTEM PARAMETERS ----------------
//float counts_per_mm = 4.66;
//long max_counts = 1397;
//float counts_per_mm = 2.87;
//long max_counts = 949;

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
  if (b == HIGH)
    encoderCount--;
  else
    encoderCount++  ;
}

// ---------------- MOTOR CONTROL ----------------
void motorStop() {
  ledcWrite(PWM_PIN, 0);
  moving = false;
  Serial.println("STATUS:IDLE");
}

void motorUp(int speed) {
  digitalWrite(DIR_PIN, HIGH);
  ledcWrite(PWM_PIN, speed);
  moving = true;
}

void motorDown(int speed) {
  digitalWrite(DIR_PIN, LOW);
  ledcWrite(PWM_PIN, speed);
  moving = true;
}

// ---------------- MOVE ----------------
void moveToHeight(float height_mm) {

  // Clamp for safety
  if (height_mm < 0) height_mm = 0;
  if (height_mm > (max_counts / counts_per_mm))
    height_mm = max_counts / counts_per_mm;

  long target = height_mm * counts_per_mm;

  targetPosition = target;
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
      if (moving)
        Serial.println("STATUS:BUSY");
      else
        Serial.println("STATUS:IDLE");
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
      if (abs(error) < 50)
        motorUp(slowSpeed);
      else
        motorUp(pwmSpeed);
    }
    else {
      if (abs(error) < 50)
        motorDown(slowSpeed);
      else
        motorDown(pwmSpeed);
    }
  }
}