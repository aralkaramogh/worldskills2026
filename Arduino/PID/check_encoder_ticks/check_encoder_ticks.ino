/*
 * ================================================================
 *  MOTOR CHARACTERIZATION (2 MOTOR - AMR)
 * ================================================================
 *
 *  PURPOSE:
 *  - Find relation between PWM and actual motor response
 *  - Detect mismatch between left and right motor
 *
 *  OUTPUT:
 *  - Serial Monitor: PWM vs Encoder Ticks
 *
 *  HOW TO USE:
 *  - Upload code
 *  - Open Serial Monitor (115200)
 *  - Note difference between left and right ticks
 *
 * ================================================================
 */

// ================== PIN CONFIG ==================
#define ENC_L_A 18
#define ENC_L_B 17
#define ENC_R_A 16
#define ENC_R_B 15

#define PWM_L 5
#define DIR_L 4
#define PWM_R 7
#define DIR_R 6

// ================== PWM CONFIG ==================
#define PWM_FREQ 20000     // 20kHz (silent + efficient)
#define PWM_RES  8         // 8-bit (0–255)

// ================== ENCODER VARIABLES ==================
volatile long ticksL = 0;   // Left motor encoder count
volatile long ticksR = 0;   // Right motor encoder count

// ================== ENCODER ISR ==================
void IRAM_ATTR encL_ISR() {
  // Quadrature decoding
  if (digitalRead(ENC_L_A) == digitalRead(ENC_L_B)) ticksL++;
  else ticksL--;
}

void IRAM_ATTR encR_ISR() {
  if (digitalRead(ENC_R_A) == digitalRead(ENC_R_B)) ticksR++;
  else ticksR--;
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  // Encoder pins
  pinMode(ENC_L_A, INPUT_PULLUP);
  pinMode(ENC_L_B, INPUT_PULLUP);
  pinMode(ENC_R_A, INPUT_PULLUP);
  pinMode(ENC_R_B, INPUT_PULLUP);

  // Attach interrupts
  attachInterrupt(digitalPinToInterrupt(ENC_L_A), encL_ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_R_A), encR_ISR, CHANGE);

  // Motor pins
  pinMode(DIR_L, OUTPUT);
  pinMode(DIR_R, OUTPUT);

  // Attach PWM (ESP32 Core 3.x style)
  ledcAttach(PWM_L, PWM_FREQ, PWM_RES);
  ledcAttach(PWM_R, PWM_FREQ, PWM_RES);

  // Set forward direction
  digitalWrite(DIR_L, HIGH);
  digitalWrite(DIR_R, HIGH);

  Serial.println("PWM\tTicks_L\tTicks_R");
}

// ================== LOOP ==================
void loop() {

  /*
   * Sweep PWM from low to high
   * This helps identify:
   *  - Dead zone (minimum PWM to move motor)
   *  - Non-linearity
   *  - Motor mismatch
   */

  for (int pwm = 50; pwm <= 255; pwm += 20) {

    ticksL = 0;
    ticksR = 0;

    // Apply same PWM to both motors
    ledcWrite(PWM_L, pwm);
    ledcWrite(PWM_R, pwm);

    delay(2000);   // Run motor for fixed time

    // Stop motors
    ledcWrite(PWM_L, 0);
    ledcWrite(PWM_R, 0);

    // Print results
    Serial.print(pwm);
    Serial.print("\t");
    Serial.print(ticksL);
    Serial.print("\t");
    Serial.println(ticksR);

    delay(2000);   // Pause before next step
  }

  // Stop execution after one sweep
  while (1);
}