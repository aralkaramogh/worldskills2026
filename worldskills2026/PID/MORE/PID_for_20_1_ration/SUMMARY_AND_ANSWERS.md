# Summary: Arduino Installation & Your 20:1 Gearbox Setup

## Your Two Questions Answered

### 1️⃣ Arduino/ESP32 Installation Issues

**Problem:** ESP32 board manager times out and fails to install

**Solutions (in order of ease):**

#### Quick Fix #1: Use Pre-installed Version
- Arduino IDE 2.0+ may already have ESP32 bundled
- Check: **Tools → Board → Boards Manager** → search "esp32"
- If installed, use it (skip the manager)

#### Quick Fix #2: Manual ZIP Installation (MOST RELIABLE)
1. Download ESP32 ZIP directly from GitHub:
   - https://github.com/espressif/arduino-esp32/releases
   - Get the latest `esp32-x.x.x-windows.zip` (or your OS)

2. Extract to correct folder:
   - **Windows:** `C:\Users\YourUsername\AppData\Local\Arduino15\packages\esp32`
   - **Mac:** `~/Library/Arduino15/packages/esp32`
   - **Linux:** `~/.arduino15/packages/esp32`

3. Restart Arduino IDE
4. Tools → Board → **ESP32S3 Dev Module** should appear

#### Quick Fix #3: Use PlatformIO Instead
- Install VS Code
- Add PlatformIO extension
- Create project → Select ESP32-S3
- No board manager timeouts ever

**See the detailed guide:** `ESP32_INSTALLATION_FIXES.md`

---

### 2️⃣ Your Gearbox Configuration

**You said:** "I added cartridges of 5:1 and 4:1, therefore 20"

**That's CORRECT!** ✓

But the calculation is **MULTIPLICATION**, not addition:

```
Stacked gearboxes: 5:1 × 4:1 = 20:1 total reduction
```

**Motor Output Shaft Specifications:**

```
Parameter              Your Setup         Calculation
─────────────────────────────────────────────────────
Motor base CPR         28                 (at motor shaft)
Gearbox reduction      5 × 4 = 20:1       (5:1 × 4:1)
Output shaft CPR       560                28 × 20 ✓
Max RPM               300                 6000 ÷ 20 ✓
Max Torque            ~2.1 Nm             0.105 × 20
```

**Why NOT 112 + 140?**
- 112 = 28 × 4 (only 4:1 cartridge)
- 140 = 28 × 5 (only 5:1 cartridge)
- But you have BOTH stacked, so: 28 × 4 × 5 = 560 ✓

---

## What to Change in the Code

### In all three sketches, change these two lines:

```cpp
// OLD (for direct drive):
#define COUNTS_PER_REV 28
#define MOTOR_MAX_RPM 6000

// NEW (for your 20:1 gearbox):
#define COUNTS_PER_REV 560      // 28 × 20
#define MOTOR_MAX_RPM 300       // 6000 ÷ 20
```

### Also update waveform amplitude:

```cpp
// OLD:
float waveAmplitude = 2000;
float waveOffset = 2000;

// NEW (for 0-300 RPM range):
float waveAmplitude = 150;    // Max is 300 ÷ 2 = 150
float waveOffset = 150;       // Center at 150 RPM
```

### Start P-tuning with different value:

```cpp
// OLD: kP = 0.05 (fast response)
// NEW: kP = 0.02 (slower, for high-inertia system)

float kP = 0.02;   // 20:1 has much higher inertia
```

---

## Why Your 20:1 Setup is Different

### High Inertia = Slower Response

