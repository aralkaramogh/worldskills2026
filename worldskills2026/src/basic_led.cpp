/**
 * Basic LED Flasher for ESP32
 * Controlled via Serial (USB) from Raspberry Pi
 *
 * Commands:
 * '1' - Turn LED ON
 * '0' - Turn LED OFF
 * 'f' - Flash LED (blink 3 times)
 * '?' - Status
 */

#include <Arduino.h>

const int LED_PIN = 2;  // Built-in LED on ESP32
const long SERIAL_BAUD = 115200;

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("Basic LED Flasher Ready");
  Serial.println("Commands: 1=ON, 0=OFF, f=FLASH, ?=STATUS");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    switch (cmd) {
      case '1':
        digitalWrite(LED_PIN, HIGH);
        Serial.println("LED ON");
        break;

      case '0':
        digitalWrite(LED_PIN, LOW);
        Serial.println("LED OFF");
        break;

      case 'f':
      case 'F':
        // Flash 3 times
        for (int i = 0; i < 3; i++) {
          digitalWrite(LED_PIN, HIGH);
          delay(200);
          digitalWrite(LED_PIN, LOW);
          delay(200);
        }
        Serial.println("LED FLASHED");
        break;

      case '?':
        Serial.print("LED Status: ");
        Serial.println(digitalRead(LED_PIN) ? "ON" : "OFF");
        break;

      default:
        Serial.println("Unknown command. Use 1, 0, f, ?");
        break;
    }
  }
}