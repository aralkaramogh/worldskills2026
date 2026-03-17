#include <Arduino.h>

const int servoPin = 18;

int pulse = 1500;      // start at center
const int stepSmall = 20;
const int stepBig = 100;

void setServoPulse(int pulse_us) {
  pulse_us = constrain(pulse_us, 500, 2500);
  uint32_t duty = (pulse_us * 65535) / 20000;
  ledcWrite(0, duty);
}

void setup() {
  Serial.begin(115200);

  ledcSetup(0, 50, 16);
  ledcAttachPin(servoPin, 0);

  setServoPulse(pulse);

  Serial.println("Servo Tester Ready");
  Serial.println("O = small left");
  Serial.println("P = small right");
  Serial.println("K = big left");
  Serial.println("L = big right");
  Serial.println("Space = center");
}

void loop() {

  if (Serial.available()) {
    char cmd = Serial.read();

    if (cmd == 'o' || cmd == 'O') {
      pulse -= stepSmall;
    }

    if (cmd == 'p' || cmd == 'P') {
      pulse += stepSmall;
    }

    if (cmd == 'k' || cmd == 'K') {
      pulse -= stepBig;
    }

    if (cmd == 'l' || cmd == 'L') {
      pulse += stepBig;
    }

    if (cmd == ' ') {
      pulse = 1500;
    }

    pulse = constrain(pulse, 500, 2500);
    setServoPulse(pulse);

    Serial.print("Pulse: ");
    Serial.println(pulse);
  }

}