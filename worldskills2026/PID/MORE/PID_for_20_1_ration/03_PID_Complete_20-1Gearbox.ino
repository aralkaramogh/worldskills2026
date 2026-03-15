/*
 * COMPLETE PID CONTROLLER - ESP32-S3 + Cytron MDD10
 * Step 3: Full PID with P, I, and D gains
 * 
 * UPDATED FOR: 5:1 + 4:1 GEARBOX (20:1 total reduction)
 * CPR: 560 (28 × 20)
 * Max RPM: 300
 * 
 * Features:
 * - Robust quadrature encoder reading with filtering
 * - Full PID controller with anti-windup
 * - Multiple waveform test inputs (square, sine, ramp)
 * - Graphical data logging
 * - Real-time tuning
 * - Motor safety limits
 */

// ===== CONFIGURATION FOR 20:1 GEARBOX =====
#define ENCODER_A_PIN 4
#define ENCODER_B_PIN 5
#define MOTOR_PWM_PIN 9
#define MOTOR_DIR_PIN 8

// GEARBOX: 5:1 × 4:1 = 20:1 reduction
#define COUNTS_PER_REV 560      // 28 × 20
#define MOTOR_MAX_RPM 300       // 6000 / 20
#define MAX_PWM 255
#define MAX_RPM_OUTPUT 300

#define SAMPLE_INTERVAL 20  // 20ms = 50Hz loop

// Anti-windup integral limits
#define INTEGRAL_MAX 100
#define INTEGRAL_MIN -100

// Low-pass filter for RPM
#define RPM_FILTER_ALPHA 0.7

// ===== GLOBAL VARIABLES =====
volatile long encoderCount = 0;
volatile long lastEncoderCount = 0;

// PID Gains - Adjusted for 20:1 gearbox (lower P due to high inertia)
float kP = 0.02;   // Start lower for high-inertia systems
float kI = 0.0;    // Usually small or zero
float kD = 0.005;  // Damping term

// PID state
float setpoint = 0;
float currentRPM = 0;
float filteredRPM = 0;
float error = 0;
float lastError = 0;
float integral = 0;
float derivative = 0;
float motorPWM = 0;

unsigned long lastPIDTime = 0;
unsigned long lastRPMTime = 0;

// Waveform configuration
enum WaveformType { SQUARE, SINE, RAMP, CONSTANT };
WaveformType currentWaveform = SQUARE;
float waveAmplitude = 150;    // RPM (max 150 for 0-300 range)
float waveFrequency = 0.2;    // Hz
float waveOffset = 150;       // Center RPM

// Data logging
bool graphicalMode = true;
unsigned long dataCounter = 0;
unsigned long startTime = 0;

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n===== COMPLETE PID CONTROLLER =====");
  Serial.println("Configuration: 20:1 Gearbox (5:1 + 4:1)");
  Serial.print("Motor CPR: ");
  Serial.println(COUNTS_PER_REV);
  Serial.print("Max RPM: ");
  Serial.println(MOTOR_MAX_RPM);
  Serial.print("Loop Rate: ");
  Serial.print(1000 / SAMPLE_INTERVAL);
  Serial.println(" Hz");
  Serial.print("Current Gains: kP=");
  Serial.print(kP, 4);
  Serial.print(" kI=");
  Serial.print(kI, 6);
  Serial.print(" kD=");
  Serial.println(kD, 4);
  Serial.println();
  
  // Encoder setup
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), encoderISR, CHANGE);
  
  // Motor control setup
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  ledcSetup(0, 5000, 8);
  ledcAttachPin(MOTOR_PWM_PIN, 0);
  
  digitalWrite(MOTOR_DIR_PIN, HIGH);  // Start forward
  
  lastPIDTime = micros();
  lastRPMTime = millis();
  startTime = millis();
  
  Serial.println("Ready for PID tuning");
  Serial.println("Type 'HELP' for commands\n");
}

