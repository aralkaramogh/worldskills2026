/*
 * ESP32-S3 PID Motor Velocity Controller with Encoder Testing Mode
 * Motor: REV-41-1291 HD Hex Motor with 20:1 Gearbox
 * Platform: PlatformIO (ESP32-S3)
 * 
 * ╔════════════════════════════════════════════════════════════════╗
 * ║                    HOW TO USE THIS CODE                        ║
 * ╚════════════════════════════════════════════════════════════════╝
 * 
 * QUICK START:
 * 1. Upload code to ESP32-S3
 * 2. Open Serial Monitor (115200 baud)
 * 3. Type: test=on
 * 4. Type: test=run
 * 5. Watch automatic verification (0, 128, 255 PWM)
 * 6. Type: test=off
 * 7. Type: Kp=0.8
 * 8. Type: v=100
 * 9. Open Serial Plotter (Ctrl+Shift+L) to watch tuning
 * 
 * ╔════════════════════════════════════════════════════════════════╗
 * ║                   SERIAL MONITOR COMMANDS                      ║
 * ╚════════════════════════════════════════════════════════════════╝
 * 
 * TESTING MODE (verify encoder BEFORE tuning):
 *   test=on              → Enable testing (disables PID)
 *   test=off             → Disable testing (enables PID)
 *   test=run             → Automatic test at PWM 0, 128, 255
 *   test=pwm=128         → Manual PWM control (0-255)
 *   test=status          → Show test results
 * 
 * PID TUNING (after test=off):
 *   Kp=0.8               → Set proportional gain (0.1-2.0)
 *   Ki=0.15              → Set integral gain (0.0-0.5)
 *   Kd=0.12              → Set derivative gain (0.0-0.3)
 *   v=100                → Set desired RPM (0-300)
 * 
 * MONITORING:
 *   status               → Show all parameters and speeds
 *   stop                 → Stop motor (disable testing)
 *   help                 → Show all commands
 * 
 * ╔════════════════════════════════════════════════════════════════╗
 * ║                     RECOMMENDED WORKFLOW                       ║
 * ╚════════════════════════════════════════════════════════════════╝
 * 
 * PHASE 1: ENCODER VERIFICATION (5 minutes)
 *   test=on
 *   test=run
 *   → Wait for test to complete
 *   → Check if speeds look reasonable:
 *       PWM=0:   Should be ~0 RPM
 *       PWM=128: Should be ~150 RPM (50% of max)
 *       PWM=255: Should be ~300 RPM (max)
 *   test=off
 * 
 * PHASE 2: PID TUNING (10-15 minutes)
 *   Kp=0.8      (start with default)
 *   Ki=0.15
 *   Kd=0.12
 *   v=100       (set target speed)
 * 
 *   → Open Serial Plotter (Ctrl+Shift+L)
 *   → Observe three lines:
 *       BLUE:  Your target velocity
 *       RED:   Actual velocity (should follow blue)
 *       GREEN: Motor power (0-255)
 * 
 *   → If too slow: Increase Kp (try 1.0, 1.2, 1.5)
 *   → If oscillates: Increase Kd (try 0.15, 0.20)
 *   → If can't reach: Increase Ki (try 0.20)
 * 
 *   → Test at different speeds: v=50, v=100, v=150, v=200
 *   → Tune until blue and red lines closely match
 * 
 * ╔════════════════════════════════════════════════════════════════╗
 * ║                    PIN CONFIGURATION                           ║
 * ╚════════════════════════════════════════════════════════════════╝
 * 
 * Encoder:
 *   Phase A → GPIO 16 (with internal pull-up)
 *   Phase B → GPIO 15 (with internal pull-up)
 *   GND     → GND
 * 
 * Motor:
 *   PWM Dir → GPIO 5  (0-255, motor power)
 *   Dir Pin → GPIO 4  (HIGH=forward, LOW=reverse)
 *   GND     → GND
 *   12V     → Power
 * 
 * ╔════════════════════════════════════════════════════════════════╗
 * ║                  DEFAULT STARTING VALUES                       ║
 * ╚════════════════════════════════════════════════════════════════╝
 * 
 * Kp = 0.8   (proportional - response speed)
 * Ki = 0.0   (integral - steady-state error)
 * Kd = 0.0   (derivative - smoothness/damping)
 * 
 * These are optimized for REV motor with 20:1 gearbox.
 * Adjust based on Serial Plotter observations.
 * 
 * ╔════════════════════════════════════════════════════════════════╗
 * ║                   UNDERSTANDING OUTPUT                         ║
 * ╚════════════════════════════════════════════════════════════════╝
 * 
 * Serial Monitor (status):
 *   Kp: 0.8000        → Your PID gains
 *   Desired Velocity: 100.0 RPM  → What you asked for
 *   Current Velocity: 98.5 RPM   → What you're getting
 *   PID Error: 1.5 RPM           → Difference
 *   PID Output: 110 (PWM)        → Motor power
 * 
 * Serial Plotter (three lines):
 *   BLUE  = Desired Velocity (your setpoint)
 *   RED   = Current Velocity (encoder reading)
 *   GREEN = PID Output (motor power 0-255)
 * 
 * Good tuning:
 *   → RED line closely follows BLUE line
 *   → Reaches setpoint in 1-2 seconds
 *   → No oscillation
 *   → GREEN decreases as it gets close
 * 
 * ╔════════════════════════════════════════════════════════════════╗
 * ║                    TROUBLESHOOTING                             ║
 * ╚════════════════════════════════════════════════════════════════╝
 * 
 * Issue: test=run shows speed=0 at all PWM levels
 *   → Check: Encoder connected (GPIO 16, 15)?
 *   → Check: Encoder cable secure?
 *   → Try: Manually rotate motor, send 'status'
 *   → Look: Encoder Count should change
 * 
 * Issue: Motor won't move even at PWM=255
 *   → Check: Motor has 12V power?
 *   → Check: Motor can spin freely by hand?
 *   → Check: Direction pin (GPIO 4) connected?
 * 
 * Issue: Serial Plotter shows garbage
 *   → Check: Baud rate = 115200 (top right dropdown)
 *   → Close and reopen Serial Plotter
 *   → Try: Unplug USB and replug
 * 
 * Issue: Motor oscillates (bounces above/below target)
 *   → Send: Kd=0.15 (increase damping)
 *   → If still oscillating: Kd=0.20
 *   → If too sluggish: Reduce Kd or Kp
 * 
 * Issue: Motor too slow to reach target
 *   → Send: Kp=1.0 (increase from 0.8)
 *   → If still slow: Kp=1.2
 *   → If oscillates: Increase Kd to smooth it
 * 
 * For more detailed help, send: help
 */

