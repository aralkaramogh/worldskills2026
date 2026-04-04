/*
===========================================================
ESP32 LIFT CALIBRATION TOOL
===========================================================

🎯 PURPOSE:
→ Measure encoder counts for full lift travel
→ Calculate counts_per_mm

-----------------------------------------------------------
📌 HOW TO USE
-----------------------------------------------------------

STEP 1:
Manually bring lift to BOTTOM

STEP 2:
Reset ESP32
→ encoderCount = 0

STEP 3:
Use commands:

U → move UP
D → move DOWN
S → STOP
P → print encoder count

STEP 4:
Move to TOP → STOP → press P

Example:
POS: 1397

STEP 5:
Measure actual height (mm)

STEP 6:
Calculate:
counts_per_mm = total_counts / height_mm

-----------------------------------------------------------
⚠️ NOTES
-----------------------------------------------------------
✔ No limits → be careful at top/bottom
✔ Keep finger ready on STOP (S)
✔ Use slow speed near ends

===========================================================
*/

#include <Arduino.h>

// ---------------- PIN CONFIG ----------------
#define PWM_PIN 7
#define DIR_PIN 6

#define ENC_A 18
#define ENC_B 17

// ---------------- VARIABLES ----------------
volatile long encoderCount = 0;

int pwmSpeed = 150;

// ---------------- ENCODER ISR ----------------
void IRAM_ATTR handleEncoder() {
  int b = digitalRead(ENC_B);
  if (b == HIGH)
    encoderCount--;
  else
    encoderCount++;
}

// ---------------- MOTOR CONTROL ----------------
void motorStop() {
  ledcWrite(PWM_PIN, 0);
  Serial.println("STOPPED");
}

void motorUp() {
  digitalWrite(DIR_PIN, LOW);
  ledcWrite(PWM_PIN, pwmSpeed);
  Serial.println("MOVING UP");
}

void motorDown() {
  digitalWrite(DIR_PIN, HIGH);
  ledcWrite(PWM_PIN, pwmSpeed);
  Serial.println("MOVING DOWN");
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

  encoderCount = 0;

  Serial.println("=== LIFT CALIBRATION MODE ===");
  Serial.println("ZERO SET (CURRENT POSITION)");
  Serial.println("Commands: U / D / S / P");
}

// ---------------- LOOP ----------------
void loop() {

  // -------- SERIAL COMMANDS --------
  if (Serial.available()) {

    char cmd = Serial.read();

    if (cmd == 'U' || cmd == 'u') {
      motorUp();
    }

    else if (cmd == 'D' || cmd == 'd') {
      motorDown();
    }

    else if (cmd == 'S' || cmd == 's') {
      motorStop();
    }

    else if (cmd == 'P' || cmd == 'p') {
      Serial.print("POS: ");
      Serial.println(encoderCount);
    }
  }

  // -------- LIVE FEEDBACK (optional) --------
  static unsigned long lastPrint = 0;

  if (millis() - lastPrint > 500) {
    Serial.print("POS: ");
    Serial.println(encoderCount);
    lastPrint = millis();
  }
}