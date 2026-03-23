// ==========================================================
//  SHARP IR - RAW ADC READING (FOR CALIBRATION)
// ==========================================================
//
//  PURPOSE:
//  - Show clean ADC values from both sensors
//  - Helps you decide threshold
//
// ==========================================================


// -------- Pins --------
#define IR1 1
#define IR2 2


// ==========================================================
// FUNCTION: readIR()
// PURPOSE : reduce noise using averaging
// ==========================================================
int readIR(int pin) {
  long sum = 0;

  for (int i = 0; i < 5; i++) {   // small averaging
    sum += analogRead(pin);
    delay(2);
  }

  return sum / 5;
}


// ==========================================================
void setup() {
  Serial.begin(115200);
}


// ==========================================================
void loop() {

  int ir1 = readIR(IR1);
  int ir2 = readIR(IR2);

  // -------- Print clean values --------
  Serial.print("IR1: ");
  Serial.print(ir1);

  Serial.print("\tIR2: ");
  Serial.println(ir2);

  delay(200);
}