#include <Arduino.h>
#include <math.h>

// ==================== PIN CONFIGURATION ====================
#define ENCODER_A 16        // Quadrature Phase A (encoder input)
#define ENCODER_B 15        // Quadrature Phase B (encoder input)
#define MOTOR_PWM 5         // PWM output to motor driver (controls speed: 0-255)
#define MOTOR_DIR 4         // Motor direction pin (HIGH=forward, LOW=reverse)

// ==================== MOTOR SPECIFICATIONS ====================
// REV HD Hex Motor (REV-41-1291) with 20:1 Gearbox
// Base motor speed: 6000 RPM (no load)
// Output speed with 20:1: 300 RPM max
// Encoder: 28 counts/revolution at motor
// Effective encoder resolution at output: 28 * 20 = 560 counts/revolution

// ==================== PID PARAMETERS (TUNE THESE) ====================
// Recommended starting values for REV motor with moderate load
// 
// HOW TO TUNE (while watching Serial Plotter with Ctrl+Shift+L):
//   1. Start with Kp=0.8, Ki=0.0, Kd=0.0
//   2. Set v=100 to test at medium speed
//   3. Open Serial Plotter
//   4. Observe RED line (actual speed) vs BLUE line (desired speed)
//   5. Adjust using these rules:
//      - RED too slow to reach BLUE? Increase Kp (try 1.0, 1.2, 1.5)
//      - RED overshoots/oscillates? Increase Kd (try 0.1, 0.15, 0.2)
//      - RED doesn't reach BLUE (gap remains)? Increase Ki (try 0.1, 0.2)
//   6. Test at different speeds (v=50, v=100, v=150, v=200)
//   7. Done when RED follows BLUE smoothly without oscillation
//
// Typical final values: Kp=0.8-1.2, Ki=0.1-0.2, Kd=0.1-0.2
float Kp = 0.8;             // Proportional gain - increase if response is slow
float Ki = 0.0;             // Integral gain - start at 0, increase to fix steady-state error
float Kd = 0.0;             // Derivative gain - start at 0, increase if oscillating

