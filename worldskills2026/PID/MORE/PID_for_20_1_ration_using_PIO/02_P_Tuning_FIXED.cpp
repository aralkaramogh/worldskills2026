/*
 * P-TUNING & PID FRAMEWORK - FIXED (Non-Blocking Serial)
 * ESP32-S3 + Cytron DD10A + HD Hex Encoder
 * 20:1 Gearbox (5:1 × 4:1)
 * 
 * FIXED: Non-blocking serial input (no more hangs!)
 * 
 * Step 2: Proportional gain tuning with square/sine wave test inputs
 */

#include <Arduino.h>
#include <math.h>

// ===== CONFIGURATION FOR 20:1 GEARBOX =====
#define ENCODER_A_PIN 16
#define ENCODER_B_PIN 15
#define MOTOR_PWM_PIN 5
#define MOTOR_DIR_PIN 4

// GEARBOX: 5:1 × 4:1 = 20:1 reduction
//#define COUNTS_PER_REV 560      // 28 × 20
#define COUNTS_PER_REV 2240 //(28 CPR × 20:1 gearbox x 4 quadrature)

#define MOTOR_MAX_RPM 300       // 6000 / 20
#define MAX_PWM 255

#define SAMPLE_INTERVAL 20  // 20ms sampling (50Hz)

// ===== ENCODER VARIABLES =====
volatile long encoderCount = 0;
volatile long lastEncoderCount = 0;

// ===== PID VARIABLES =====
float kP = 0.02;    // START HERE
float kI = 0.0;     // Keep at 0 for P-tuning
float kD = 0.0;     // Keep at 0 for P-tuning

float setpoint = 0;
float currentRPM = 0;
float error = 0;
float motorPWM = 0;
float lastError = 0;

unsigned long lastTime = 0;

// ===== WAVEFORM VARIABLES =====
enum WaveformType { NONE, SQUARE, SINE };
WaveformType currentWaveform = NONE;
float waveAmplitude = 150;   // ±150 RPM
float waveFrequency = 0.2;   // 0.2 Hz = 5 sec period
float waveOffset = 150;      // Center at 150 RPM

// ===== SERIAL BUFFER (NON-BLOCKING) =====
String serialBuffer = "";
unsigned long lastDataPrint = 0;

// ===== FUNCTION PROTOTYPES =====
void encoderISR();
void updateRPM();
void updateSetpoint(float timeSeconds);
void calculatePID(float deltaTime);
void applyMotorControl();
void handleSerialInput();
void printGraphicalData();

// ===== ENCODER ISR =====
void IRAM_ATTR encoderISR() {
  encoderCount++;
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n===== P-TUNING & PID FRAMEWORK (FIXED) =====");
  Serial.println("Configuration: 20:1 Gearbox");
  Serial.println("Encoder: 560 CPR (28 × 20)");
  Serial.println("Max RPM: 300");
  Serial.println();
  
  // Encoder setup
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);
  
  // Motor setup
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  ledcSetup(0, 5000, 8);
  ledcAttachPin(MOTOR_PWM_PIN, 0);
  digitalWrite(MOTOR_DIR_PIN, HIGH);  // Forward
  
  lastTime = millis();
  
  Serial.println("Ready for P-tuning");
  Serial.println("Commands: SQUARE, SINE, STOP, kP:<value>, HELP");
  Serial.println();
}