Your 20:1 gearbox has:
- ✓ **20× more torque** (2.1 Nm vs 0.105 Nm)
- ✓ **Much more mechanical inertia** (harder to accelerate)
- ✓ **Easier to control** (stable, won't oscillate easily)
- ✓ **Lower speeds** (0-300 RPM, better for precision)

### PID Tuning Adjustments

| Parameter | Direct Drive | Your 20:1 | Reason |
|-----------|--------------|-----------|--------|
| kP start | 0.05 | 0.02 | Lower for high inertia |
| kD typical | 0.008 | 0.005 | Less damping needed |
| Rise time | 1-2 sec | 3-5 sec | More mass to move |
| Best for | Speed | Torque & precision |

---

## File Organization

### Original Files (for direct drive or 3:1, 4:1, 5:1 alone)
```
01_EncoderTest.ino          (28 CPR, 6000 RPM max)
02_P_Tuning.ino
03_PID_Complete.ino
README.md
PID_SETUP_GUIDE.md
QUICK_REFERENCE.md
pid_monitor.py
```

### NEW Files (for your 20:1 gearbox)
```
01_EncoderTest_20-1Gearbox.ino      ← USE THESE!
02_P_Tuning_20-1Gearbox.ino         ← USE THESE!
03_PID_Complete_20-1Gearbox.ino     ← USE THESE!
GEARBOX_CALCULATION.md              (explains the math)
ESP32_INSTALLATION_FIXES.md         (Arduino fixes)
```

### Use the files ending in `_20-1Gearbox.ino`
They already have:
- ✓ COUNTS_PER_REV = 560
- ✓ MOTOR_MAX_RPM = 300
- ✓ Waveform amplitude = 150
- ✓ kP starts at 0.02
- ✓ Helpful comments about 20:1 tuning

---

## Verification Checklist

Before starting PID tuning:

- [ ] Arduino IDE can find ESP32S3 Dev Module board
- [ ] Serial monitor connects at 115200 baud
- [ ] You're using the `_20-1Gearbox.ino` versions
- [ ] Both 5:1 and 4:1 cartridges physically installed
- [ ] Encoder A & B connected to GPIO 4 & 5
- [ ] Motor runs forward when using FWD command
- [ ] RPM reading increases with PWM increase

---

## Quick Start for Your Setup

```
1. Fix Arduino installation (use ZIP method)
2. Upload 01_EncoderTest_20-1Gearbox.ino
3. Open Serial Monitor (115200 baud)
4. Type: FWD
5. Type: PWM:128 (should see ~150 RPM)
6. Type: PWM:200 (should see ~230 RPM)
7. Confirm RPM matches physical measurement
   
Then continue with Step 2 (P-tuning) and Step 3 (full PID)
```

---

## Expected Performance After Tuning

With your 20:1 gearbox and properly tuned PID:

```
Setpoint change: 0 → 150 RPM
├─ Rise time: 3-4 seconds
├─ Overshoot: <5-10%
├─ Settling time: 1-2 seconds
├─ Steady-state error: <10 RPM
└─ Very smooth, predictable response

Square wave test:
├─ Smooth transitions between 0-300 RPM
├─ No oscillation
├─ Responsive but not jerky
└─ Torque available for load

Typical final gains:
├─ kP = 0.025-0.035
├─ kI = 0.0 (usually not needed)
└─ kD = 0.005-0.010
```

---

## Common Gearbox Mistakes (Avoid These!)

❌ **Wrong**: Using 28 CPR when you have 20:1 gearbox
- Result: Shows 20× higher RPM than actual

❌ **Wrong**: Adding ratios (5 + 4 = 9)
- Result: Wrong encoder counts

❌ **Wrong**: Using direct-drive starting kP (0.05) for 20:1
- Result: System oscillates wildly

✓ **Correct**: 560 CPR, 300 RPM max, kP = 0.02
- Result: Smooth, predictable control

---

## Need More Help?

| Issue | File to Read |
|-------|---|
| Arduino won't install | ESP32_INSTALLATION_FIXES.md |
| Gearbox math unclear | GEARBOX_CALCULATION.md |
| PID tuning process | PID_SETUP_GUIDE.md (original) |
| Commands & pins | QUICK_REFERENCE.md (original) |
| Code implementation | The `_20-1Gearbox.ino` files have comments |

---

## Key Differences: 20:1 vs Direct Drive

### Physical Response

**Direct Drive (28 CPR, 6000 RPM):**
- Responds very quickly
- Can overshoot easily
- Needs aggressive damping (high kD)
- Good for speed control

**Your 20:1 (560 CPR, 300 RPM):**
- Responds more slowly
- Very stable, hard to overshoot
- Needs moderate damping
- Excellent for position & torque control

### Encoder Perspective

**Direct Drive:**
- 28 pulses per output shaft rotation
- Motor shaft = output shaft

**Your 20:1:**
- 560 pulses per output shaft rotation
- Output shaft rotates 20× slower than motor shaft
- 20× more position resolution!

---

## Final Summary Table

```
YOUR CONFIGURATION: 5:1 + 4:1 = 20:1 Gearbox

PARAMETER                    VALUE           WHY
════════════════════════════════════════════════════════════
Gear ratio multiplication    5 × 4           Not addition!
Total reduction             20:1             Stacked cartridges
Motor base CPR              28               Fixed spec
Output shaft CPR            560              28 × 20
Motor max RPM               6000             Fixed spec
Output shaft max RPM        300              6000 ÷ 20
Max torque                  ~2.1 Nm          0.105 × 20
Loop rate                   50 Hz            20ms intervals
PID start: kP               0.02             Conservative for inertia
PID start: kD               0.005            Stability focus
════════════════════════════════════════════════════════════

YOUR SETUP IS PERFECT FOR:
✓ Heavy loads
✓ Precise positioning
✓ Stable speed control
✓ Low-speed high-torque applications
```

---

## Next Steps

1. **Install Arduino** using ZIP method (not board manager)
2. **Upload `01_EncoderTest_20-1Gearbox.ino`**
3. **Verify encoder reads correctly**
4. **Run `02_P_Tuning_20-1Gearbox.ino` with SQUARE wave**
5. **Record your kP value**
6. **Run `03_PID_Complete_20-1Gearbox.ino`**
7. **Add D-gain until smooth response**
8. **Test with SINE and RAMP waves**
9. **Document your final gains**

**Estimated time: 2.5-3 hours total**

---

Good luck with your setup! Your 20:1 gearbox is an excellent choice for a DC motor PID system. The high torque and stability make it easier to tune than a direct drive motor.

Ask questions if anything isn't clear! 🚀
