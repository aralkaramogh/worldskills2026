#include <Arduino.h>

const int servoPin = 18;

void setup() {

  ledcSetup(0, 50, 16);      // channel, frequency, resolution
  ledcAttachPin(servoPin, 0);

}

void loop() {

  // 0 deg
  ledcWrite(0, 1638); 
  delay(2000);

  // center
  ledcWrite(0, 4915);
  delay(2000);

  // other side
  ledcWrite(0, 8192);
  delay(2000);

}