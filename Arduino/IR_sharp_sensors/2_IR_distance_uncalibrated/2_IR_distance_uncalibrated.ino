// ==========================================================
//  SHARP IR SENSOR INTERFACE (2 Sensors) - ESP32-S3
// ==========================================================
//
//  WHAT THIS CODE DOES:
//  - Reads 2 Sharp IR sensors using ADC
//  - Applies averaging filter (reduces noise)
//  - Converts ADC value → approximate distance (cm)
//  - Prints both raw and distance values
//
//  HARDWARE CONNECTION:
//  IR1 OUT → GPIO1
//  IR2 OUT → GPIO2
//  VCC     → 5V (preferred) or 3.3V
//  GND     → GND
//
// ==========================================================


// -------- Pin Definitions --------
#define IR1 1   // GPIO1 (ADC1_0)
#define IR2 2   // GPIO2 (ADC1_1)


// ==========================================================
// FUNCTION: readIR()
// PURPOSE : Read analog value with noise filtering
// METHOD  : Take 10 samples and average them
// ==========================================================
int readIR(int pin) {
  long sum = 0;

  for (int i = 0; i < 10; i++) {
    sum += analogRead(pin);   // read ADC value (0–4095)
    delay(2);                 // small delay for stability
  }

  return sum / 10;            // return average value
}


// ==========================================================
// FUNCTION: getDistance()
// PURPOSE : Convert ADC value → approximate distance (cm)
// NOTE    : Sharp IR sensors are NON-LINEAR
//           This is an empirical formula (approximation)
// ==========================================================
float getDistance(int adc) {

  // If value too low → object is far (or no object)
  if (adc < 100) return 80;   // assume ~80 cm max range

  // Convert ADC → distance using approximation formula
  return 4800.0 / (adc - 20);
  //d = k/(adc-c) --- document in help folder
}


// ==========================================================
// SETUP FUNCTION (runs once)
// ==========================================================
void setup() {
  Serial.begin(115200);   // start serial communication
}


// ==========================================================
// LOOP FUNCTION (runs repeatedly)
// ==========================================================
void loop() {

  // -------- Step 1: Read sensor values --------
  int val1 = readIR(IR1);   // filtered ADC value from IR1
  int val2 = readIR(IR2);   // filtered ADC value from IR2


  // -------- Step 2: Convert to distance --------
  float d1 = getDistance(val1);
  float d2 = getDistance(val2);


  // -------- Step 3: Print values --------
  // Format: IRx: ADC (distance cm)

  Serial.print("IR1: ");
  Serial.print(val1);        // raw ADC value
  Serial.print(" (");
  Serial.print(d1);          // converted distance
  Serial.print(" cm)");

  Serial.print("   IR2: ");
  Serial.print(val2);
  Serial.print(" (");
  Serial.print(d2);
  Serial.println(" cm");


  // -------- Step 4: Delay --------
  delay(200);   // prevents excessive printing + stabilizes readings
}