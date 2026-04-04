/*
===========================================================
ESP32 LIFT CONTROL SYSTEM (UPDATED FOR ESP32 CORE v3.x)
===========================================================

FEATURES:
✔ Homing using bottom limit switch
✔ Encoder-based position control
✔ Stall detection (no current sensor)
✔ Serial testing interface (no ROS2 needed)
✔ Compatible with ESP32-S3 (NEW PWM API)

-----------------------------------------------------------
📌 HOW TO TEST (VERY IMPORTANT)
-----------------------------------------------------------

1. Upload this code
2. Open Serial Monitor (115200 baud, newline ending)

---------------- ENCODER TEST ----------------
Rotate shaft manually → type:
p
EXPECT: position changes

---------------- MOTOR TEST ----------------
u → move UP
d → move DOWN
s → stop

If direction wrong → swap motor wires OR invert DIR logic

---------------- HOMING TEST ----------------
h
EXPECT:
→ moves DOWN
→ stops at switch
→ prints: POSITION = 0

---------------- POSITION TEST ----------------
m 500
EXPECT:
→ goes to position
→ slows near target
→ stops

---------------- STALL TEST ----------------
Hold lift while moving
EXPECT:
→ "STALL DETECTED!"
→ motor stops

-----------------------------------------------------------
⚙️ TUNING
-----------------------------------------------------------
stallTime     → increase if false stall (500 → 800)
pwmSpeed      → reduce if too aggressive
homingSpeed   → reduce if hitting hard

===========================================================
*/

#include <Arduino.h>

// ---------------- PIN CONFIG ----------------
#define PWM_PIN 4
#define DIR_PIN 5

#define ENC_A 17
#define ENC_B 18

#define LIMIT_BOTTOM 15  // ACTIVE LOW

// ---------------- VARIABLES ----------------
volatile long encoderCount = 0;

long targetPosition = 0;
bool moving = false;
bool directionUp = true;

// Speed settings
int pwmSpeed = 150;
int slowSpeed = 100;
int homingSpeed = 100;

// Stall detection
long lastEncoderCount = 0;
unsigned long lastCheckTime = 0;
const int stallTime = 500;

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
  directionUp = true;
}

void motorDown(int speed) {
  digitalWrite(DIR_PIN, LOW);
  ledcWrite(PWM_PIN, speed);
  moving = true;
  directionUp = false;
}

// ---------------- HOMING ----------------
void homeLift() {
  Serial.println("HOMING...");

  motorDown(homingSpeed);

  unsigned long startTime = millis();

  while (digitalRead(LIMIT_BOTTOM) == HIGH) {

    if (millis() - startTime > 5000) {
      Serial.println("HOMING FAILED (timeout)");
      motorStop();
      return;
    }
  }

  motorStop();
  encoderCount = 0;

  Serial.println("HOMING DONE → POSITION = 0");
}

// ---------------- MOVE TO POSITION ----------------
void moveToPosition(long target) {
  targetPosition = target;
  moving = true;
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(DIR_PIN, OUTPUT);
  pinMode(LIMIT_BOTTOM, INPUT_PULLUP);

  pinMode(ENC_A, INPUT);
  pinMode(ENC_B, INPUT);

  attachInterrupt(digitalPinToInterrupt(ENC_A), handleEncoder, RISING);

  // NEW PWM API (ESP32 core v3.x)
  ledcAttach(PWM_PIN, 5000, 8);

  Serial.println("SYSTEM READY");
}

// ---------------- LOOP ----------------
void loop() {

  // -------- SERIAL COMMANDS --------
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "h") {
      homeLift();
    }
    else if (cmd.startsWith("m")) {
      long val = cmd.substring(1).toInt();
      moveToPosition(val);

      Serial.print("TARGET SET: ");
      Serial.println(val);
    }
    else if (cmd == "u") {
      motorUp(pwmSpeed);
    }
    else if (cmd == "d") {
      motorDown(pwmSpeed);
    }
    else if (cmd == "s") {
      motorStop();
    }
    else if (cmd == "p") {
      Serial.print("POSITION: ");
      Serial.println(encoderCount);
    }
  }

  // -------- POSITION CONTROL --------
  if (moving) {
    long error = targetPosition - encoderCount;

    if (abs(error) < 5) {
      motorStop();
      Serial.println("TARGET REACHED");
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

  // -------- STALL DETECTION --------
  if (moving) {
    if (millis() - lastCheckTime > stallTime) {

      if (abs(encoderCount - lastEncoderCount) < 2) {
        Serial.println("STALL DETECTED!");
        motorStop();
      }

      lastEncoderCount = encoderCount;
      lastCheckTime = millis();
    }
  }
}