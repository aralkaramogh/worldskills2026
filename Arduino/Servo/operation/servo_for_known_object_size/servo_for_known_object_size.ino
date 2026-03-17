/*
micro-relase

o = open
c = close

code initially opens the gripper (openPos)
set the close position (closePos) - 2 using the calibration code

+2 is for releaseOffset used for micro-release

*/

#include <ESP32Servo.h>

Servo servo1;

const int servoPin = 14;

int openPos  = 178;
int closePos = 130;   // 👈 set based on object size ("" - 2)
int releaseOffset = 2;

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
      Serial.println("CLOSE (fixed)");

      servo1.attach(servoPin, 500, 2500);

      // Direct close
      servo1.write(closePos);
      delay(300);

      // 🔥 Micro-release
      servo1.write(closePos + releaseOffset);

      Serial.println("GRIPPED");
    }
  }
}