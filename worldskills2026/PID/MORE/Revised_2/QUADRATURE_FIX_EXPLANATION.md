# ✅ ENCODER QUADRATURE FIX - Updated All 3 Codes

## The Problem

Your diagnostic code was **only counting 2x** instead of **4x** quadrature:

```cpp
❌ WRONG:
attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);  // Only pin A

void IRAM_ATTR encoderISR() {
  encoderCount++;  // Just increments, doesn't check B pin
}

✅ CORRECT:
attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);
attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), encoderISR, CHANGE);  // Both pins!

void IRAM_ATTR encoderISR() {
  int a = digitalRead(ENCODER_A_PIN);
  int b = digitalRead(ENCODER_B_PIN);
  if (a == b) {
    encoderCount--;
  } else {
    encoderCount++;
  }
}
```

---

## Impact on Your Numbers

Your previous readings (with 2x counting):

```
PWM 100:   Showed RPM: 12   →  Actual: 24 RPM
PWM 128:   Showed RPM: 16   →  Actual: 32 RPM
PWM 255:   Showed RPM: 36   →  Actual: 72 RPM
```

Still not reaching 300 RPM at full power, so hardware issue remains.

---

## Files Updated (All 3)

✅ **MOTOR_DIAGNOSTIC_TEST_FIXED_QUADRATURE.cpp**
- Proper 4x quadrature ISR
- Interrupts on both pins A and B
- COUNTS_PER_REV = 2240 (correct)

✅ **02_P_Tuning_FIXED.cpp**
- Updated ISR with quadrature logic
- Added interrupt on pin B
- COUNTS_PER_REV = 2240

✅ **03_PID_Complete_FIXED.cpp**
- Updated ISR with quadrature logic
- Added interrupt on pin B
- COUNTS_PER_REV = 2240

---

## New Expected Output

After uploading fixed diagnostic code:

```
Type: PWM:100

OLD (2x counting):   RPM: 12
NEW (4x counting):   RPM: 48-50

Type: PWM:255

OLD (2x counting):   RPM: 36
NEW (4x counting):   RPM: 144-150

If EVEN HIGHER (~300):
  Motor is actually working as expected!
```

---

## What to Do Now

### 1. Upload MOTOR_DIAGNOSTIC_TEST_FIXED_QUADRATURE.cpp

```
1. Copy the new diagnostic code
2. Paste into Arduino IDE
3. Upload to ESP32
4. Open serial monitor (115200)
```

### 2. Test

```
Type: PWM:255
Press Enter

Watch RPM value for 3 seconds
Report what you see
```

### Expected:

If motor is actually 20:1 gearbox at 6000 RPM:
```
Encoder: 5710 ticks | RPM: 285 | PWM: 256
Encoder: 6050 ticks | RPM: 302 | PWM: 256
Encoder: 6390 ticks | RPM: 295 | PWM: 256
```

Should be **250-300 RPM** now ✓

---

## If Still Low (100-150 RPM)

Then gearbox ratio or motor specs are different:
- Motor might be 3000 RPM (not 6000)
- Gearbox might be 50:1 (not 20:1)
- High mechanical friction

---

## How Quadrature Decoding Works

The HD Hex encoder outputs two signals: A and B

```
Signal A:  ___┐  ┌____┐  ┌___
              └──┘    └──┘

Signal B:  ____┐  ┌____┐  ┌__
               └──┘    └──┘

By reading both:
- A changes alone = 1 count (rising edge)
- B changes when A is high = 1 count  
- A changes with B = 1 count (falling edge)
- B changes when A is low = 1 count

Total = 4 counts per encoder cycle = 4x multiplier
```

Your old code only counted A changes = 2x

---

## Technical Details

### Old ISR (2x counting)
```cpp
attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);

void encoderISR() {
  encoderCount++;  // Fires on every A edge (rising + falling)
}
// Result: 2 counts per encoder pulse
```

### New ISR (4x counting)
```cpp
attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderISR, CHANGE);
attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), encoderISR, CHANGE);

void encoderISR() {
  int a = digitalRead(ENCODER_A_PIN);
  int b = digitalRead(ENCODER_B_PIN);
  if (a == b) {
    encoderCount--;
  } else {
    encoderCount++;
  }
}
// Result: 4 counts per encoder pulse
```

The XOR check (a == b) tells us the direction and counts every state change properly.

---

## Ready to Test

**Upload MOTOR_DIAGNOSTIC_TEST_FIXED_QUADRATURE.cpp**

Run:
```
PWM:255
```

Report RPM value and I can tell you if motor/gearbox are correct! 🚀

---

## Summary

| Item | Before | After |
|------|--------|-------|
| ISR Type | Simple increment | Proper quadrature |
| Interrupts | 1 pin (A only) | 2 pins (A and B) |
| Counting | 2x | 4x ✓ |
| Expected max RPM | ~36 | ~144-300 |
| COUNTS_PER_REV | Should be 560 | Should be 2240 ✓ |

All three codes are now fixed and use proper 4x quadrature counting!
