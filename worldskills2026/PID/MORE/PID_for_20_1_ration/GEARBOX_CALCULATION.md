# ⚙️ Gearbox Stacking Calculation Guide

## Your Setup: 5:1 + 4:1 Cartridges

You've added TWO cartridges in **series** (stacked). This is common with UltraPlanetary!

### ❌ WRONG: Addition
```
CPR = 112 + 140 = 252  (INCORRECT!)
```

### ✅ CORRECT: Multiplication
When gearboxes are stacked, **gear ratios multiply**:
```
Total Reduction = 5:1 × 4:1 = 20:1
```

---

## Your Motor Specifications

### Base Motor (REV-41-1600 HD Hex)
```
Free Speed:    6000 RPM
Stall Torque:  0.105 Nm
Encoder CPR:   28 counts/revolution (at motor shaft)
```

### With Your 20:1 Gearbox Stack

```
Output Shaft RPM:    6000 ÷ 20 = 300 RPM (max)
Output Shaft CPR:    28 × 20 = 560 CPR ← USE THIS IN CODE
Torque Multiplication: × 20 (approximately)
```

---

## What to Change in Your Code

### In ALL THREE sketches (.ino files):

**Find this line:**
```cpp
#define COUNTS_PER_REV 28  // Change based on gearbox
```

**Replace with:**
```cpp
#define COUNTS_PER_REV 560  // 28 × 20 (5:1 × 4:1 stacked)
```

### Also update max RPM:

**Find:**
```cpp
#define MAX_RPM_OUTPUT 6000
```

**Replace with:**
```cpp
#define MAX_RPM_OUTPUT 300  // 6000 / 20 reduction
```

### Update waveform defaults:

In `02_P_Tuning.ino` and `03_PID_Complete.ino`, find:
```cpp
float waveAmplitude = 2000;  // RPM amplitude
float waveOffset = 2000;     // Center point in RPM
```

**Replace with:**
```cpp
float waveAmplitude = 150;   // RPM amplitude (max is 300)
float waveOffset = 150;      // Center point in RPM
```

---

## Why Multiplication and Not Addition?

### Gearbox Principle: Each Stage Reduces Speed

```
Input Speed × Ratio₁ × Ratio₂ = Output Speed
6000 RPM × (1/5) × (1/4) = 6000 × 0.2 × 0.25 = 300 RPM
```

### For Encoder Counts: Same Logic

```
Input CPR × Ratio₁ × Ratio₂ = Output CPR
28 × 5 × 4 = 28 × 20 = 560 CPR
```

Each rotation of the output shaft is now equivalent to 20 motor shaft rotations.

---

## Visual Diagram

```
Motor Input
   |
   v
[5:1 Cartridge]  ← Reduces by 5×
   |
   v (1200 RPM after this stage)
   |
[4:1 Cartridge]  ← Reduces by 4× again
   |
   v
Output Shaft (300 RPM final)
```

---

## Example: Encoder Reading

### With your 20:1 gearbox:

**If output shaft rotates 1 complete revolution:**
```
Encoder counts = 560 CPR
Time = 0.2 seconds (at 300 RPM)

RPM = (560 counts × 60 seconds/min) / (0.2 sec × 560 CPR)
    = 300 RPM ✓ Correct!
```

**Without the change (using 28 CPR):**
```
RPM = (560 counts × 60) / (0.2 × 28)
    = 33600 / 5.6
    = 6000 RPM ✗ WRONG! (Shows motor speed, not output speed)
```

---

## Comparison: Your Three Options

| Configuration | CPR | Max RPM | Use Case |
|--------------|-----|---------|----------|
| Direct drive (no gearbox) | 28 | 6000 | High speed, low torque |
| 4:1 gearbox only | 112 | 1500 | Medium speed & torque |
| **Your: 5:1 + 4:1 (20:1)** | **560** | **300** | **High torque, low speed** |

---

## Updated Code Variables Summary

```cpp
// ===== CONFIGURATION =====
#define ENCODER_A_PIN 4
#define ENCODER_B_PIN 5
#define MOTOR_PWM_PIN 9
#define MOTOR_DIR_PIN 8

// FOR YOUR 20:1 GEARBOX:
#define COUNTS_PER_REV 560        // ← CHANGED from 28
#define MOTOR_MAX_RPM 300         // ← CHANGED from 6000
#define MAX_PWM 255
#define MAX_RPM_OUTPUT 300        // ← CHANGED from 6000

#define SAMPLE_INTERVAL 20  // 20ms sampling (50Hz)

// WAVEFORM SETTINGS (in 02_P_Tuning.ino and 03_PID_Complete.ino):
float waveAmplitude = 150;   // ← CHANGED from 2000
float waveFrequency = 0.2;   // Hz - keep same
float waveOffset = 150;      // ← CHANGED from 2000
```

