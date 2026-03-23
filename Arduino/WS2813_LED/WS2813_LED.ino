/*
 * ============================================================
 * WS2813 LED CONTROL - ESP32 S3 (ROS READY VERSION)
 * ============================================================
 *
 * PURPOSE:
 * - Control WS2813 LED strip
 * - Easily integrate with micro-ROS later
 * - Accept flexible RGB values (any color)
 *
 * HARDWARE:
 *  DATA -> GPIO 8
 *  VCC  -> 5V external
 *  GND  -> Common with ESP32
 *
 * ============================================================
 * ROS2 INTEGRATION PLAN (IMPORTANT)
 * ============================================================
 *
 * CURRENT:
 *  - Using Serial input for testing
 *
 * LATER (micro-ROS):
 *  1. Replace Serial input with ROS2 subscriber
 *  2. Subscribe to topic: /led_cmd
 *
 * MESSAGE OPTIONS:
 *
 *  Option A (Recommended):
 *    std_msgs/Int32MultiArray
 *    Example:
 *      [255, 0, 0]   -> RED
 *      [0, 255, 0]   -> GREEN
 *      [0, 0, 255]   -> BLUE
 *      [255, 255, 0] -> YELLOW
 *
 *  Option B (Advanced):
 *    Custom msg:
 *      uint8 r
 *      uint8 g
 *      uint8 b
 *      uint8 mode   // blink, solid, etc.
 *
 * ============================================================
 */

#include <Adafruit_NeoPixel.h>

#define LED_PIN     21
#define NUM_LEDS    10   // Change as needed

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);

  strip.begin();
  strip.show();   // Turn OFF all LEDs initially
}

// ============================================================
// CORE FUNCTION (IMPORTANT)
// ============================================================
//
// This is the ONLY function your ROS callback will use later
//
void setColor(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

// Turn OFF LEDs
void clearLEDs() {
  strip.clear();
  strip.show();
}

// ============================================================
// LOOP (TESTING MODE ONLY)
// ============================================================
//
// Replace this section with micro-ROS subscriber later
//
void loop() {

  if (Serial.available()) {

    String cmd = Serial.readStringUntil('\n');

    // --------------------------------------------------------
    // METHOD 1: PREDEFINED COLORS
    // --------------------------------------------------------
    if (cmd == "RED") {
      setColor(255, 0, 0);
    }
    else if (cmd == "GREEN") {
      setColor(0, 255, 0);
    }
    else if (cmd == "BLUE") {
      setColor(0, 0, 255);
    }
    else if (cmd == "WHITE") {
      setColor(255, 255, 255);
    }
    else if (cmd == "OFF") {
      clearLEDs();
    }

    // --------------------------------------------------------
    // METHOD 2: CUSTOM RGB INPUT  (IMPORTANT)
    // Format:
    //   R,G,B
    // Example:
    //   255,100,0   -> ORANGE
    //   255,0,255   -> MAGENTA
    //   0,255,255   -> CYAN
    // --------------------------------------------------------
    else {

      int r, g, b;

      // Parse input string
      if (sscanf(cmd.c_str(), "%d,%d,%d", &r, &g, &b) == 3) {

        // Limit values between 0–255
        r = constrain(r, 0, 255);
        g = constrain(g, 0, 255);
        b = constrain(b, 0, 255);

        setColor(r, g, b);
      }
    }
  }
}

/*
 * ============================================================
 * MICRO-ROS INTEGRATION (DROP-IN LOGIC)
 * ============================================================
 *
 * Replace loop() with ROS callback like this:
 *
 * void led_callback(const void * msgin) {
 *
 *   const std_msgs__msg__Int32MultiArray * msg =
 *       (const std_msgs__msg__Int32MultiArray *)msgin;
 *
 *   int r = msg->data.data[0];
 *   int g = msg->data.data[1];
 *   int b = msg->data.data[2];
 *
 *   setColor(r, g, b);
 * }
 *
 * ============================================================
 */