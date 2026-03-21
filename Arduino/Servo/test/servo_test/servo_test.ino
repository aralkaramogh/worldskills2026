#include <Arduino.h>
#include <ESP32Servo.h>

Servo servo1;

int pos = 0;

void setup() {
  servo1.setPeriodHertz(50);            // standard 50 Hz servo
  servo1.attach(7, 500, 2500);         // GPIO 7
}

void loop() {

  for (pos = 0; pos <= 270; pos += 1) {   // your servo supports ~270°
    servo1.write(pos);
    delay(15);
  }

  for (pos = 270; pos >= 0; pos -= 1) {
    servo1.write(pos);
    delay(15);
  }

}