// ===== MAIN LOOP =====
void loop() {
  unsigned long now = millis();
  
  // Update RPM (every SAMPLE_INTERVAL ms)
  static unsigned long lastRPMUpdate = 0;
  if (now - lastRPMUpdate >= SAMPLE_INTERVAL) {
    lastRPMUpdate = now;
    
    long deltaCounts = encoderCount - lastEncoderCount;
    lastEncoderCount = encoderCount;
    currentRPM = (deltaCounts * 60000.0) / (SAMPLE_INTERVAL * COUNTS_PER_REV);
  }
  
  // Generate setpoint from waveform
  float timeSeconds = now / 1000.0;
  if (currentWaveform == SQUARE) {
    setpoint = (fmod(timeSeconds * waveFrequency, 1.0) < 0.5) ? 
               (waveOffset + waveAmplitude) : (waveOffset - waveAmplitude);
  }
  else if (currentWaveform == SINE) {
    setpoint = waveOffset + waveAmplitude * sin(2 * M_PI * waveFrequency * timeSeconds);
  }
  
  // Calculate PID
  error = setpoint - currentRPM;
  float deltaTime = (now - lastTime) / 1000.0;
  lastTime = now;
  
  motorPWM = kP * error + kI * 0 + kD * 0;
  motorPWM = constrain(motorPWM, -255, 255);
  
  // Apply motor control
  if (motorPWM >= 0) {
    digitalWrite(MOTOR_DIR_PIN, HIGH);
    ledcWrite(0, (int)motorPWM);
  } else {
    digitalWrite(MOTOR_DIR_PIN, LOW);
    ledcWrite(0, (int)(-motorPWM));
  }
  
  // Print CSV data (every 100ms)
  if (now - lastDataPrint >= 100) {
    lastDataPrint = now;
    Serial.printf("%.2f,%.1f,%.1f,%.1f,%d,%.4f,%.4f,%.4f\n",
      timeSeconds,
      setpoint,
      currentRPM,
      error,
      (int)((fabs(motorPWM) / 255.0) * 100),
      kP, kI, kD);
  }
  
  // ===== NON-BLOCKING SERIAL INPUT =====
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        // Process complete command
        serialBuffer.trim();
        serialBuffer.toUpperCase();
        
        if (serialBuffer == "SQUARE") {
          currentWaveform = SQUARE;
          Serial.println("[SQUARE] Wave started");
        }
        else if (serialBuffer == "SINE") {
          currentWaveform = SINE;
          Serial.println("[SINE] Wave started");
        }
        else if (serialBuffer == "STOP") {
          currentWaveform = NONE;
          setpoint = 0;
          ledcWrite(0, 0);
          Serial.println("[STOP] Motor stopped");
        }
        else if (serialBuffer.startsWith("KP:")) {
          kP = serialBuffer.substring(3).toFloat();
          Serial.printf("[kP] = %.4f\n", kP);
        }
        else if (serialBuffer.startsWith("KI:")) {
          kI = serialBuffer.substring(3).toFloat();
          Serial.printf("[kI] = %.6f\n", kI);
        }
        else if (serialBuffer.startsWith("KD:")) {
          kD = serialBuffer.substring(3).toFloat();
          Serial.printf("[kD] = %.6f\n", kD);
        }
        else if (serialBuffer == "HELP") {
          Serial.println("\n===== COMMANDS =====");
          Serial.println("SQUARE         - Square wave (step response)");
          Serial.println("SINE           - Sine wave (smooth)");
          Serial.println("STOP           - Stop motor");
          Serial.println("kP:<val>       - Set P gain (e.g., kP:0.03)");
          Serial.println("kI:<val>       - Set I gain");
          Serial.println("kD:<val>       - Set D gain");
          Serial.println("HELP           - Show this");
          Serial.println("\nP-TUNING WORKFLOW:");
          Serial.println("1. Type: SQUARE");
          Serial.println("2. Watch motor oscillate");
          Serial.println("3. Type: kP:0.025 (increase by 0.005 each test)");
          Serial.println("4. Type: SQUARE (test new value)");
          Serial.println("5. Find where oscillation starts");
          Serial.println("6. Back off 20% and you're done!");
          Serial.println();
        }
        
        serialBuffer = "";  // Clear buffer
      }
    }
    else if (serialBuffer.length() < 32) {
      serialBuffer += c;  // Add character to buffer
    }
  }
  
  delay(1);  // Prevent watchdog timeout
}
