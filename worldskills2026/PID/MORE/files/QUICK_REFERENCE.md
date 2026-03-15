# Quick Reference Card - PID DC Motor Control

## Pin Configuration (ESP32-S3)

```
ENCODER PINS:
  A Signal  → GPIO 4
  B Signal  → GPIO 5
  GND       → GND

MOTOR CONTROL (Cytron MDD10):
  PWM       → GPIO 9
  DIR       → GPIO 8
  GND       → GND (common return)

POWER:
  +12V  → Cytron power input
  GND   → Common ground (Arduino, Cytron, Motor)
```

## Quick Wiring Checklist

- [ ] Encoder A (yellow) → GPIO 4
- [ ] Encoder B (green) → GPIO 5
- [ ] Encoder GND (black) → ESP32 GND
- [ ] Cytron PWM → GPIO 9
- [ ] Cytron DIR → GPIO 8
- [ ] Cytron GND → ESP32 GND
- [ ] Cytron +12V → 12V supply
- [ ] Motor connectors to Cytron output
- [ ] Pull-up resistors on encoder lines (enabled in code)

---

## Step-by-Step Tuning Commands

### Step 1: Encoder Test (01_EncoderTest.ino)

```
FWD              Start forward rotation
PWM:128          Set motor to 50% speed
PWM:200          Set motor to ~78% speed
STOP             Stop motor
REV              Reverse direction
RESET            Reset encoder counter
HELP             Show all commands
```

**Expected Output:**
- Increasing RPM as PWM increases
- Smooth RPM readings (not erratic)
- Symmetric forward/reverse

---

### Step 2: P-Tuning (02_P_Tuning.ino)

```
SQUARE           Start square wave test (BEST FOR P-TUNING)
SINE             Start sine wave test
STOP             Stop motor

kP:0.03          Very smooth, slow response
kP:0.05          Good balance point
kP:0.08          Fast, might oscillate
kP:0.10          Oscillating (too high)

AMP:2000         Set wave amplitude (RPM)
OFFSET:2000      Set wave center point (RPM)
FREQ:0.2         Set wave frequency (Hz)
```

**Tuning Process:**
1. Start with kP:0.03
2. Increase by 0.01 increments
3. Watch for smooth response (no overshoot)
4. Record kP value when oscillation appears
5. Back off to 80% of that value
6. **Fine-tune by ±0.005**

**Expected P-Tuning Result:**
- Rise time: 2-3 seconds ✓
- Overshoot: <15% ✓
- Settling: 1-2 seconds ✓
- No oscillation ✓

---

### Step 3: Full PID (03_PID_Complete.ino)

```
SQUARE           Start square wave test
SINE             Smooth sine wave test
RAMP             Linear ramp test (best for slope tracking)
STOP             Stop motor

kP:0.06          Your tuned P value from Step 2
kI:0.0           Start with zero
kD:0.003         Very small D to start
kD:0.005         Increase gradually
kD:0.010         Stop if becomes sluggish

INFO             Show current RPM, error, gains
HELP             Show all commands
```

**D-Gain Tuning:**
| kP Range | Start kD | Max kD | Effect |
|----------|----------|--------|--------|
| 0.03-0.04 | 0.003 | 0.006 | Small overshoot |
| 0.05-0.07 | 0.005 | 0.010 | Moderate overshoot |
| 0.08-0.10 | 0.008 | 0.015 | Large overshoot |

---

## CSV Data Format (For Graphing)

Each line contains:
```
Time(s), Setpoint(RPM), Actual(RPM), Error(RPM), PWM, Integral, kP, kI, kD
0.20,    4000.0,        2100.5,      1899.5,     242.1, 18.5, 0.0500, 0.0000, 0.0000
0.40,    4000.0,        3200.3,      799.7,      198.3, 22.1, 0.0500, 0.0000, 0.0000
```

**How to log:**
1. Open serial monitor (115200 baud)
2. Send: `SQUARE` or `SINE` or `RAMP`
3. Copy all CSV lines to file `data.csv`
4. Open in Excel, LibreOffice, or Python

---

## Performance Targets

### Ideal Response Characteristics

| Metric | Good Value | Acceptable Range |
|--------|-----------|------------------|
| Rise Time | 2-3 sec | 1.5-4.0 sec |
| Overshoot | <5% | <15% |
| Settling Time | <1.5 sec | <3 sec |
| Steady-State Error | <100 RPM | <200 RPM |
| RMS Error | <150 RPM | <250 RPM |

### Tuning Indicators

**If response too slow:**
→ Increase kP by 0.01

**If overshoot too high:**
→ Increase kD by 0.002 or reduce kP by 0.01

