/*
 * P-TUNING & PID FRAMEWORK - ESP32-S3 + Cytron MDD10
 * Step 2: Proportional gain tuning with square/sine wave test inputs
 * 
 * Features:
 * - Quadrature encoder reading
 * - PID controller framework
 * - Square wave and sine wave reference signals
 * - Graphical data output for monitoring
 * - Serial command interface
 */

// ===== CONFIGURATION =====
#define ENCODER_A_PIN 4
#define ENCODER_B_PIN 5
#define MOTOR_PWM_PIN 9
#define MOTOR_DIR_PIN 8

#define COUNTS_PER_REV 28  // Change based on gearbox (28, 84, 112, or 140)
#define MOTOR_MAX_RPM 6000
#define MAX_PWM 255
#define MAX_RPM_OUTPUT 6000  // Adjust based on your gearbox

#define SAMPLE_INTERVAL 20  // 20ms sampling (50Hz)

// ===== GLOBAL VARIABLES =====
volatile long encoderCount = 0;
volatile long lastEncoderCount = 0;

// PID variables
float kP = 0.05;  // START HERE - adjust this value for P-tuning
float kI = 0.0;   // Keep at 0 for P-tuning
float kD = 0.0;   // Keep at 0 for P-tuning

float setpoint = 0;        // Target RPM
float currentRPM = 0;
float error = 0;
float integral = 0;
float derivative = 0;
float lastError = 0;
float motorPWM = 0;

unsigned long lastTime = 0;
unsigned long currentTime = 0;

// Waveform generation
enum WaveformType { SQUARE, SINE, CONSTANT };
WaveformType currentWaveform = SQUARE;
float waveAmplitude = 2000;  // RPM amplitude
float waveFrequency = 0.2;   // Hz (period = 5 seconds)
float waveOffset = 2000;     // Center point in RPM

// Data logging
bool graphicalMode = true;  // CSV format for plotting
unsigned long dataCounter = 0;

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n===== P-TUNING & PID FRAMEWORK =====");
  Serial.print("Motor CPR: ");
  Serial.println(COUNTS_PER_REV);
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
  
  digitalWrite(MOTOR_DIR_PIN, HIGH);  // Forward
  
  lastTime = micros();
  
  Serial.println("Ready for P-tuning");
  Serial.println("Commands: SQUARE, SINE, STOP, kP:<value>, HELP");
  Serial.println();
}

// ===== MAIN LOOP =====
void loop() {
  currentTime = micros();
  float deltaTime = (currentTime - lastTime) / 1000000.0;  // Convert to seconds
  lastTime = currentTime;
  
  // Read and update RPM
  updateRPM();
  
  // Generate setpoint based on waveform
  updateSetpoint(currentTime / 1000000.0);  // Time in seconds
  
  // Calculate PID
  calculatePID(deltaTime);
  
  // Apply motor control
  applyMotorControl();
  
  // Output data
  if (dataCounter++ % 10 == 0) {  // Output every ~200ms
    if (graphicalMode) {
      printGraphicalData();
    }
  }
  
  // Handle serial commands
  handleSerialInput();
  
  delayMicroseconds(SAMPLE_INTERVAL * 1000 - (micros() - currentTime));
}

// ===== UPDATE RPM =====
void updateRPM() {
  static unsigned long lastRPMUpdate = 0;
  unsigned long now = millis();
  
  if (now - lastRPMUpdate >= SAMPLE_INTERVAL) {
    lastRPMUpdate = now;
    
    long deltaCounts = encoderCount - lastEncoderCount;
    lastEncoderCount = encoderCount;
    
    currentRPM = (deltaCounts * 60000.0) / (SAMPLE_INTERVAL * COUNTS_PER_REV);
  }
}

// ===== UPDATE SETPOINT (WAVEFORM) =====
void updateSetpoint(float timeSeconds) {
  switch (currentWaveform) {
    case SQUARE:
      // Square wave: switches between 0 and full amplitude
      setpoint = (fmod(timeSeconds * waveFrequency, 1.0) < 0.5) ? 
                 (waveOffset + waveAmplitude) : (waveOffset - waveAmplitude);
      break;
      
    case SINE:
      // Sine wave
      setpoint = waveOffset + waveAmplitude * sin(2 * PI * waveFrequency * timeSeconds);
      break;
      
    case CONSTANT:
      // Constant speed
      setpoint = waveOffset;
      break;
  }
  
  // Constrain setpoint
  setpoint = constrain(setpoint, 0, MAX_RPM_OUTPUT);
}

