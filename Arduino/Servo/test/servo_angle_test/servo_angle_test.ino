#include <ESP32Servo.h>

Servo servo1;

int angle = 135;   // start near center (for 270° servo)

void setup() {
  Serial.begin(115200);

  servo1.setPeriodHertz(50);
  servo1.attach(7, 500, 2500);

  servo1.write(angle);

  Serial.println("Servo Position Tester");
  Serial.println("+ : increase angle");
  Serial.println("- : decrease angle");
  Serial.println("Enter number (0–270)");
  Serial.println("c : center");
}

void loop() {

  if (Serial.available()) {

    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input == "+") {
      angle += 5;
    }
    else if (input == "-") {
      angle -= 5;
    }
    else if (input == "c") {
      angle = 135;
    }
    else {
      int val = input.toInt();
      if (val >= 0 && val <= 270) {
        angle = val;
      } else {
        Serial.println("Invalid input");
        return;
      }
    }

    angle = constrain(angle, 0, 270);

    servo1.write(angle);

    Serial.print("Angle: ");
    Serial.println(angle);
  }
}