float desiredVelocity = 150.0;  // RPM at output shaft (0-300 RPM max with 20:1 ratio)

// ==================== ENCODER CONFIGURATION ====================
#define MOTOR_PPR 28                    // Motor encoder: 28 counts/revolution
#define GEAR_RATIO 20                   // Gearbox reduction ratio: 20:1
#define OUTPUT_PPR (MOTOR_PPR * GEAR_RATIO)  // Effective PPR at output: 560
#define VELOCITY_SAMPLES 20             // Number of samples to average velocity

volatile long encoderCount = 0;
volatile long lastEncoderCount = 0;
long previousEncoderCount = 0;

float currentVelocity = 0.0;
float velocityArray[VELOCITY_SAMPLES] = {0};
int velocityIndex = 0;

// ==================== PID VARIABLES ====================
float pidError = 0;
float pidPrevError = 0;
float pidIntegral = 0;
float pidOutput = 0;
float pidOutputMax = 255;    // Max PWM value
float pidOutputMin = -255;   // Min PWM value (for reverse)

unsigned long lastTime = 0;
unsigned long currentTime = 0;
float deltaTime = 0;

// ==================== TIMING ====================
unsigned long lastVelocityCalcTime = 0;
unsigned long velocityCalcInterval = 100;  // Calculate velocity every 100ms

// ==================== SERIAL OUTPUT ====================
unsigned long lastSerialTime = 0;
unsigned long serialInterval = 50;  // Send to serial plotter every 50ms

// ==================== TESTING MODE ====================
bool testingMode = false;           // Toggle between testing and PID mode
int testPWM = 0;                    // Fixed PWM value for testing (0, 128, 255)
float maxSpeedAt255 = 0.0;          // Will store max speed at PWM=255
float maxSpeedAt128 = 0.0;          // Will store speed at PWM=128
float speedAt0 = 0.0;               // Will store speed at PWM=0

// ==================== FUNCTION DECLARATIONS ====================
void encoderInterrupt();
void calculateVelocity();
void updatePID();
void applyMotorControl();
void stopMotor();
void sendSerialPlotterData();
void checkSerialInput();
void printHelp();
void applyTestPWM();
void runEncoderTest();
void displayTestResults();

// ==================== SETUP ====================
void setup() {
  // Serial communication
  Serial.begin(115200);  // High baud rate for ESP32
  delay(1000);
  
  Serial.println("\n=== ESP32-S3 PID Motor Controller ===");
  Serial.println("Motor: REV HD Hex Motor (REV-41-1291)");
  Serial.println("Gearbox: 20:1 Reduction");
  Serial.println("Max Output RPM: 300");
  Serial.println("Encoder: 28 CPR (560 CPR effective at output)");
  Serial.println("");
  Serial.println("Pin Configuration:");
  Serial.println("  Encoder A: GPIO " + String(ENCODER_A));
  Serial.println("  Encoder B: GPIO " + String(ENCODER_B));
  Serial.println("  Motor PWM: GPIO " + String(MOTOR_PWM));
  Serial.println("  Motor Dir: GPIO " + String(MOTOR_DIR));
  
  // Pin configuration
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  pinMode(MOTOR_PWM, OUTPUT);
  pinMode(MOTOR_DIR, OUTPUT);
  
  // Attach interrupts for quadrature encoder
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), encoderInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B), encoderInterrupt, CHANGE);
  
  // Motor PWM frequency (25kHz is good for most motors)
  ledcSetup(0, 25000, 8);  // Channel 0, 25kHz frequency, 8-bit resolution
  ledcAttachPin(MOTOR_PWM, 0);
  
  // Stop motor initially
  stopMotor();
  
  lastTime = millis();
  lastVelocityCalcTime = millis();
  lastSerialTime = millis();
  
  delay(2000);
  Serial.println("\n✓ System Ready!");
  Serial.println("Velocity is displayed in RPM at the output shaft");
  Serial.println("Maximum safe velocity: 300 RPM");
  Serial.println("Recommended test velocity: 100-200 RPM");
  Serial.println("\n🧪 ENCODER VERIFICATION AVAILABLE:");
  Serial.println("  Command: test=on      (Enable testing mode)");
  Serial.println("  Command: test=run     (Run automatic verification)");
  Serial.println("  Then:    test=off     (Return to PID mode)");
  Serial.println("\n📈 TUNING WORKFLOW:");
  Serial.println("  1. Verify encoder with test=run");
  Serial.println("  2. Set Kp=0.8, Ki=0.15, Kd=0.12");
  Serial.println("  3. Set v=100 and open Serial Plotter");
  Serial.println("  4. Adjust parameters until satisfied");
  Serial.println("\nType 'help' for full command list\n");
}

