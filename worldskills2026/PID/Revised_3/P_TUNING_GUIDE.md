# 📖 P-Tuning Guide - 02_P_Tuning_20-1Gearbox.cpp

## Overview

This is **Step 2 of 3** in PID tuning. After encoder verification (Step 1), you'll use this sketch to find the optimal **Proportional gain (kP)** value.

**What it does:**
- Generates test waveforms (square, sine)
- Measures motor response to setpoint changes
- Outputs CSV data for graphing
- Allows real-time kP adjustment

**Time needed:** 30-45 minutes

---

## Before You Start

### ✅ Prerequisites
- [ ] Encoder test (Step 1) completed successfully
- [ ] RPM readings were smooth (120-140 RPM range)
- [ ] Motor runs in both directions
- [ ] Serial monitor working at 115200 baud
- [ ] Python monitor installed (optional but recommended)

### ⚠️ Important Notes
- **Start with low kP values** (0.01-0.02) for 20:1 gearbox
- High inertia = slower response = start conservative
- Avoid kP > 0.05 in first test (may oscillate)

---

## Step 1: Upload the Code

### Using PlatformIO

```bash
# 1. Copy the file
cp 02_P_Tuning_20-1Gearbox.cpp src/main.cpp

# 2. Upload
platformio run --target upload

# 3. Monitor (choose one)
platformio device monitor --baud 115200
# OR use Python script
python pid_monitor.py COM10
```

### Using Arduino IDE

1. Open `02_P_Tuning_20-1Gearbox.ino`
2. Select board: ESP32S3 Dev Module
3. Select port: COM10 (or your port)
4. Upload (Ctrl+U)
5. Open Serial Monitor (Ctrl+Shift+M)

---

## Step 2: Initial Startup

### What You'll See

```
===== P-TUNING & PID FRAMEWORK =====
Configuration: 20:1 Gearbox (5:1 + 4:1)
Motor CPR: 560
Max RPM: 300
Note: Starting kP is lower due to high inertia of 20:1 gearbox

Ready for P-tuning
Commands: SQUARE, SINE, STOP, kP:<value>, HELP
```

**This is normal!** ✓

---

## Step 3: Start Testing - Square Wave

### Command 1: Start Square Wave Test

**Type in Serial Monitor:**
```
SQUARE
```

**You'll see:**
```
Waveform: SQUARE (0.2 Hz, ±150 RPM around 150 RPM center)
Time(s),Setpoint(RPM),Actual(RPM),Error(RPM),PWM,kP,kI,kD
0.20,300.0,150.5,149.5,128.3, 0.02, 0, 0
0.40,300.0,220.3,79.7,140.1, 0.02, 0, 0.005
0.60,0.0,80.1,-80.1,45.2, 0.02, 0, 0
...
```

**What's happening:**
- Setpoint switches between 0 and 300 RPM every 5 seconds
- Motor tries to follow the setpoint
- CSV data is logged for graphing
- Current kP is 0.02 (conservative start)

### Expected Behavior at kP = 0.02

```
TIME 0s:     Setpoint: 0 RPM   → Actual: 0 RPM     (motor off)
TIME 2.5s:   Setpoint jumps to 300 RPM
TIME 3-5s:   Actual slowly ramps up (high inertia)
TIME 5s:     Actual: ~100-150 RPM (still rising)
TIME 7.5s:   Setpoint drops back to 0 RPM
TIME 8-10s:  Actual slowly ramps down
```

**At kP=0.02:**
- Response is **slow** (takes 4-5 seconds to reach setpoint)
- **No overshoot** (good for stability)
- Motor is smooth and predictable

---

## Step 4: Tune kP - Increase Gradually

### Finding the Sweet Spot

**Theory:** Higher kP = faster response, but too high causes oscillation

### Tuning Sequence

#### Test 1: Current Setting (kP = 0.02)
```
SQUARE
```
**Expected:** Slow response, no overshoot
**Record:** "kP=0.02: Takes 4-5 sec to reach 150 RPM"

#### Test 2: Increase to kP = 0.025
```
kP:0.025
```
Wait 5 seconds, observe response
**Expected:** Slightly faster than kP=0.02
**Record:** "kP=0.025: Takes 3-4 sec to reach 150 RPM"

#### Test 3: Increase to kP = 0.03
```
kP:0.03
```
**Expected:** Good response, minimal overshoot
**Record:** "kP=0.03: Takes 2-3 sec, slight overshoot <10%"

#### Test 4: Increase to kP = 0.035
```
kP:0.035
```
**Expected:** Faster, maybe small oscillation
**Record:** "kP=0.035: Takes 2 sec, overshoot ~15%, settles quickly"

#### Test 5: Increase to kP = 0.04
```
kP:0.04
```
**Expected:** May start oscillating
**Record:** "kP=0.04: Overshoots 20%, oscillates 2-3 times"

