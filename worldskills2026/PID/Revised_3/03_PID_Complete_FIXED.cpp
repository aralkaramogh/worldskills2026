/*
 * COMPLETE PID CONTROL - FIXED (Non-Blocking Serial)
 * ESP32-S3 + Cytron DD10A + HD Hex Encoder
 * 20:1 Gearbox (5:1 × 4:1)
 * 
 * Step 3: Full PID with I and D tuning
 * FIXED: Non-blocking serial (no hangs!)
 */

#include <Arduino.h>
#include <math.h>

// ===== PINS (SAME AS BEFORE) =====
#define ENCODER_A_PIN 16
#define ENCODER_B_PIN 15
#define MOTOR_PWM_PIN 5
#define MOTOR_DIR_PIN 4

// ===== GEARBOX SPECS =====
#define COUNTS_PER_REV 560
#define MOTOR_MAX_RPM 300

#define SAMPLE_INTERVAL 20  // 50 Hz

// ===== ENCODER =====
volatile long encoderCount = 0;
volatile long lastEncoderCount = 0;

// ===== PID GAINS (Your tuned values from Steps 1&2) =====
float kP = 0.03;   // From P-tuning
float kI = 0.001;  // Start very small
float kD = 0.008;  // From D-tuning

float setpoint = 0;
float currentRPM = 0;
float error = 0;
float integral = 0;  // Accumulated error
float lastError = 0;
float motorPWM = 0;

unsigned long lastTime = 0;

// ===== WAVEFORMS =====
enum WaveformType { NONE, SQUARE, SINE, RAMP };
WaveformType currentWaveform = NONE;
float waveAmplitude = 150;
float waveFrequency = 0.2;
float waveOffset = 150;

// ===== SERIAL =====
String serialBuffer = "";
unsigned long lastDataPrint = 0;

// ===== ENCODER ISR (WORKING QUADRATURE WITH STATE TRACKING) =====
void IRAM_ATTR encoderISR() {
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

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n===== COMPLETE PID CONTROL (FIXED) =====");
  Serial.println("20:1 Gearbox | 560 CPR | Max 300 RPM");
  Serial.println();
  Serial.printf("Starting gains: kP=%.4f, kI=%.4f, kD=%.4f\n\n", kP, kI, kD);
  
  // Encoder setup - BOTH pins for 4x quadrature
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), encoderISR, CHANGE);
  
  // Motor
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  ledcSetup(0, 5000, 8);
  ledcAttachPin(MOTOR_PWM_PIN, 0);
  digitalWrite(MOTOR_DIR_PIN, HIGH);
  
  lastTime = millis();
  
  Serial.println("Ready for PID tuning");
  Serial.println("Commands: SQUARE, SINE, RAMP, STOP, kP:x, kI:x, kD:x, HELP");
  Serial.println();
}

// ===== MAIN LOOP =====
void loop() {
  unsigned long now = millis();
  
  // ===== UPDATE RPM =====
  static unsigned long lastRPMUpdate = 0;
  if (now - lastRPMUpdate >= SAMPLE_INTERVAL) {
    lastRPMUpdate = now;
    
    long deltaCounts = encoderCount - lastEncoderCount;
    lastEncoderCount = encoderCount;
    currentRPM = (deltaCounts * 60000.0) / (SAMPLE_INTERVAL * COUNTS_PER_REV);
  }
  
  // ===== WAVEFORM SETPOINT =====
  float timeSeconds = now / 1000.0;
  
  if (currentWaveform == SQUARE) {
    setpoint = (fmod(timeSeconds * waveFrequency, 1.0) < 0.5) ? 
               (waveOffset + waveAmplitude) : (waveOffset - waveAmplitude);
  }
  else if (currentWaveform == SINE) {
    setpoint = waveOffset + waveAmplitude * sin(2 * M_PI * waveFrequency * timeSeconds);
  }
  else if (currentWaveform == RAMP) {
    float phase = fmod(timeSeconds * waveFrequency, 1.0);
    if (phase < 0.5) {
      setpoint = waveOffset + waveAmplitude * (phase * 2.0);
    } else {
      setpoint = waveOffset + waveAmplitude;
    }
  }
  
  // ===== PID CALCULATION =====
  float deltaTime = (now - lastTime) / 1000.0;
  lastTime = now;
  
  error = setpoint - currentRPM;
  
  // Integral (anti-windup: clamp between -100 and 100)
  integral += error * deltaTime;
  integral = constrain(integral, -100.0f, 100.0f);
  
  // Derivative
  float derivative = (error - lastError) / deltaTime;
  lastError = error;
  
  // Calculate output
  motorPWM = (kP * error) + (kI * integral) + (kD * derivative);
  motorPWM = constrain(motorPWM, -255, 255);
  
  // ===== APPLY MOTOR CONTROL =====
  if (motorPWM >= 0) {
    digitalWrite(MOTOR_DIR_PIN, HIGH);
    ledcWrite(0, (int)motorPWM);
  } else {
    digitalWrite(MOTOR_DIR_PIN, LOW);
    ledcWrite(0, (int)(-motorPWM));
  }
  
  // ===== PRINT CSV DATA =====
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
        serialBuffer.trim();
        serialBuffer.toUpperCase();
        
        if (serialBuffer == "SQUARE") {
          currentWaveform = SQUARE;
          integral = 0;  // Reset integral for new test
          Serial.println("[SQUARE] Square wave test started");
        }
        else if (serialBuffer == "SINE") {
          currentWaveform = SINE;
          integral = 0;
          Serial.println("[SINE] Sine wave test started");
        }
        else if (serialBuffer == "RAMP") {
          currentWaveform = RAMP;
          integral = 0;
          Serial.println("[RAMP] Ramp test started");
        }
        else if (serialBuffer == "STOP") {
          currentWaveform = NONE;
          setpoint = 0;
          ledcWrite(0, 0);
          integral = 0;
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
        else if (serialBuffer == "STATUS") {
          Serial.printf("\n[STATUS] kP=%.4f, kI=%.6f, kD=%.6f\n", kP, kI, kD);
          Serial.printf("Current: RPM=%.1f, Error=%.1f, Integral=%.1f\n\n", 
            currentRPM, error, integral);
        }
        else if (serialBuffer == "HELP") {
          Serial.println("\n===== PID TUNING COMMANDS =====");
          Serial.println("SQUARE         - Step response test");
          Serial.println("SINE           - Smooth oscillation test");
          Serial.println("RAMP           - Ramp response test");
          Serial.println("STOP           - Stop motor");
          Serial.println("kP:<val>       - Set proportional (e.g., kP:0.035)");
          Serial.println("kI:<val>       - Set integral (e.g., kI:0.001)");
          Serial.println("kD:<val>       - Set derivative (e.g., kD:0.008)");
          Serial.println("STATUS         - Show current gains");
          Serial.println("HELP           - Show this help\n");
          Serial.println("FULL PID TUNING WORKFLOW:");
          Serial.println("1. You already have best kP and kD from steps 1-2");
          Serial.println("2. Now fine-tune with I and D together:");
          Serial.println("   - Type: SQUARE");
          Serial.println("   - Type: kI:0.001");
          Serial.println("   - Type: SQUARE (observe error elimination)");
          Serial.println("   - If still oscillates, reduce kI");
          Serial.println("3. Test with SINE for smooth response");
          Serial.println("4. Test with RAMP for tracking");
          Serial.println("5. When satisfied, record your kP, kI, kD values\n");
        }
        
        serialBuffer = "";
      }
    }
    else if (serialBuffer.length() < 32) {
      serialBuffer += c;
    }
  }
  
  delay(1);
}
