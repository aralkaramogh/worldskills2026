// ==========================================================
//  RAW INPUT - 2 SHARP IR SENSORS (ESP32-S3)
// ==========================================================
//
//  PURPOSE:
//  - Read 2 IR sensors (analog)
//  - Send clean ADC values over Serial
//
//  OUTPUT FORMAT:
//  IR1:<value> IR2:<value>
//
// ==========================================================


// -------- Pins (ADC capable) --------
#define IR1 1
#define IR2 2


// ==========================================================
// FUNCTION: readIR()
// PURPOSE : reduce noise using small averaging
// ==========================================================
int readIR(int pin) {
  long sum = 0;

  for (int i = 0; i < 5; i++) {   // take 5 samples
    sum += analogRead(pin);
    delay(2);                     // small delay for stability
  }

  return sum / 5;                 // average value
}


// ==========================================================
void setup() {
  Serial.begin(115200);           // start serial
}


// ==========================================================
void loop() {

  // -------- Read sensors --------
  int ir1 = readIR(IR1);
  int ir2 = readIR(IR2);

  // -------- Output --------
  Serial.print("IR1:");
  Serial.print(ir1);

  Serial.print(" IR2:");
  Serial.println(ir2);

  delay(100);   // ~10 Hz output
}