// ===== MAIN LOOP =====
void loop() {
  unsigned long loopStart = micros();
  
  // Update RPM reading (non-blocking)
  updateRPM();
  
  // Update setpoint based on selected waveform
  updateSetpoint();
  
  // Calculate and apply PID (fixed 50Hz rate)
  unsigned long now = micros();
  if ((now - lastPIDTime) >= (SAMPLE_INTERVAL * 1000)) {
    float deltaTime = (now - lastPIDTime) / 1000000.0;
    lastPIDTime = now;
    
    // Run PID controller
    pidControl(deltaTime);
    
    // Apply to motor
    applyControl();
  }
  
  // Output data for graphing
  if (dataCounter++ % 10 == 0) {
    if (graphicalMode) {
      printCSVData();
    }
  }
  
  // Handle serial input
  if (Serial.available()) {
    handleSerialInput();
  }
  
  // Maintain loop timing
  long loopTime = micros() - loopStart;
  if (loopTime < (SAMPLE_INTERVAL * 1000)) {
    delayMicroseconds((SAMPLE_INTERVAL * 1000) - loopTime);
  }
}

// ===== UPDATE RPM =====
void updateRPM() {
  unsigned long now = millis();
  
  if ((now - lastRPMTime) >= SAMPLE_INTERVAL) {
    lastRPMTime = now;
    
    // Get delta counts since last read
    long deltaCounts = encoderCount - lastEncoderCount;
    lastEncoderCount = encoderCount;
    
    // Calculate raw RPM
    float rawRPM = (deltaCounts * 60000.0) / (SAMPLE_INTERVAL * COUNTS_PER_REV);
    
    // Apply low-pass filter for smoother readings
    filteredRPM = (RPM_FILTER_ALPHA * rawRPM) + ((1.0 - RPM_FILTER_ALPHA) * filteredRPM);
    currentRPM = filteredRPM;
  }
}

// ===== UPDATE SETPOINT (WAVEFORMS) =====
void updateSetpoint() {
  float elapsedTime = (millis() - startTime) / 1000.0;
  
  switch (currentWaveform) {
    case SQUARE:
      // Square wave with hysteresis
      if (fmod(elapsedTime * waveFrequency, 1.0) < 0.5) {
        setpoint = waveOffset + waveAmplitude;
      } else {
        setpoint = waveOffset - waveAmplitude;
      }
      break;
      
    case SINE:
      // Sine wave oscillation
      setpoint = waveOffset + waveAmplitude * sin(2.0 * PI * waveFrequency * elapsedTime);
      break;
      
    case RAMP:
      // Linear ramp up and down
      float modTime = fmod(elapsedTime * waveFrequency, 1.0);
      if (modTime < 0.5) {
        setpoint = waveOffset - waveAmplitude + (2.0 * waveAmplitude * modTime);
      } else {
        setpoint = waveOffset + waveAmplitude - (2.0 * waveAmplitude * (modTime - 0.5));
      }
      break;
      
    case CONSTANT:
      setpoint = waveOffset;
      break;
  }
  
  // Constrain to valid range
  setpoint = constrain(setpoint, 0, MAX_RPM_OUTPUT);
}

// ===== PID CONTROLLER =====
void pidControl(float deltaTime) {
  // Error calculation
  error = setpoint - currentRPM;
  
  // PROPORTIONAL term
  float pTerm = kP * error;
  
  // INTEGRAL term with anti-windup
  integral += error * deltaTime * kI;
  integral = constrain(integral, INTEGRAL_MIN, INTEGRAL_MAX);
  float iTerm = integral;
  
  // DERIVATIVE term
  float dTerm = 0;
  if (deltaTime > 0) {
    float rawDerivative = (error - lastError) / deltaTime;
    dTerm = kD * rawDerivative;
  }
  
  // Combine all terms
  float pidOutput = pTerm + iTerm + dTerm;
  
  // Convert PID output to PWM command
  float basePWM = (setpoint / MAX_RPM_OUTPUT) * MAX_PWM;
  float pidCorrection = (pidOutput / MAX_RPM_OUTPUT) * MAX_PWM * 0.5;
  
  motorPWM = basePWM + pidCorrection;
  motorPWM = constrain(motorPWM, 0, MAX_PWM);
  
  // Store error for next iteration
  lastError = error;
}

