#include <Arduino.h>

void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.println("ESP32-S3 running");
  delay(1000);
}