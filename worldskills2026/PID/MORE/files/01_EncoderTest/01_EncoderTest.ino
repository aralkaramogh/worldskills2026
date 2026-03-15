/*
 * ENCODER TEST - ESP32-S3 + Cytron MDD10 + Quadrature Encoder
 * Step 1: Verify encoder readings and RPM calculation
 * 
 * Hardware:
 * - ESP32-S3
 * - Cytron MDD10 Motor Driver
 * - DC Motor with Quadrature Encoder (28 CPR at motor shaft)
 * - Encoder A: GPIO 4
 * - Encoder B: GPIO 5
 * - Motor PWM: GPIO 9 (or adjust to your pin)
 * - Motor DIR: GPIO 8 (or adjust to your pin)
 */

// ===== CONFIGURATION =====
#define ENCODER_A_PIN 16
#define ENCODER_B_PIN 15
#define MOTOR_PWM_PIN 4
#define MOTOR_DIR_PIN 5

// Motor specs
#define COUNTS_PER_REV 28  // At motor shaft - change to 84 (3:1), 112 (4:1), or 140 (5:1) if using gearbox
#define MOTOR_MAX_RPM 6000

// ===== GLOBAL VARIABLES =====
volatile long encoderCount = 0;
volatile long lastEncoderCount = 0;
long previousMillis = 0;
const long SAMPLE_INTERVAL = 100;  // 100ms sampling for RPM calculation

float currentRPM = 0;
float motorPWM = 0;

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  
  Serial.println("\n===== ENCODER & RPM TEST =====");
  Serial.print("Motor CPR: ");
  Serial.println(COUNTS_PER_REV);
  Serial.println();
  
  // Encoder setup - using interrupts for accurate counting
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  
  // Attach interrupts for quadrature decoding
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), encoderISR, CHANGE);
  
  // Motor control setup
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  
  // PWM setup for ESP32 (use ledcSetup for better control)
  ledcSetup(0, 5000, 8);  // Channel 0, 5kHz, 8-bit (0-255)
  ledcAttachPin(MOTOR_PWM_PIN, 0);
  
  // Start motor at 50% speed
  digitalWrite(MOTOR_DIR_PIN, HIGH);  // Forward direction
  motorPWM = 128;
  ledcWrite(0, (int)motorPWM);
  
  Serial.println("Motor started at 50% speed (PWM = 128)");
  Serial.println("\nTime(ms)\tCounts\tDelta\tRPM\tPWM%");
  Serial.println("================================================");
}

// ===== MAIN LOOP =====
void loop() {
  unsigned long currentMillis = millis();
  
  // Update RPM every SAMPLE_INTERVAL
  if (currentMillis - previousMillis >= SAMPLE_INTERVAL) {
    previousMillis = currentMillis;
    
    // Calculate RPM
    long deltaCounts = encoderCount - lastEncoderCount;
    lastEncoderCount = encoderCount;
    
    // RPM = (counts/interval) * (60000ms / interval) / CPR
    // RPM = deltaCounts * (60000 / SAMPLE_INTERVAL) / CPR
    currentRPM = (deltaCounts * 60000.0) / (SAMPLE_INTERVAL * COUNTS_PER_REV);
    
    // Print data
    Serial.print(currentMillis);
    Serial.print("\t");
    Serial.print(encoderCount);
    Serial.print("\t");
    Serial.print(deltaCounts);
    Serial.print("\t");
    Serial.print(currentRPM, 1);
    Serial.print("\t");
    Serial.println((motorPWM / 255.0) * 100, 1);
  }
  
  // Optional: Adjust motor speed with serial commands
  handleSerialInput();
  
  delay(10);
}

// ===== QUADRATURE ENCODER ISR =====
// This function is called on EVERY edge of A or B channel
void encoderISR() {
  // Read both channels
  int a = digitalRead(ENCODER_A_PIN);
  int b = digitalRead(ENCODER_B_PIN);
  
  // Simple quadrature decoding
  // A leads B = forward, B leads A = reverse
  static int lastA = 0;
  static int lastB = 0;
  
  if (a != lastA) {  // A changed
    if (a == b) {
      encoderCount++;  // Forward
    } else {
      encoderCount--;  // Reverse
    }
    lastA = a;
  } else if (b != lastB) {  // B changed
    if (a != b) {
      encoderCount++;  // Forward
    } else {
      encoderCount--;  // Reverse
    }
    lastB = b;
  }
}

// ===== SERIAL COMMAND HANDLER =====
void handleSerialInput() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.startsWith("PWM:")) {
      int pwmValue = command.substring(4).toInt();
      pwmValue = constrain(pwmValue, 0, 255);
      motorPWM = pwmValue;
      ledcWrite(0, pwmValue);
      Serial.print("PWM set to: ");
      Serial.println(pwmValue);
    }
    else if (command.equals("STOP")) {
      motorPWM = 0;
      ledcWrite(0, 0);
      Serial.println("Motor stopped");
    }
    else if (command.equals("RESET")) {
      encoderCount = 0;
      lastEncoderCount = 0;
      Serial.println("Encoder count reset to 0");
    }
    else if (command.equals("FWD")) {
      digitalWrite(MOTOR_DIR_PIN, HIGH);
      Serial.println("Direction: Forward");
    }
    else if (command.equals("REV")) {
      digitalWrite(MOTOR_DIR_PIN, LOW);
      Serial.println("Direction: Reverse");
    }
    else if (command.equals("HELP")) {
      Serial.println("\n===== COMMANDS =====");
      Serial.println("PWM:0-255   - Set motor PWM (0-255)");
      Serial.println("FWD         - Forward direction");
      Serial.println("REV         - Reverse direction");
      Serial.println("STOP        - Stop motor");
      Serial.println("RESET       - Reset encoder count");
      Serial.println("HELP        - Show this help");
      Serial.println("====================\n");
    }
  }
}
