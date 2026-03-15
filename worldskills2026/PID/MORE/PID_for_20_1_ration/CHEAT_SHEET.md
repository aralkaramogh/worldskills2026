# ⚙️ CHEAT SHEET: Your 20:1 Gearbox PID Setup

## Your Motor Configuration at a Glance

```
┌─────────────────────────────────────────────────────┐
│           REV-41-1600 HD HEX MOTOR                  │
│                                                      │
│  Base Specs:                                         │
│  • Free Speed: 6000 RPM                             │
│  • Encoder: 28 CPR (counts per revolution)          │
│  • Max Torque: 0.105 Nm                             │
│                                                      │
│  With Your 20:1 Gearbox:                            │
│  • Output Speed: 300 RPM max                        │
│  • Encoder CPR: 560 (28 × 20)                       │
│  • Output Torque: ~2.1 Nm (20× multiplication)     │
└─────────────────────────────────────────────────────┘
```

---

## Code Changes (Copy & Paste)

### Step 1: Encoder Test
```cpp
#define COUNTS_PER_REV 560      // ← CHANGE THIS
#define MOTOR_MAX_RPM 300       // ← CHANGE THIS
```

### Step 2: P-Tuning
```cpp
#define COUNTS_PER_REV 560
#define MOTOR_MAX_RPM 300
float waveAmplitude = 150;   // ← CHANGE THIS (was 2000)
float waveOffset = 150;      // ← CHANGE THIS (was 2000)
float kP = 0.02;             // ← START HERE (was 0.05)
```

### Step 3: Complete PID
```cpp
#define COUNTS_PER_REV 560
#define MOTOR_MAX_RPM 300
float waveAmplitude = 150;
float waveOffset = 150;
float kP = 0.02;   // From your P-tuning results
float kI = 0.0;    // Usually stays 0
float kD = 0.005;  // Start here, adjust up to 0.010
```

---

## Tuning Command Reference

### Step 1: Encoder Test
```
FWD              → Motor forward
REV              → Motor reverse
PWM:50           → ~60 RPM expected
PWM:128          → ~150 RPM expected
PWM:200          → ~234 RPM expected
STOP             → Stop motor
INFO             → Show settings
HELP             → Show all commands
```

### Step 2: P-Tuning
```
SQUARE           → Start square wave test ← DO THIS FIRST
SINE             → Sine wave test
kP:0.02          → Set kP value
kP:0.025         → Increase slightly
kP:0.030         → Continue tuning
AMP:150          → Set amplitude (max 150)
STOP             → Stop motor
```

### Step 3: Full PID
```
SQUARE           → Test with square wave
SINE             → Smooth test
RAMP             → Ramp following test
kP:0.025         → Your P value from step 2
kD:0.005         → Start D-gain
kD:0.008         → Increase to reduce overshoot
kD:0.010         → Stop if becomes sluggish
INFO             → Show current RPM & error
STOP             → Stop motor
```

---

## Expected Behavior at Each Stage

### ✓ Encoder Test (Step 1)
```
PWM 128 → RPM ~150  (smooth, no jerking)
PWM 200 → RPM ~234  (proportional increase)
PWM 255 → RPM ~300  (max speed achieved)
```

### ✓ P-Tuning with SQUARE (Step 2)
```
kP=0.01  → SLOW    (takes 5+ sec to reach setpoint)
kP=0.02  → GOOD    (reaches in 3-4 sec, stable)
kP=0.03  → BETTER  (reaches in 2-3 sec, slight overshoot)
kP=0.04  → BAD     (oscillates around setpoint)

Best choice: kP = 0.03 (sweet spot)
```

### ✓ Full PID with D-Gain (Step 3)
```
kP=0.03, kD=0.003  → Still has overshoot
kP=0.03, kD=0.005  → Overshoot reduced
kP=0.03, kD=0.008  → Smooth, stable ← IDEAL
kP=0.03, kD=0.010  → Sluggish (too much damping)

Final gains: kP=0.03, kI=0.0, kD=0.008
```

---

## Typical Values by Gearbox Type

```
CONFIG              CPR    MAX_RPM  kP_START  kD_TYPICAL
──────────────────────────────────────────────────────
Direct drive        28     6000     0.06      0.008
3:1 gearbox         84     2000     0.05      0.007
4:1 gearbox        112     1500     0.04      0.006
5:1 gearbox        140     1200     0.03      0.005
YOUR: 5:1+4:1     560      300     0.02      0.005
──────────────────────────────────────────────────────
```

---

## Pin Configuration (Don't Change)

```
ENCODER
├─ A Signal → GPIO 4
├─ B Signal → GPIO 5
└─ GND → ESP32 GND

MOTOR CONTROL (Cytron MDD10)
├─ PWM → GPIO 9
├─ DIR → GPIO 8
└─ GND → ESP32 GND (common return)
```

---

## Serial Monitor Settings
```
Port: COMx (your device)
Baud: 115200
Line Ending: Newline
```

---

## CSV Data Output Format (For graphing)