// ==================== MAIN LOOP ====================
void loop() {
  currentTime = millis();
  deltaTime = (currentTime - lastTime) / 1000.0;  // Convert to seconds
  
  if (deltaTime > 0.05) {  // Update every 50ms minimum
    lastTime = currentTime;
    
    // Calculate velocity every 100ms
    if (currentTime - lastVelocityCalcTime >= velocityCalcInterval) {
      calculateVelocity();
      lastVelocityCalcTime = currentTime;
    }
    
    // Either run testing mode OR PID mode (not both)
    // testingMode flag controls which one is active
    if (testingMode) {
      // TESTING MODE: Apply fixed PWM (no feedback control)
      // Use this to verify encoder readings
      // Commands: test=on, test=pwm=VALUE, test=run
      // Cannot adjust Kp/Ki/Kd in this mode
      applyTestPWM();
    } else {
      // PID MODE: Normal controller (feedback control)
      // Use this for tuning with Serial Plotter
      // Commands: Kp=VALUE, Ki=VALUE, Kd=VALUE, v=VALUE
      // This is where you tune the motor
      updatePID();
      applyMotorControl();
    }
    
    // Send data to Serial Plotter
    if (currentTime - lastSerialTime >= serialInterval) {
      sendSerialPlotterData();
      lastSerialTime = currentTime;
    }
  }
  
  // Check for serial input to adjust parameters
  checkSerialInput();
}

// ==================== ENCODER INTERRUPT HANDLER ====================
void encoderInterrupt() {
  // Read current state
  bool aState = digitalRead(ENCODER_A);
  bool bState = digitalRead(ENCODER_B);
  
  // Quadrature decoding (2x resolution)
  static bool lastAState = false;
  static bool lastBState = false;
  
  if (aState != lastAState) {
    if (aState == bState) {
      encoderCount++;  // Forward
    } else {
      encoderCount--;  // Reverse
    }
    lastAState = aState;
  }
  
  if (bState != lastBState) {
    if (bState != aState) {
      encoderCount++;  // Forward
    } else {
      encoderCount--;  // Reverse
    }
    lastBState = bState;
  }
}

// ==================== VELOCITY CALCULATION ====================
void calculateVelocity() {
  // Get change in encoder count
  long deltaCount = encoderCount - previousEncoderCount;
  previousEncoderCount = encoderCount;
  
  // Calculate velocity in RPM at output shaft
  // Formula: (deltaCount / OUTPUT_PPR) * (60,000 / velocityCalcInterval) = RPM
  // deltaCount = encoder pulses in interval
  // OUTPUT_PPR = 560 (28 motor counts * 20 gear ratio)
  // 60,000 = 60 seconds * 1000 milliseconds
  // velocityCalcInterval = 100ms
  float revolutions = (float)deltaCount / OUTPUT_PPR;
  float rawVelocity = revolutions * (60000.0 / velocityCalcInterval);  // RPM at output shaft
  
  // Apply moving average filter for smooth velocity
  velocityArray[velocityIndex] = rawVelocity;
  velocityIndex = (velocityIndex + 1) % VELOCITY_SAMPLES;
  
  // Calculate average
  float sum = 0;
  for (int i = 0; i < VELOCITY_SAMPLES; i++) {
    sum += velocityArray[i];
  }
  currentVelocity = sum / VELOCITY_SAMPLES;
}

