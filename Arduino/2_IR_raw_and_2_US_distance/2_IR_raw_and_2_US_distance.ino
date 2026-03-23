// ==========================================================
//  RAW INPUT - 2 IR + 2 ULTRASONIC (ESP32-S3)
// ==========================================================
//
//  PURPOSE:
//  - Read 2 IR (analog)
//  - Read 2 Ultrasonic (distance in cm)
//  - Output clean raw data for ROS parsing
//
//  OUTPUT FORMAT:
//  IR1:<v> IR2:<v> US1:<v> US2:<v>
//
// ==========================================================


// -------- IR Pins --------
#define IR1 1
#define IR2 2

// -------- Ultrasonic Pins --------
#define TRIG1 8
#define ECHO1 9

#define TRIG2 10
#define ECHO2 11


// ==========================================================
// FUNCTION: readIR()
// PURPOSE : stable analog reading
// ==========================================================
int readIR(int pin) {
  long sum = 0;

  for (int i = 0; i < 5; i++) {
    sum += analogRead(pin);
    delay(2);
  }

  return sum / 5;
}


// ==========================================================
// FUNCTION: readUS()
// PURPOSE : measure distance in cm
// NOTE    : returns -1 if no echo
// ==========================================================
int readUS(int trig, int echo) {

  digitalWrite(trig, LOW);
  delayMicroseconds(2);

  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH, 30000);  // timeout 30ms

  if (duration == 0) return -1;

  return duration * 0.034 / 2;
}


// ==========================================================
void setup() {
  Serial.begin(115200);

  // Ultrasonic pin setup
  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);

  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);
}


// ==========================================================
void loop() {

  // -------- Read IR --------
  int ir1 = readIR(IR1);
  int ir2 = readIR(IR2);

  // -------- Read Ultrasonic (sequential to avoid interference) --------
  int us1 = readUS(TRIG1, ECHO1);
  delay(60);   // IMPORTANT: prevents cross-talk

  int us2 = readUS(TRIG2, ECHO2);

  // -------- Output --------
  Serial.print("IR1:");
  Serial.print(ir1);

  Serial.print(" IR2:");
  Serial.print(ir2);

  Serial.print(" US1:");
  Serial.print(us1);

  Serial.print(" US2:");
  Serial.println(us2);

  delay(150);   // control data rate
}