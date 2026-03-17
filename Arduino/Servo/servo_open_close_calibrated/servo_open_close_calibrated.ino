#include <ESP32Servo.h>

Servo servo1;

// 🔧 Calibrated values (change if needed)
int openPos = 175;   
int closePos = 10; 

void setup() {
  Serial.begin(115200);

  servo1.setPeriodHertz(50);
  servo1.attach(7, 500, 2500);

  servo1.write(openPos);

  Serial.println("Gripper Ready");
  Serial.println("Commands: open / close");
}

void loop() {

  if (Serial.available()) {

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "open") {
      servo1.write(openPos);
      Serial.println("Gripper OPEN");
    }

    else if (cmd == "close") {
      servo1.write(closePos);
      Serial.println("Gripper CLOSE");
    }
  }
}