#### Test 6: Try kP = 0.045
```
kP:0.045
```
**Expected:** Oscillates more
**Record:** "kP=0.045: Bad oscillation, too fast"

---

## Step 5: Identify Oscillation Point

When you see oscillation:
```
Overshoot visible in CSV output:
0.50,300.0,200.0,100.0,...   ← Motor is at 200 RPM (overshooting)
0.52,300.0,280.0,20.0,...    ← Motor jumps to 280 RPM (oscillating!)
0.54,300.0,250.0,50.0,...    ← Back down to 250 RPM
0.56,300.0,290.0,10.0,...    ← Bounces around 300 RPM
```

**The kP where oscillation starts = your upper limit**

### Example Results

| kP Value | Response Time | Overshoot | Oscillation |
|----------|---------------|-----------|-------------|
| 0.020 | 5 sec | None | No |
| 0.025 | 4 sec | ~5% | No |
| 0.030 | 3 sec | ~8% | No ✓ |
| 0.035 | 2.5 sec | ~15% | Slight |
| 0.040 | 2 sec | ~20% | Yes |

**Best choice:** kP = 0.03 (stable, responsive)

---

## Step 6: Record Your Best kP

**After finding the best value:**

```bash
# Final test - run 2-3 more squares with your best kP
kP:0.03
SQUARE
(wait 20 seconds for 3-4 complete cycles)
STOP
```

**Save this value!** You'll use it in Step 3 (Full PID)

---

## Understanding the CSV Output

### Column by Column

```
Time(s), Setpoint(RPM), Actual(RPM), Error(RPM), PWM, kP, kI, kD
0.20   ,  300.0      ,  150.5     ,  149.5    , 128.3, 0.02, 0, 0
```

| Column | Meaning | Notes |
|--------|---------|-------|
| Time | Elapsed seconds | Starts at 0 |
| Setpoint | Target RPM | 0-300 (square wave) |
| Actual | Measured RPM | What motor is doing |
| Error | Setpoint - Actual | How far off we are |
| PWM | Control signal | 0-255 (duty cycle) |
| kP | P-gain | Value you're tuning |
| kI | I-gain | Always 0 in P-tuning |
| kD | D-gain | Always 0 in P-tuning |

---

## All Commands

### Motion Commands
```
SQUARE          Start square wave test (0-300 RPM, 5 sec each)
SINE            Start sine wave test (smooth 0-300 RPM)
STOP            Stop motor immediately
```

### Tuning Commands
```
kP:0.02         Set P-gain (replace 0.02 with your value)
kP:0.025        Increase slightly
kP:0.03         Try this value
kP:0.035        Higher
kP:0.04         Upper limit (may oscillate)
```

### Parameter Commands
```
AMP:150         Set amplitude in RPM (default 150, max 150)
FREQ:0.2        Set frequency in Hz (default 0.2 = 5 sec period)
```

### Info Commands
```
HELP            Show all available commands
```

---

## What Good P-Tuning Looks Like

### CSV Data Markers

**Good response:**
```
Time=0.0:   Setpoint jumps 0→300, Actual=0, Error=300
Time=1.0:   Setpoint=300, Actual≈50, Error≈250 (moving!)
Time=2.0:   Setpoint=300, Actual≈120, Error≈180 (getting closer)
Time=3.0:   Setpoint=300, Actual≈180, Error≈120 (halfway there)
Time=4.0:   Setpoint=300, Actual≈230, Error≈70 (almost there)
Time=5.0:   Setpoint=300, Actual≈270, Error≈30 (close!)
Time=5.5:   Setpoint drops 300→0, Actual starts decreasing
```

**Bad response (oscillating):**
```
Time=2.0:   Setpoint=300, Actual=150, Error=150
Time=2.5:   Setpoint=300, Actual=280, Error=20 (jumped too far!)
Time=3.0:   Setpoint=300, Actual=200, Error=100 (bounced back!)
Time=3.5:   Setpoint=300, Actual=290, Error=10 (jumped again!)
        ← This is oscillation, kP is too high
```

---

## Graphing Your Data (Optional)

### Export Data to CSV

1. Run SQUARE test for 20+ seconds
2. Select all output (Ctrl+A in serial monitor)
3. Copy (Ctrl+C)
4. Paste into Excel/Google Sheets

### Plot in Excel

1. Create columns: Time, Setpoint, Actual, Error, PWM
2. Paste CSV data
3. Select data → Insert Chart
4. Choose Line Chart

### Plot in Python

```python
import pandas as pd
import matplotlib.pyplot as plt

# Read CSV
df = pd.read_csv('pid_data.csv')

# Plot
plt.figure(figsize=(12, 6))
plt.plot(df['Time'], df['Setpoint'], label='Setpoint', linewidth=2)
plt.plot(df['Time'], df['Actual'], label='Actual', linewidth=2)
plt.xlabel('Time (seconds)')
plt.ylabel('RPM')
plt.legend()
plt.grid(True)
plt.show()
```