// ==================== PID CONTROLLER ====================
void updatePID() {
  // Calculate error
  pidError = desiredVelocity - currentVelocity;
  
  // Proportional term
  float pTerm = Kp * pidError;
  
  // Integral term (with anti-windup)
  pidIntegral += pidError * deltaTime;
  if (pidIntegral > pidOutputMax) pidIntegral = pidOutputMax;
  if (pidIntegral < pidOutputMin) pidIntegral = pidOutputMin;
  float iTerm = Ki * pidIntegral;
  
  // Derivative term
  float dTerm = 0;
  if (deltaTime > 0) {
    dTerm = Kd * (pidError - pidPrevError) / deltaTime;
  }
  pidPrevError = pidError;
  
  // Combine terms
  pidOutput = pTerm + iTerm + dTerm;
  
  // Constrain output
  pidOutput = constrain(pidOutput, pidOutputMin, pidOutputMax);
}

// ==================== MOTOR CONTROL ====================
void applyMotorControl() {
  if (pidOutput >= 0) {
    // Forward direction
    digitalWrite(MOTOR_DIR, HIGH);
    ledcWrite(0, (int)pidOutput);
  } else {
    // Reverse direction
    digitalWrite(MOTOR_DIR, LOW);
    ledcWrite(0, (int)(-pidOutput));
  }
}

void stopMotor() {
  digitalWrite(MOTOR_DIR, LOW);
  ledcWrite(0, 0);
}

// ==================== TESTING MODE FUNCTIONS ====================
void applyTestPWM() {
  // Apply FIXED PWM value (no feedback, no PID)
  // Use this to verify encoder works BEFORE PID tuning
  // Motor always goes forward at the PWM value you set
  // Examples:
  //   testPWM = 0:   Motor OFF, should read ~0 RPM
  //   testPWM = 128: Motor at half power, should read ~150 RPM
  //   testPWM = 255: Motor at full power, should read ~300 RPM
  // 
  // How to use:
  //   Send: test=on              (enable testing)
  //   Send: test=pwm=128         (set PWM to 128)
  //   Wait 3 seconds
  //   Send: status               (check current velocity)
  digitalWrite(MOTOR_DIR, HIGH);  // Always forward direction
  ledcWrite(0, testPWM);           // Apply the PWM value
}

void runEncoderTest() {
  // AUTOMATIC ENCODER VERIFICATION TEST
  // 
  // What it does:
  //   1. Tests motor at PWM=0   (3 seconds) → Should be ~0 RPM
  //   2. Tests motor at PWM=128 (3 seconds) → Should be ~150 RPM
  //   3. Tests motor at PWM=255 (3 seconds) → Should be ~300 RPM
  //   4. Analyzes results and gives recommendations
  // 
  // When to use:
  //   BEFORE starting PID tuning, to verify encoder works correctly
  // 
  // How to use:
  //   Send: test=on              (enable testing mode first)
  //   Send: test=run             (this function runs automatically)
  //   Wait ~30 seconds for all tests to complete
  //   Review the results
  //   Send: test=off             (return to PID mode)
  // 
  // Expected results:
  //   PWM=0:   0-5 RPM             ✓ Motor is OFF
  //   PWM=128: 100-200 RPM         ✓ Half power, reasonable
  //   PWM=255: 250-300 RPM         ✓ Full power, near max
  //
  // Bad signs to watch for:
  //   PWM=0:   >10 RPM             ✗ Motor spinning at no power?
  //   PWM=255: <100 RPM            ✗ Motor very slow
  //   PWM=255: >350 RPM            ✗ Motor too fast
  
  Serial.println("\n╔════════════════════════════════════════════════╗");
  Serial.println("║     ENCODER SPEED VERIFICATION TEST            ║");
  Serial.println("║   PID Controller: DISABLED                      ║");
  Serial.println("║   Motor will apply fixed PWM values             ║");
  Serial.println("╚════════════════════════════════════════════════╝");
  
  Serial.println("\n📊 Testing at different PWM levels...\n");
  
  // Clear velocity arrays
  for (int i = 0; i < VELOCITY_SAMPLES; i++) {
    velocityArray[i] = 0;
  }
  velocityIndex = 0;
  currentVelocity = 0;
  
  // Test 1: PWM = 0 (should be stationary)
  Serial.println("Test 1: PWM = 0 (Motor OFF)");
  Serial.println("  Waiting 3 seconds for motor to stabilize...");
  testPWM = 0;
  for (int i = 0; i < 30; i++) {
    delay(100);
    Serial.print(".");
  }
  speedAt0 = currentVelocity;
  Serial.println("\n  ✓ Speed at PWM=0: " + String(speedAt0, 2) + " RPM");
  Serial.println("  (Should be ~0 RPM)\n");
  
  // Test 2: PWM = 128 (half power)
  Serial.println("Test 2: PWM = 128 (Half Power)");
  Serial.println("  Waiting 3 seconds for motor to stabilize...");
  testPWM = 128;
  for (int i = 0; i < 30; i++) {
    delay(100);
    Serial.print(".");
  }
  maxSpeedAt128 = currentVelocity;
  Serial.println("\n  ✓ Speed at PWM=128: " + String(maxSpeedAt128, 2) + " RPM");
  Serial.println("  (Should be ~50% of max speed)\n");
  
  // Test 3: PWM = 255 (full power)
  Serial.println("Test 3: PWM = 255 (Full Power)");
  Serial.println("  Waiting 3 seconds for motor to stabilize...");
  testPWM = 255;
  for (int i = 0; i < 30; i++) {
    delay(100);
    Serial.print(".");
  }
  maxSpeedAt255 = currentVelocity;
  Serial.println("\n  ✓ Speed at PWM=255: " + String(maxSpeedAt255, 2) + " RPM");
  Serial.println("  (Should be close to 300 RPM max)\n");
  
  // Display results
  displayTestResults();
  
  // Stop motor
  stopMotor();
  Serial.println("Motor stopped. Ready for PID tuning!\n");
}