---

## Why This Matters for PID Tuning

### Motor torque is much higher with 20:1

Your motor now has:
- **Low speed** (300 RPM max) ✓
- **High torque** (≈20× more) ✓
- **Slower response** (needs more time to accelerate)
- **More stable** (easier to control)

### P-gain will likely be different

With 20:1 gearbox:
- You probably need **lower kP** (maybe 0.02-0.04 instead of 0.05-0.07)
- Why? More mechanical inertia, higher torque makes system "stiffer"
- D-gain becomes more important
- I-gain might help more

**Recommendation for your setup:**
```
Start with:
  kP = 0.02
  kI = 0.0
  kD = 0.005
```

---

## RPM Measurement Verification

### After you upload with COUNTS_PER_REV = 560:

**Test procedure:**
1. Manually measure your motor shaft RPM with a tachometer
2. Or, measure the output shaft and multiply by 20
3. Compare to the Serial Monitor reading

**They should match!**

If they don't:
- Check COUNTS_PER_REV value
- Verify encoder is connected correctly
- Check gear cartridge installation (verify 5:1 and 4:1 as labeled)

---

## Quick Reference: Common Configurations

```
CONFIG                    CPR      MAX_RPM    kP_START
────────────────────────────────────────────────────────
Direct drive             28       6000       0.06
3:1 gearbox only         84       2000       0.05
4:1 gearbox only        112       1500       0.04
5:1 gearbox only        140       1200       0.03
YOUR: 5:1 + 4:1        560        300       0.02
────────────────────────────────────────────────────────
```

---

## Torque Consideration

With a 20:1 gearbox, your motor can deliver significant torque:

```
Motor stall torque:     0.105 Nm
With 20:1 reduction:    0.105 × 20 = 2.1 Nm (approx)

This is quite a lot! Your motor will be very strong.
```

### Practical implications:
- Can hold position easily against external force
- Responds more slowly to commands
- PID gains need to be conservative
- Excellent for heavy loads

---

## Checklist for Your Changes

- [ ] Change `COUNTS_PER_REV` to **560** in all three sketches
- [ ] Change `MOTOR_MAX_RPM` to **300** in all three sketches
- [ ] Change `MAX_RPM_OUTPUT` to **300** in all three sketches
- [ ] Update waveform amplitude to **150** RPM
- [ ] Update waveform offset to **150** RPM
- [ ] Start P-tuning with **kP = 0.02** (not 0.05)
- [ ] Verify measured RPM matches Serial Monitor reading
- [ ] Record your final gains for 20:1 setup

---

## Example: Expected P-Tuning Results

With your 20:1 gearbox, a typical tuning session might look like:

```
Test 1: kP = 0.01  → Response very slow, takes 5+ seconds
Test 2: kP = 0.02  → Better, reaches ~150 RPM in 3 seconds
Test 3: kP = 0.03  → Fast, reaches 150 RPM in 2 seconds, slight overshoot
Test 4: kP = 0.04  → Too fast, oscillates around setpoint
Result: Best is kP = 0.03 (sweet spot)

Then add:
Test 5: kP = 0.03, kD = 0.005  → Reduces overshoot, stable
Test 6: kP = 0.03, kD = 0.008  → Very smooth, no overshoot
Final: kP = 0.03, kD = 0.008, kI = 0.0
```

---

## Summary Table

```
BEFORE (thinking direct drive):
├─ COUNTS_PER_REV = 28
├─ MAX_RPM = 6000
├─ Waveform amplitude = 2000
└─ kP_start = 0.05

AFTER (with your 5:1 + 4:1):
├─ COUNTS_PER_REV = 560  ← 28 × 5 × 4
├─ MAX_RPM = 300         ← 6000 / (5 × 4)
├─ Waveform amplitude = 150
└─ kP_start = 0.02
```

---

## Verify Your Cartridge Installation

If your readings seem off, double-check your cartridge stack:

```
Input from motor pinion →
    ↓
  [5:1 Cartridge]  ← Should say "5:1" on side
    ↓
  [4:1 Cartridge]  ← Should say "4:1" on side
    ↓
    Output shaft

If cartridges are reversed or wrong ratio:
  5:1 + 3:1 = 15:1 (different!)
  4:1 + 3:1 = 12:1 (different!)
```

---

Good luck with your 20:1 setup! This will give you excellent torque and control. The slower response is actually easier to tune a PID controller for.