**If oscillating:**
→ Reduce kP immediately or increase kD

**If steady-state error remains:**
→ Add small kI (0.0005-0.001)

---

## Troubleshooting Quick Guide

### Problem: Motor won't spin
**Check:**
- [ ] 12V power to Cytron?
- [ ] PWM pin connected to Cytron?
- [ ] DIR pin = HIGH for forward?
- [ ] Motor load not too high?

### Problem: Encoder reads nothing
**Check:**
- [ ] Encoder A, B wired to GPIO 4, 5?
- [ ] Encoder GND connected?
- [ ] Motor actually turning?
- [ ] Encoder plugged in?

### Problem: RPM readings jump around
**Fix:**
- Add 100nF capacitors near encoder inputs
- Use shielded encoder cable
- Move encoder wires away from power cables
- Reduce RPM_FILTER_ALPHA to 0.5

### Problem: System oscillates
**Fix:**
- Reduce kP by 0.01-0.02
- Increase kD by 0.005
- Reduce frequency of test signal
- Check for loose mechanical parts

### Problem: Sluggish response
**Fix:**
- Increase kP by 0.01-0.02
- Reduce kD (check if too high)
- Verify motor can deliver required torque
- Check for friction in drivetrain

---

## Motor Specs Reference

**REV-41-1600 HD Hex Motor:**
- Voltage: 12V DC
- Free Speed: 6000 RPM
- Stall Current: 8.5A
- Max Output Power: 15W
- **Encoder: 28 counts/revolution** (at motor shaft)

**With Gearbox:**
- 3:1 reduction → 84 CPR
- 4:1 reduction → 112 CPR
- 5:1 reduction → 140 CPR

**Always set COUNTS_PER_REV in code to match your configuration!**

---

## Recommended Starting Values

```
For DIRECT DRIVE (28 CPR):
  kP = 0.04 - 0.06
  kI = 0.0
  kD = 0.005 - 0.008

For 3:1 GEARBOX (84 CPR):
  kP = 0.05 - 0.07
  kI = 0.0005
  kD = 0.008 - 0.010

For 4:1 GEARBOX (112 CPR):
  kP = 0.06 - 0.08
  kI = 0.001
  kD = 0.010 - 0.012

For 5:1 GEARBOX (140 CPR):
  kP = 0.07 - 0.09
  kI = 0.001
  kD = 0.012 - 0.015
```

---

## Serial Port Tricks

### Finding your COM port

**Windows:**
```
Device Manager → Ports (COM & LPT)
Look for "USB-SERIAL CH340" or similar
```

**Linux/Mac:**
```bash
ls /dev/tty*
Usually: /dev/ttyUSB0 or /dev/ttyACM0
```

### Testing connection
```bash
# Linux/Mac
stty -f /dev/ttyUSB0 115200

# Windows (PowerShell)
[System.IO.Ports.SerialPort]::GetPortNames()
```

---

## Gain Tuning Summary

### Proportional (kP)
- **Purpose**: Fast response to error
- **Effect**: Higher = Faster response but oscillates
- **Tuning**: Increase until oscillation, back off 20%

### Integral (kI)
- **Purpose**: Remove steady-state error
- **Effect**: Slow accumulation, can cause lag
- **Tuning**: Usually very small (0.0001-0.001), add only if needed

### Derivative (kD)
- **Purpose**: Damping, reduce overshoot
- **Effect**: Higher = More damping but sluggish
- **Tuning**: Add only after kP is tuned, typically 0.1× to 0.3× of kP

### Anti-Windup
- Integral bounded to ±100 to prevent unbounded growth
- Integral resets when setpoint is zero

---

## Record Your Final Tuning

```
FINAL TUNED GAINS:
kP = _______
kI = _______
kD = _______

Best waveform response: [ ] SQUARE [ ] SINE [ ] RAMP

Performance achieved:
  Rise time: _______ sec
  Overshoot: _______ %
  Settling time: _______ sec
  Steady-state error: _______ RPM

Notes:
_________________________________________
_________________________________________
_________________________________________
```

---

## Typical Tuning Timeline

| Stage | Duration | Task |
|-------|----------|------|
| Setup | 15 min | Verify wiring, encoder reading |
| Step 1 | 10 min | Test encoder, record RPM |
| Step 2 | 30-45 min | P-gain tuning with square wave |
| Step 3 | 20-30 min | D-gain tuning, test all waves |
| Verification | 15 min | Final performance check |
| **TOTAL** | **~2-3 hours** | Complete PID system |

---

**Last Updated:** 2024
**Tested With:** ESP32-S3, Cytron MDD10, REV-41-1600 Motor
