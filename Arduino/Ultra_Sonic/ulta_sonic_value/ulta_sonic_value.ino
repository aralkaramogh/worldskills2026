// -------- Sensor 1 --------
#define TRIG1 8
#define ECHO1 9

// -------- Sensor 2 --------
#define TRIG2 10
#define ECHO2 11

long duration1, duration2;
int distance1, distance2;

// -------- Read Function --------
int readUltrasonic(int trigPin, int echoPin) {
  long duration;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout

  if (duration == 0) return -1; // no echo

  return duration * 0.034 / 2;
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);

  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);
}

void loop() {
  // -------- Sensor 1 --------
  distance1 = readUltrasonic(TRIG1, ECHO1);
  delay(50); // avoid cross-talk

  // -------- Sensor 2 --------
  distance2 = readUltrasonic(TRIG2, ECHO2);

  // -------- Output --------
  Serial.print("D1: ");
  Serial.print(distance1);
  Serial.print(" cm\t");

  Serial.print("D2: ");
  Serial.print(distance2);
  Serial.println(" cm");

  delay(200);
}