// ===== APPLY MOTOR CONTROL =====
void applyControl() {
  if (setpoint > 50) {  // Dead zone
    digitalWrite(MOTOR_DIR_PIN, HIGH);   // Forward
    ledcWrite(0, (uint8_t)motorPWM);
  } else if (setpoint < -50) {
    digitalWrite(MOTOR_DIR_PIN, LOW);    // Reverse
    ledcWrite(0, (uint8_t)abs(motorPWM));
  } else {
    // Stop if setpoint near zero
    ledcWrite(0, 0);
    integral = 0;  // Reset integral to prevent windup
  }
}

// ===== ENCODER ISR =====
void encoderISR() {
  int a = digitalRead(ENCODER_A_PIN);
  int b = digitalRead(ENCODER_B_PIN);
  
  static int lastA = 0;
  static int lastB = 0;
  
  if (a != lastA) {
    if (a == b) {
      encoderCount++;  // Forward
    } else {
      encoderCount--;  // Reverse
    }
    lastA = a;
  } else if (b != lastB) {
    if (a != b) {
      encoderCount++;  // Forward
    } else {
      encoderCount--;  // Reverse
    }
    lastB = b;
  }
}

// ===== CSV DATA OUTPUT =====
void printCSVData() {
  static unsigned long startLog = millis();
  float elapsed = (millis() - startLog) / 1000.0;
  
  Serial.print(elapsed, 2);
  Serial.print(",");
  Serial.print(setpoint, 1);
  Serial.print(",");
  Serial.print(currentRPM, 1);
  Serial.print(",");
  Serial.print(error, 1);
  Serial.print(",");
  Serial.print(motorPWM, 1);
  Serial.print(",");
  Serial.print(integral, 2);
  Serial.print(",");
  Serial.print(kP, 4);
  Serial.print(",");
  Serial.print(kI, 6);
  Serial.print(",");
  Serial.println(kD, 4);
}

// ===== SERIAL COMMAND HANDLER =====
void handleSerialInput() {
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toUpperCase();
  
  if (cmd.equals("SQUARE")) {
    currentWaveform = SQUARE;
    startTime = millis();
    integral = 0;
    Serial.println("Waveform: SQUARE (0.2 Hz, ±150 RPM)");
    Serial.println("Time(s),Setpoint(RPM),Actual(RPM),Error(RPM),PWM,Integral,kP,kI,kD");
  }
  else if (cmd.equals("SINE")) {
    currentWaveform = SINE;
    startTime = millis();
    integral = 0;
    Serial.println("Waveform: SINE (0.2 Hz, ±150 RPM)");
    Serial.println("Time(s),Setpoint(RPM),Actual(RPM),Error(RPM),PWM,Integral,kP,kI,kD");
  }
  else if (cmd.equals("RAMP")) {
    currentWaveform = RAMP;
    startTime = millis();
    integral = 0;
    Serial.println("Waveform: RAMP");
    Serial.println("Time(s),Setpoint(RPM),Actual(RPM),Error(RPM),PWM,Integral,kP,kI,kD");
  }
  else if (cmd.equals("STOP")) {
    ledcWrite(0, 0);
    setpoint = 0;
    integral = 0;
    Serial.println("Motor stopped");
  }
  else if (cmd.startsWith("KP:")) {
    kP = cmd.substring(3).toFloat();
    Serial.print("kP = ");
    Serial.println(kP, 4);
  }
  else if (cmd.startsWith("KI:")) {
    kI = cmd.substring(3).toFloat();
    Serial.print("kI = ");
    Serial.println(kI, 6);
  }
  else if (cmd.startsWith("KD:")) {
    kD = cmd.substring(3).toFloat();
    Serial.print("kD = ");
    Serial.println(kD, 4);
  }
  else if (cmd.startsWith("AMP:")) {
    waveAmplitude = cmd.substring(4).toFloat();
    waveAmplitude = constrain(waveAmplitude, 0, 150);
    Serial.print("Amplitude = ");
    Serial.println(waveAmplitude);
  }
  else if (cmd.startsWith("OFFSET:")) {
    waveOffset = cmd.substring(7).toFloat();
    waveOffset = constrain(waveOffset, 0, 300);
    Serial.print("Offset = ");
    Serial.println(waveOffset);
  }
  else if (cmd.startsWith("FREQ:")) {
    waveFrequency = cmd.substring(5).toFloat();
    Serial.print("Frequency = ");
    Serial.println(waveFrequency);
  }
  else if (cmd.equals("INFO")) {
    printInfo();
  }
  else if (cmd.equals("HELP")) {
    printHelp();
  }
}

