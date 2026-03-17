
//Adjust the maxStepsWithoutProgress as per the friction obtained at the gear mechanism

#include <ESP32Servo.h>

Servo servo1;

const int servoPin = 14;

int openPos  = 178;
int minClosePos = 10;

int stepSize = 2;
int stepDelay = 40;

int releaseOffset = 2;

// pseudo "stall detection"
int maxStepsWithoutProgress = 85;

void setup() {
  Serial.begin(115200);
  servo1.attach(servoPin, 500, 2500);

  servo1.write(openPos);
  Serial.println("Ready");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    if (cmd == 'o') {
      servo1.attach(servoPin, 500, 2500);
      servo1.write(openPos);
      Serial.println("OPEN");
    }

    if (cmd == 'c') {
      Serial.println("AUTO CLOSE START");

      servo1.attach(servoPin, 500, 2500);

      int pos = openPos;
      int stepCounter = 0;

      while (pos > minClosePos) {
        int nextPos = pos - stepSize;

        servo1.write(nextPos);
        delay(stepDelay);

        stepCounter++;

        // 🧠 Simulated detection
        if (stepCounter > maxStepsWithoutProgress) {
          Serial.println("OBJECT DETECTED");

          // 🔥 Micro-release
          servo1.write(nextPos + releaseOffset);
          break;
        }

        pos = nextPos;
      }

      Serial.println("DONE");
    }
  }
}