// ===== CALCULATE PID =====
void calculatePID(float deltaTime) {
  // Error calculation
  error = setpoint - currentRPM;
  
  // Proportional term
  float proportional = kP * error;
  
  // Integral term (trapezoidal integration)
  integral += error * deltaTime * kI;
  integral = constrain(integral, -255, 255);
  
  // Derivative term
  derivative = (deltaTime > 0) ? (error - lastError) / deltaTime : 0;
  derivative *= kD;
  
  // Total output (PWM units: 0-255)
  float pidOutput = proportional + integral + derivative;
  
  // Convert from RPM control to PWM (linear approximation)
  // PWM ≈ (RPM / MAX_RPM_OUTPUT) * MAX_PWM
  motorPWM = (setpoint / MAX_RPM_OUTPUT) * MAX_PWM + (pidOutput / MAX_RPM_OUTPUT) * MAX_PWM;
  
  // Constrain to valid PWM range
  motorPWM = constrain(motorPWM, 0, MAX_PWM);
  
  lastError = error;
}

// ===== APPLY MOTOR CONTROL =====
void applyMotorControl() {
  // Handle direction and PWM
  if (setpoint >= 0) {
    digitalWrite(MOTOR_DIR_PIN, HIGH);   // Forward
    ledcWrite(0, (int)motorPWM);
  } else {
    digitalWrite(MOTOR_DIR_PIN, LOW);    // Reverse
    ledcWrite(0, (int)abs(motorPWM));
  }
}

// ===== QUADRATURE ENCODER ISR =====
void encoderISR() {
  int a = digitalRead(ENCODER_A_PIN);
  int b = digitalRead(ENCODER_B_PIN);
  
  static int lastA = 0;
  static int lastB = 0;
  
  if (a != lastA) {
    if (a == b) encoderCount++;
    else encoderCount--;
    lastA = a;
  } else if (b != lastB) {
    if (a != b) encoderCount++;
    else encoderCount--;
    lastB = b;
  }
}

// ===== PRINT GRAPHICAL DATA (CSV Format) =====
void printGraphicalData() {
  // Format: Time(s), Setpoint(RPM), Actual(RPM), Error(RPM), PWM, kP, kI, kD
  static unsigned long startTime = millis();
  float elapsed = (millis() - startTime) / 1000.0;
  
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
  Serial.print(kP, 4);
  Serial.print(",");
  Serial.print(kI, 4);
  Serial.print(",");
  Serial.println(kD, 4);
}

// ===== SERIAL COMMAND HANDLER =====
void handleSerialInput() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toUpperCase();
    
    if (command.equals("SQUARE")) {
      currentWaveform = SQUARE;
      Serial.println("Waveform: SQUARE (0.2 Hz, ±2000 RPM)");
      Serial.println("Time(s),Setpoint(RPM),Actual(RPM),Error(RPM),PWM,kP,kI,kD");
    }
    else if (command.equals("SINE")) {
      currentWaveform = SINE;
      Serial.println("Waveform: SINE (0.2 Hz, ±2000 RPM)");
      Serial.println("Time(s),Setpoint(RPM),Actual(RPM),Error(RPM),PWM,kP,kI,kD");
    }
    else if (command.equals("STOP")) {
      ledcWrite(0, 0);
      setpoint = 0;
      Serial.println("Motor stopped");
    }
    else if (command.startsWith("KP:")) {
      kP = command.substring(3).toFloat();
      Serial.print("kP set to: ");
      Serial.println(kP, 4);
    }
    else if (command.startsWith("KI:")) {
      kI = command.substring(3).toFloat();
      Serial.print("kI set to: ");
      Serial.println(kI, 4);
    }
    else if (command.startsWith("KD:")) {
      kD = command.substring(3).toFloat();
      Serial.print("kD set to: ");
      Serial.println(kD, 4);
    }
    else if (command.startsWith("AMP:")) {
      waveAmplitude = command.substring(4).toFloat();
      Serial.print("Amplitude set to: ");
      Serial.println(waveAmplitude);
    }
    else if (command.startsWith("FREQ:")) {
      waveFrequency = command.substring(5).toFloat();
      Serial.print("Frequency set to: ");
      Serial.println(waveFrequency);
    }
    else if (command.equals("HELP")) {
      printHelp();
    }
  }
}

// ===== PRINT HELP =====
void printHelp() {
  Serial.println("\n===== COMMANDS =====");
  Serial.println("SQUARE          - Square wave setpoint");
  Serial.println("SINE            - Sine wave setpoint");
  Serial.println("STOP            - Stop motor");
  Serial.println("kP:<value>      - Set proportional gain (e.g., kP:0.05)");
  Serial.println("kI:<value>      - Set integral gain (e.g., kI:0.0)");
  Serial.println("kD:<value>      - Set derivative gain (e.g., kD:0.0)");
  Serial.println("AMP:<value>     - Set amplitude in RPM");
  Serial.println("FREQ:<value>    - Set frequency in Hz");
  Serial.println("HELP            - Show this help");
  Serial.println("====================\n");
  Serial.println("P-TUNING GUIDE:");
  Serial.println("1. Start with kP=0.05, run SQUARE wave");
  Serial.println("2. Increase kP until you see oscillation");
  Serial.println("3. Back off kP slightly for stable response");
  Serial.println("4. Record the best kP value");
  Serial.println("5. Then proceed to D-tuning\n");
}