void displayTestResults() {
  // Display test results and recommendations
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║              TEST RESULTS SUMMARY              ║");
  Serial.println("╚════════════════════════════════════════════════╝");
  
  Serial.println("\n📈 Speed Readings:");
  Serial.println("  PWM =   0: " + String(speedAt0, 2) + " RPM");
  Serial.println("  PWM = 128: " + String(maxSpeedAt128, 2) + " RPM");
  Serial.println("  PWM = 255: " + String(maxSpeedAt255, 2) + " RPM");
  
  Serial.println("\n🔍 Analysis:");
  
  // Check PWM=0
  if (speedAt0 > 5) {
    Serial.println("  ⚠️  Warning: Motor spinning at PWM=0!");
    Serial.println("      Check: Motor may be damaged or encoder misconfigured");
  } else {
    Serial.println("  ✓ PWM=0: Motor is stationary");
  }
  
  // Check PWM=255
  if (maxSpeedAt255 < 100) {
    Serial.println("  ⚠️  Warning: Max speed very low (<100 RPM)");
    Serial.println("      Check: Encoder connections, mechanical issues, or power supply");
  } else if (maxSpeedAt255 > 350) {
    Serial.println("  ⚠️  Warning: Max speed very high (>350 RPM)");
    Serial.println("      Check: Encoder PPR settings or gear ratio");
  } else {
    Serial.println("  ✓ PWM=255: Speed " + String(maxSpeedAt255, 0) + " RPM (reasonable)");
  }
  
  // Check linearity
  if (maxSpeedAt128 > 0) {
    float ratio = maxSpeedAt128 / maxSpeedAt255;
    Serial.println("\n📊 Speed Linearity:");
    Serial.println("  PWM128/PWM255 Ratio: " + String(ratio, 2));
    if (ratio > 0.4 && ratio < 0.6) {
      Serial.println("  ✓ Good: Fairly linear response");
    } else if (ratio > 0.3 && ratio < 0.7) {
      Serial.println("  ⚠️  Acceptable: Some non-linearity (normal for DC motors)");
    } else {
      Serial.println("  ✗ Bad: Very non-linear response");
    }
  }
  
  Serial.println("\n💡 Recommendations:");
  Serial.println("  1. If speeds look reasonable, proceed to PID tuning");
  Serial.println("  2. Command: test=off   (to disable testing mode)");
  Serial.println("  3. Command: Kp=0.8    (start PID tuning)");
  Serial.println("  4. Command: v=100     (set desired RPM)");
  Serial.println("  5. Open Serial Plotter to watch tuning\n");
}