// ===== PRINT INFO =====
void printInfo() {
  Serial.println("\n===== CURRENT STATUS =====");
  Serial.print("Setpoint: ");
  Serial.print(setpoint, 1);
  Serial.println(" RPM");
  Serial.print("Actual: ");
  Serial.print(currentRPM, 1);
  Serial.println(" RPM");
  Serial.print("Error: ");
  Serial.print(error, 1);
  Serial.println(" RPM");
  Serial.print("PWM: ");
  Serial.print(motorPWM, 1);
  Serial.print(" (");
  Serial.print((motorPWM / MAX_PWM) * 100, 1);
  Serial.println("%)");
  Serial.print("Encoder Count: ");
  Serial.println(encoderCount);
  Serial.print("kP=");
  Serial.print(kP, 4);
  Serial.print(", kI=");
  Serial.print(kI, 6);
  Serial.print(", kD=");
  Serial.println(kD, 4);
  Serial.println();
}

// ===== PRINT HELP =====
void printHelp() {
  Serial.println("\n===== PID TUNING FOR 20:1 GEARBOX =====");
  Serial.println("\nWaveform Selection:");
  Serial.println("  SQUARE          - Square wave test");
  Serial.println("  SINE            - Sine wave test");
  Serial.println("  RAMP            - Ramp wave test");
  Serial.println("\nGain Tuning:");
  Serial.println("  kP:<value>      - Set P gain (start: 0.02, range: 0.01-0.04)");
  Serial.println("  kI:<value>      - Set I gain (usually: 0.0)");
  Serial.println("  kD:<value>      - Set D gain (start: 0.005, range: 0.003-0.010)");
  Serial.println("\nWaveform Parameters:");
  Serial.println("  AMP:<value>     - Amplitude in RPM (max 150)");
  Serial.println("  OFFSET:<value>  - Center RPM (0-300)");
  Serial.println("  FREQ:<value>    - Frequency in Hz");
  Serial.println("\nOther:");
  Serial.println("  STOP            - Stop motor");
  Serial.println("  INFO            - Show current status");
  Serial.println("  HELP            - Show this help");
  Serial.println("\n===== TYPICAL TUNING SEQUENCE =====");
  Serial.println("1. Start: kP=0.02, kD=0.005");
  Serial.println("2. Run SQUARE wave");
  Serial.println("3. If slow response → increase kP (try 0.025, 0.030)");
  Serial.println("4. If oscillating → reduce kP or increase kD");
  Serial.println("5. Switch to SINE for smooth verification");
  Serial.println("6. Fine-tune kD to minimize overshoot");
  Serial.println("7. Test RAMP for ramp-tracking ability");
  Serial.println("========================================\n");
}