```
Time(s),Setpoint(RPM),Actual(RPM),Error(RPM),PWM,Integral,kP,kI,kD
0.20,   300.0,        150.5,       149.5,     128.3, 5.2, 0.02, 0, 0.005
0.40,   300.0,        220.3,        79.7,     140.1, 8.1, 0.02, 0, 0.005
```

**Copy these lines and paste into Excel to graph!**

---

## Troubleshooting Quick Fixes

### Motor won't run
→ Check: DIR pin HIGH for forward
→ Check: 12V power to Cytron

### RPM reads 0
→ Verify: Encoder A, B on GPIO 4, 5
→ Verify: Motor actually spinning
→ Try: RESET command

### RPM jumps around
→ Add: 100nF capacitors to encoder pins
→ Use: Shielded encoder cable

### System oscillates
→ Reduce: kP by 0.005
→ Increase: kD by 0.002

### Response too slow
→ Increase: kP by 0.005
→ Check: Motor has enough torque

---

## Performance Targets

```
After proper tuning, you should see:

Rise Time:        3-4 seconds (time to reach setpoint)
Overshoot:        <5-10% (how much it overshoots)
Settling Time:    1-2 seconds (time to stabilize)
Steady-State Error: <10 RPM (final error)
RMS Error:        <50 RPM (overall accuracy)
```

---

## Your Hardware Advantage with 20:1

✓ HIGH TORQUE: 2.1 Nm (vs 0.105 Nm direct drive)
✓ STABLE: High inertia = harder to oscillate
✓ PRECISE: 560 CPR = 20× better resolution
✓ RELIABLE: Slower response = easier to tune
✓ POWERFUL: Can handle significant loads

---

## Tuning Timeline

```
Encoder Test:    10 minutes
P-Tuning:        30-45 minutes
Full PID:        20-30 minutes
Verification:    15 minutes
─────────────────────────
TOTAL:           ~2 hours
```

---

## Files You Need

| File | Use | When |
|------|-----|------|
| `01_EncoderTest_20-1Gearbox.ino` | Verify encoder | First |
| `02_P_Tuning_20-1Gearbox.ino` | Find kP | Second |
| `03_PID_Complete_20-1Gearbox.ino` | Full tuning | Third |
| `pid_monitor.py` | Graph data | Optional |

---

## Arduino Installation Quick Fix

```
1. Download: https://github.com/espressif/arduino-esp32/releases
   Get the ZIP file for your OS (Windows/Mac/Linux)

2. Extract to:
   Windows: C:\Users\YourName\AppData\Local\Arduino15\packages\esp32
   Mac: ~/Library/Arduino15/packages/esp32
   Linux: ~/.arduino15/packages/esp32

3. Restart Arduino IDE

4. Tools → Board → ESP32S3 Dev Module ✓
```

---

## Gearbox Math Made Simple

```
Motor:      28 CPR,  6000 RPM
Cartridge1: 5:1 reduction
Cartridge2: 4:1 reduction

MULTIPLY (not add):
Reduction = 5 × 4 = 20:1
CPR = 28 × 20 = 560
RPM = 6000 ÷ 20 = 300
Torque = 0.105 × 20 = 2.1 Nm

✓ This is correct!
✗ NOT: 112 + 140 = 252 (wrong method)
```

---

## One-Page Tuning Process

```
1. Upload 01_EncoderTest_20-1Gearbox.ino
   Type: FWD, PWM:128, verify ~150 RPM

2. Upload 02_P_Tuning_20-1Gearbox.ino
   Type: SQUARE
   Adjust: kP from 0.01 to 0.04
   Record: Best kP value

3. Upload 03_PID_Complete_20-1Gearbox.ino
   Set: Your kP value
   Type: SQUARE
   Adjust: kD from 0.003 to 0.010
   Verify: SINE and RAMP work well

4. Record final kP, kI, kD values

DONE! 🎉
```

---

## Success Indicators

✓ **Encoder reading smooth values**
✓ **Motor runs in both directions**
✓ **Response reaches setpoint in 3-4 sec**
✓ **Overshoot less than 10%**
✓ **No oscillation or hunting**
✓ **SINE wave tracking is smooth**
✓ **You've recorded final gains**

---

## Common Mistakes (AVOID!)

❌ Using 28 CPR instead of 560
❌ Using 6000 RPM instead of 300
❌ Starting with kP=0.05 (too high!)
❌ Not using `_20-1Gearbox.ino` files
❌ Adding gains instead of multiplying
❌ Skipping encoder test
❌ Not recording final values
❌ Assuming one setup works for all

---

## Your Setup Summary Card

```
MOTOR:      REV-41-1600 HD Hex
GEARBOX:    5:1 + 4:1 = 20:1
ENCODER:    560 CPR at output
MAX RPM:    300 at output
TORQUE:     ~2.1 Nm at output
BEST FOR:   Torque, precision, stability
START kP:   0.02
START kD:   0.005
LOOP RATE:  50 Hz
TUNING TIME: 2-3 hours
```

---

**Print this sheet and keep it next to your workbench!** 📋

Good luck! You've got a solid setup. 🚀