// ==================== SERIAL PLOTTER OUTPUT ====================
void sendSerialPlotterData() {
  // Format for Arduino Serial Plotter:
  // desiredVelocity,currentVelocity,pidOutput
  // This creates three independent plots
  
  Serial.print(desiredVelocity);
  Serial.print(",");
  Serial.print(currentVelocity);
  Serial.print(",");
  Serial.println(pidOutput);
}

// ==================== SERIAL MONITOR CONTROL ====================
void checkSerialInput() {
  // This function reads commands from Serial Monitor
  // 
  // TESTING MODE COMMANDS (for encoder verification):
  //   test=on              Enable testing (disables PID)
  //   test=off             Disable testing (enables PID)
  //   test=run             Run automatic test at PWM 0, 128, 255
  //   test=pwm=128         Set manual PWM (use any value 0-255)
  //   test=status          Show test results
  // 
  // PID TUNING COMMANDS (for motor control):
  //   Kp=0.8               Set proportional gain (default: 0.8)
  //   Ki=0.15              Set integral gain (default: 0.0)
  //   Kd=0.12              Set derivative gain (default: 0.0)
  //   v=100                Set desired velocity in RPM (default: 150)
  // 
  // MONITORING COMMANDS:
  //   status               Show all parameters and current speeds
  //   stop                 Stop motor (also disables testing)
  //   help                 Show all commands
  // 
  // WORKFLOW EXAMPLE:
  //   1. test=on           (enable testing)
  //   2. test=run          (run automatic verification)
  //   3. test=off          (return to PID mode)
  //   4. Kp=0.8            (set PID gains)
  //   5. v=100             (set target RPM)
  //   6. Open Serial Plotter to watch tuning
  //   7. Adjust Kp/Ki/Kd based on graph
  
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    // Testing mode commands
    if (input == "test=on") {
      testingMode = true;
      testPWM = 0;
      Serial.println("✓ Testing Mode: ENABLED");
      Serial.println("  PID Controller is now DISABLED");
      Serial.println("  Use 'test=run' to run automatic test");
      Serial.println("  Or use 'test=pwm=VALUE' to set manual PWM (0, 128, 255)");
      Serial.println("  Use 'test=off' to return to PID mode\n");
    }
    else if (input == "test=off") {
      testingMode = false;
      stopMotor();
      Serial.println("✓ Testing Mode: DISABLED");
      Serial.println("  PID Controller is now ACTIVE");
      Serial.println("  Motor stopped. Ready for PID tuning!\n");
    }
    else if (input == "test=run") {
      if (!testingMode) {
        Serial.println("⚠️  Testing mode not enabled!");
        Serial.println("  First send: test=on\n");
      } else {
        runEncoderTest();
      }
    }
    else if (input.startsWith("test=pwm=")) {
      int pwmValue = input.substring(9).toInt();
      if (pwmValue < 0 || pwmValue > 255) {
        Serial.println("⚠️  PWM value must be 0-255");
        Serial.println("  Try: test=pwm=128\n");
      } else {
        testPWM = pwmValue;
        Serial.println("✓ Test PWM set to: " + String(testPWM));
        Serial.println("  Motor will run at fixed PWM=" + String(testPWM));
        Serial.println("  Send 'status' to see current speed\n");
      }
    }
    else if (input == "test=status" || (input == "status" && testingMode)) {
      Serial.println("\n╔════════════════════════════════════════════════╗");
      Serial.println("║            TESTING MODE STATUS                 ║");
      Serial.println("╚════════════════════════════════════════════════╝");
      Serial.println("Mode: " + String(testingMode ? "TESTING (PID OFF)" : "PID (Testing OFF)"));
      Serial.println("Test PWM: " + String(testPWM));
      Serial.println("Current Velocity: " + String(currentVelocity, 2) + " RPM");
      Serial.println("Encoder Count: " + String(encoderCount));
      Serial.println("");
      Serial.println("Test Results:");
      Serial.println("  PWM =   0: " + String(speedAt0, 2) + " RPM");
      Serial.println("  PWM = 128: " + String(maxSpeedAt128, 2) + " RPM");
      Serial.println("  PWM = 255: " + String(maxSpeedAt255, 2) + " RPM\n");
    }
    else if (input.startsWith("Kp=")) {
      if (testingMode) {
        Serial.println("⚠️  Cannot adjust PID in testing mode!");
        Serial.println("  First send: test=off\n");
      } else {
        Kp = input.substring(3).toFloat();
        Serial.println("Kp updated to: " + String(Kp, 4));
      }
    }
    else if (input.startsWith("Ki=")) {
      if (testingMode) {
        Serial.println("⚠️  Cannot adjust PID in testing mode!");
        Serial.println("  First send: test=off\n");
      } else {
        Ki = input.substring(3).toFloat();
        Serial.println("Ki updated to: " + String(Ki, 4));
      }
    }
    else if (input.startsWith("Kd=")) {
      if (testingMode) {
        Serial.println("⚠️  Cannot adjust PID in testing mode!");
        Serial.println("  First send: test=off\n");
      } else {
        Kd = input.substring(3).toFloat();
        Serial.println("Kd updated to: " + String(Kd, 4));
      }
    }
    else if (input.startsWith("v=")) {
      if (testingMode) {
        Serial.println("⚠️  Cannot adjust velocity in testing mode!");
        Serial.println("  First send: test=off\n");
      } else {
        desiredVelocity = input.substring(2).toFloat();
        Serial.println("Desired Velocity updated to: " + String(desiredVelocity, 2) + " RPM");
      }
    }
    else if (input == "stop") {
      stopMotor();
      testingMode = false;
      Serial.println("Motor stopped. Testing mode disabled.\n");
    }
    else if (input == "status" && !testingMode) {
      Serial.println("\n=== REV HD Hex Motor (20:1) Status ===");
      Serial.println("PID Parameters:");
      Serial.println("  Kp: " + String(Kp, 4));
      Serial.println("  Ki: " + String(Ki, 4));
      Serial.println("  Kd: " + String(Kd, 4));
      Serial.println("");
      Serial.println("Motor Control:");
      Serial.println("  Desired Velocity: " + String(desiredVelocity, 1) + " RPM");
      Serial.println("  Current Velocity: " + String(currentVelocity, 1) + " RPM");
      Serial.println("  PID Error: " + String(pidError, 1) + " RPM");
      Serial.println("  PID Output: " + String(pidOutput, 0) + " (PWM 0-255)");
      Serial.println("");
      Serial.println("Encoder Data:");
      Serial.println("  Encoder Count: " + String(encoderCount));
      Serial.println("  Output PPR: " + String(OUTPUT_PPR));
      Serial.println("  Delta Count (last interval): " + String(encoderCount - lastEncoderCount));
      Serial.println("====\n");
      lastEncoderCount = encoderCount;
    }
    else {
      printHelp();
    }
  }
}

