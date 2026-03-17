/*
a → decrease angle (close more)
d → increase angle (open slightly)
s → save & print current position (use as closePos)
o → go to open position
c -> close for the saved posn
*/
#include <ESP32Servo.h>

Servo servo1;

const int servoPin = 14;

int openPos = 178;
int pos = 100;   // start somewhere safe
int setPos;
void setup() {
  Serial.begin(115200);
  servo1.attach(servoPin, 500, 2500);

  servo1.write(pos);

  Serial.println("=== GRIPPER CALIBRATION ===");
  Serial.println("a → close more");
  Serial.println("d → open more");
  Serial.println("s → save position");
  Serial.println("o → go to open");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    // CLOSE MORE
    if (cmd == 'l') {
      pos -= 1;
      if (pos < 0) pos = 0;

      servo1.write(pos);
      Serial.print("Pos: ");
      Serial.println(pos);
    }

    // OPEN MORE
    if (cmd == 'd') {
      pos += 1;
      if (pos > 180) pos = 180;

      servo1.write(pos);
      Serial.print("Pos: ");
      Serial.println(pos);
    }

    // SAVE VALUE
    if (cmd == 's') {
      Serial.print("SET THIS AS closePos = ");
      setPos = pos;
      Serial.println(pos);o

    }

    // GO OPEN
    if (cmd == 'o') {
      servo1.write(openPos);
      pos = openPos;
      Serial.println("Moved to OPEN");
    }
     // GO OPEN
    if (cmd == 'c') {
      servo1.write(pos);
      pos = setPos;
      Serial.println("Moved to set_close");
    }
  }
}