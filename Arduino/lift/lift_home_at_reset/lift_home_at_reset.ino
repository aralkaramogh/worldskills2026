/*
===========================================================
ESP32 LIFT CONTROL (SIMPLE & STABLE VERSION)
===========================================================

✔ No homing (manual zero)
✔ On boot → position = 0 (assumed bottom)
✔ mm → encoder counts
✔ Simple position control
✔ Manual override
✔ No false stall issues

-----------------------------------------------------------
📌 HOW TO USE
-----------------------------------------------------------

1. Manually bring lift to BOTTOM
2. Reset ESP32 → position = 0

-----------------------------------------------------------
COMMANDS
-----------------------------------------------------------
L120 → move to 120 mm
U    → move up (manual)
D    → move down (manual)
S    → stop
P    → print position

===========================================================
*/

#include <Arduino.h>

// ---------------- PIN CONFIG ----------------
#define PWM_PIN 5
#define DIR_PIN 4

#define ENC_A 15
#define ENC_B 16

// ---------------- SYSTEM PARAMETERS ----------------
float counts_per_mm = 4.66;   // 🔧 CALIBRATE THIS
long max_counts = 1397;      // 🔧 MAX LIMIT

int pwmSpeed = 180;
int slowSpeed = 100;

// ---------------- VARIABLES ----------------
volatile long encoderCount = 0;

long targetPosition = 0;
bool moving = false;

// ---------------- ENCODER ISR ----------------
void IRAM_ATTR handleEncoder() {
  int b = digitalRead(ENC_B);
  if (b == HIGH)
    encoderCount++;
  else
    encoderCount--;
}

// ---------------- MOTOR CONTROL ----------------
void motorStop() {
  ledcWrite(PWM_PIN, 0);
  moving = false;
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

  long target = height_mm * counts_per_mm;

  if (target < 0 || target > max_counts) {
    Serial.println("ERROR OUT_OF_RANGE");
    return;
  }

  targetPosition = target;
  moving = true;

  Serial.print("MOVING TO: ");
  Serial.print(height_mm);
  Serial.println(" mm");
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

  encoderCount = 0;  // assume bottom

  Serial.println("LIFT READY (ZERO = CURRENT POSITION)");
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
      Serial.println("STOPPED");
    }

    else if (cmd == "P") {
      Serial.print("POS: ");
      Serial.println(encoderCount);
    }

    // Manual override
    else if (cmd == "U") {
      moving = false;
      motorUp(pwmSpeed);
    }

    else if (cmd == "D") {
      moving = false;
      motorDown(pwmSpeed);
    }
  }

  // -------- POSITION CONTROL --------
  if (moving) {

    long error = targetPosition - encoderCount;

    if (abs(error) < 5) {
      motorStop();
      Serial.println("DONE");
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