void printHelp() {
  Serial.println("\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║              AVAILABLE COMMANDS                        ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  
  Serial.println("\n🧪 ENCODER VERIFICATION (Testing Mode):");
  Serial.println("  test=on              - Enable testing mode (PID disabled)");
  Serial.println("  test=off             - Disable testing mode (return to PID)");
  Serial.println("  test=run             - Run automatic encoder verification");
  Serial.println("  test=pwm=VALUE       - Set manual PWM (0-255)");
  Serial.println("                         Example: test=pwm=128");
  Serial.println("  test=status          - Show testing mode status");
  
  Serial.println("\n🎛️  PID TUNING (Normal Mode):");
  Serial.println("  Kp=value             - Set proportional gain");
  Serial.println("                         Example: Kp=0.8");
  Serial.println("  Ki=value             - Set integral gain");
  Serial.println("                         Example: Ki=0.15");
  Serial.println("  Kd=value             - Set derivative gain");
  Serial.println("                         Example: Kd=0.12");
  Serial.println("  v=value              - Set desired velocity (RPM)");
  Serial.println("                         Example: v=100");
  
  Serial.println("\n📊 MONITORING:");
  Serial.println("  status               - Show current status and encoder data");
  Serial.println("  stop                 - Stop motor (disables testing mode)");
  Serial.println("  help                 - Show this message");
  
  Serial.println("\n📈 WORKFLOW:");
  Serial.println("  1. test=on           (enable testing mode)");
  Serial.println("  2. test=run          (run automatic test)");
  Serial.println("  3. test=off          (return to PID mode)");
  Serial.println("  4. Kp=0.8            (start tuning)");
  Serial.println("  5. v=100             (set desired RPM)");
  Serial.println("  6. (Open Serial Plotter to watch tuning)\n");
}