---

## Troubleshooting

### Problem: Motor Doesn't Move

**Possible causes:**
- kP too low (try kP:0.05 temporarily)
- Motor not connected
- Power supply issue
- Encoder wires loose

**Fix:**
```
1. Check physical connections
2. Upload Step 1 encoder test
3. If encoder test works, come back to Step 2
```

### Problem: Motor Oscillates at kP=0.02

**This shouldn't happen!** Something is wrong.

**Possible causes:**
- COUNTS_PER_REV is wrong
- Encoder connected to wrong pins
- Motor has mechanical issue

**Fix:**
```
1. Verify COUNTS_PER_REV = 560
2. Check GPIO 4, 5 connections
3. Manually spin wheel, listen for smooth encoder clicks
4. Re-upload Step 1 encoder test to verify
```

### Problem: Serial Monitor Shows Garbage

**Wrong baud rate!**

**Fix:**
```
Serial Monitor bottom-right corner → 115200 baud
(Must match Serial.begin(115200) in code)
```

### Problem: "PWM set to" message appears

**You typed PWM command from Step 1!**

**Fix:**
```
This sketch doesn't have PWM commands.
Use: kP:0.03 instead
Type: HELP to see valid commands
```

---

## Performance Targets for 20:1 Gearbox

| Metric | Target | Why |
|--------|--------|-----|
| Rise time | 2-4 seconds | High inertia, slow response expected |
| Overshoot | <10% | Stable system preferred |
| Settling time | <2 seconds | Should stabilize after overshoot |
| Oscillation | None | Indicates kP too high |
| Best kP range | 0.025-0.035 | For your 20:1 setup |

---

## When to Move to Step 3

You're ready for Step 3 (Full PID) when:

✅ You have a stable kP value (no oscillation)
✅ Response time is reasonable (2-4 seconds)
✅ You've recorded your kP value
✅ You've tested it 2-3 times to confirm stability
✅ CSV data shows smooth response curves

---

## Example: Complete P-Tuning Session

```
User: SQUARE
System: Waveform: SQUARE (0.2 Hz, ±150 RPM around 150 RPM center)
        Time(s),Setpoint(RPM),Actual(RPM),Error(RPM),PWM,kP,kI,kD
        
        (20 seconds of smooth data appear)
        
        0.20,300.0,150.5,149.5,128.3, 0.02, 0, 0
        0.40,300.0,220.3,79.7,140.1, 0.02, 0, 0
        ... (slow rise to 300 RPM)
        5.00,300.0,290.0,10.0,240.0, 0.02, 0, 0
        5.20,0.0,280.0,-280.0,20.1, 0.02, 0, 0
        (slow drop back to 0)

User: (observes: kP=0.02 is too slow, takes 4-5 seconds)

User: kP:0.03
User: SQUARE

        (CSV data starts again with kP=0.03)
        0.20,300.0,180.0,120.0,150.0, 0.03, 0, 0
        0.40,300.0,240.0,60.0,170.0, 0.03, 0, 0
        ... (faster rise)
        3.00,300.0,285.0,15.0,180.0, 0.03, 0, 0
        3.20,0.0,270.0,-270.0,30.0, 0.03, 0, 0

User: (observes: kP=0.03 is good! Reaches 285 RPM in 3 seconds)

User: kP:0.035
User: SQUARE

        (CSV shows slight overshoot to ~320 RPM)
        
User: (observes: kP=0.035 overshoots by 20 RPM, still acceptable)

User: kP:0.04
User: SQUARE

        (CSV shows oscillation around 300 RPM)
        
User: (observes: kP=0.04 oscillates, too high)

User: FINAL DECISION: kP = 0.03 (best balance)
User: kP:0.03
User: SQUARE
(wait 20 seconds to verify)
User: STOP

RECORDED: Best kP = 0.03 for 20:1 gearbox
NEXT: Move to Step 3 with kP:0.03
```

---

## Summary

| Step | What to Do | Time |
|------|-----------|------|
| 1 | Upload code | 2 min |
| 2 | Type SQUARE | 1 min |
| 3 | Observe response at kP=0.02 | 2 min |
| 4 | Increase kP by 0.005 increments | 30 min |
| 5 | Find sweet spot (no oscillation) | 5 min |
| 6 | Final verification | 3 min |
| | **Total** | **45 min** |

---

## Next Step

Once you have your **best kP value**, move to:

**Step 3: 03_PID_Complete_20-1Gearbox.cpp**
- Add D-gain for smoothness
- Optional I-gain for steady-state error
- Final tuning with all three gains

---

**Questions?** Check the CHEAT_SHEET.md or PID_SETUP_GUIDE.md

Good luck with your P-tuning! 🚀
