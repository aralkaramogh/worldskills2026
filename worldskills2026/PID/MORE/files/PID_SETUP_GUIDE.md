# ESP32-S3 PID DC Motor Control - Complete Guide

## Hardware Setup

### Wiring Diagram
```
ESP32-S3 Pinout (Adjust pins as needed):
┌─────────────────────────────────────────┐
│ Encoder A  → GPIO 4 (with pull-up)      │
│ Encoder B  → GPIO 5 (with pull-up)      │
│ Motor PWM  → GPIO 9 (via Cytron)        │
│ Motor DIR  → GPIO 8 (via Cytron)        │
│ GND        → GND (common with Cytron)   │
└─────────────────────────────────────────┘

Cytron MDD10 Motor Driver:
┌──────────────────────┐
│ PWM  ← GPIO 9        │
│ DIR  ← GPIO 8        │
│ GND  ← ESP32 GND     │
│ +12V ← Power Supply  │
│ MOTOR OUT → DC Motor │
└──────────────────────┘

DC Motor (REV-41-1600):
┌─────────────────────────────┐
│ Motor Power ← Cytron        │
│ Encoder A   → GPIO 4        │
│ Encoder B   → GPIO 5        │
│ Encoder GND → ESP32 GND     │
└─────────────────────────────┘
```

### Component List
- ESP32-S3 DevKit
- Cytron MDD10 Motor Driver (10A PWM DC Motor Driver)
- REV-41-1600 HD Hex Motor with encoder (28 CPR)
- Power supply: 12V DC (minimum 3A recommended)
- Encoder cables: A & B to GPIO 4 & 5
- Motor control cables to Cytron

### Encoder Counts Per Revolution
- **Direct drive**: 28 CPR
- **With 3:1 gearbox**: 28 × 3 = 84 CPR
- **With 4:1 gearbox**: 28 × 4 = 112 CPR
- **With 5:1 gearbox**: 28 × 5 = 140 CPR

Change `COUNTS_PER_REV` in the code to match your configuration.

---

## Step 1: Encoder Verification (01_EncoderTest.ino)

### Purpose
Verify that the encoder is reading correctly and calculate RPM accurately.

### Instructions

1. **Upload the sketch**: Load `01_EncoderTest.ino` to your ESP32-S3

2. **Open Serial Monitor**: 115200 baud

3. **Commands to try**:
   ```
   PWM:128      → Motor at 50% speed (should see RPM increase)
   PWM:200      → Motor at ~78% speed
   STOP         → Stop motor (RPM should drop to 0)
   FWD          → Forward direction
   REV          → Reverse direction
   RESET        → Reset encoder counter
   HELP         → Show all commands
   ```

4. **Expected output**:
   ```
   Time(ms)    Counts  Delta   RPM     PWM%
   ================================================
   100         17      17      726.4   50.2
   200         35      18      770.3   50.2
   300         51      16      685.7   50.2
   ```

### Troubleshooting

**No RPM reading?**
- Check encoder connections (A & B wires)
- Verify pull-up resistors are enabled
- Motor may not be running - increase PWM value
- Check direction pin (DIR) - motor won't turn if stuck in neutral

**Erratic RPM readings?**
- Noisy encoder signals - add filtering or shielded cables
- Interrupt conflicts - check other components

**Backward RPM values?**
- Reverse the encoder A/B connections
- Or add a negative sign to the count logic in encoderISR()

---

## Step 2: P-Gain Tuning (02_P_Tuning.ino)

### Purpose
Tune the proportional gain (kP) to achieve fast response with minimal overshoot.

### Instructions

1. **Upload sketch**: Load `02_P_Tuning.ino`

2. **Start with square wave**:
   ```
   SQUARE       → Start square wave oscillation
   ```
   You should see output like:
   ```
   Time(s),Setpoint(RPM),Actual(RPM),Error(RPM),PWM,kP,kI,kD
   0.00,4000.0,0.0,4000.0,255.0,0.0500,0.0000,0.0000
   0.20,4000.0,1250.5,2749.5,248.3,0.0500,0.0000,0.0000
   0.40,4000.0,2100.3,1899.7,242.1,0.0500,0.0000,0.0000
   ```

3. **Open in graphing tool** (e.g., Excel, Python, Gnuplot):
   - Copy serial output to a file (Time.csv)
   - Plot: Setpoint vs Actual RPM
   - Look for rise time, overshoot, and settling time

4. **Tune kP incrementally**:
   ```
   kP:0.03      → Too sluggish? Error decays slowly
   kP:0.05      → Better response but maybe some overshoot
   kP:0.08      → Starts oscillating (too high!)
   kP:0.06      → Sweet spot - fast response, stable
   ```

5. **What to look for**:
   - **Rise time**: How fast it reaches setpoint (~2-3 seconds OK)
   - **Overshoot**: Should not exceed 10-15% of setpoint
   - **Oscillation**: No ringing around target value
   - **Settling time**: Stabilizes within 1-2 seconds

### P-Tuning Quick Reference

| kP Value | Behavior |
|----------|----------|
| 0.01-0.02 | Very sluggish, slow response |
| 0.03-0.04 | Good for stiff systems, smooth |
| 0.05-0.07 | Fast response, slight overshoot |
| 0.08-0.10 | Quick but oscillating |
| 0.12+ | Unstable, oscillates heavily |

### Finding Optimal kP

1. **Start at kP=0.03**
2. **Increase by 0.01** each test
3. **Watch for oscillation** in the graphs
4. **When oscillation appears**, back off to 80% of that value
5. **Fine-tune by ±0.005** for best response

**Record your optimal kP value** - you'll use this in Step 3.

---

## Step 3: Full PID Tuning (03_PID_Complete.ino)

### Purpose
Add derivative (D) and optionally integral (I) gains for improved performance.

### Instructions

1. **Upload sketch**: Load `03_PID_Complete.ino`

2. **Set your tuned kP**:
   ```
   kP:0.06      → Your value from Step 2
   kI:0.0       → Keep zero for now
   kD:0.0       → Start with zero
   ```

3. **Test with square wave**:
   ```
   SQUARE       → Run square wave
   ```

4. **Add D-gain to reduce overshoot**:
   ```
   kD:0.003     → Start small
   kD:0.005     → Increase gradually
   kD:0.010     → Stop if oscillation appears
   ```

5. **Monitor the effect**:
   - **Too little D**: Still has overshoot
   - **Right D**: Overshoot reduced to <5%, smooth settling
   - **Too much D**: System becomes sluggish, laggy

6. **Optional: Add I-gain** (only if steady-state error remains):
   ```
   kI:0.0005    → Very small to start
   kI:0.001     → Increase only if needed
   ```
   **Warning**: Too much I causes oscillation. Use sparingly.

7. **Test with different waveforms**:
   ```
   SINE         → Verify smooth tracking
   RAMP         → Check ramp following
   ```

### Recommended D-Gain Ranges

| System Response | Recommended kD |
|-----------------|-----------------|
| Very slow (kP<0.03) | 0.003-0.005 |
| Moderate (kP=0.05-0.07) | 0.005-0.010 |
| Fast (kP>0.08) | 0.010-0.015 |

### Final PID Gains Examples

```
Conservative (smooth, stable):
  kP=0.04, kI=0.0, kD=0.005

Balanced (good response and stability):
  kP=0.06, kI=0.0005, kD=0.008

Aggressive (fast response):
  kP=0.08, kI=0.001, kD=0.012
```

---

## Graphical Monitoring

### Using Python (Recommended)

Create a file `plot_pid.py`:

```python
import matplotlib.pyplot as plt
import pandas as pd
import serial

# Configure serial
ser = serial.Serial('COM3', 115200, timeout=1)  # Change COM3 to your port

# Send command to start data logging
ser.write(b'SQUARE\n')

data = []
times = []

try:
    print("Logging data... Press Ctrl+C to stop")
    while True:
        if ser.in_waiting:
            line = ser.readline().decode('utf-8').strip()
            if line and line[0].isdigit():  # CSV data starts with number
                parts = line.split(',')
                if len(parts) >= 5:
                    times.append(float(parts[0]))
                    data.append({
                        'time': float(parts[0]),
                        'setpoint': float(parts[1]),
                        'actual': float(parts[2]),
                        'error': float(parts[3]),
                        'pwm': float(parts[4])
                    })
except KeyboardInterrupt:
    print("Stopped logging")

# Plot results
if data:
    df = pd.DataFrame(data)
    
    fig, axes = plt.subplots(2, 2, figsize=(12, 8))
    
    # Setpoint vs Actual
    axes[0, 0].plot(df['time'], df['setpoint'], 'r-', label='Setpoint')
    axes[0, 0].plot(df['time'], df['actual'], 'b-', label='Actual')
    axes[0, 0].set_ylabel('RPM')
    axes[0, 0].set_title('Speed Response')
    axes[0, 0].legend()
    axes[0, 0].grid(True)
    
    # Error
    axes[0, 1].plot(df['time'], df['error'], 'g-')
    axes[0, 1].set_ylabel('Error (RPM)')
    axes[0, 1].set_title('Tracking Error')
    axes[0, 1].grid(True)
    
    # PWM
    axes[1, 0].plot(df['time'], df['pwm'], 'purple')
    axes[1, 0].set_ylabel('PWM Value')
    axes[1, 0].set_xlabel('Time (s)')
    axes[1, 0].set_title('Motor Command')
    axes[1, 0].grid(True)
    
    # Error histogram
    axes[1, 1].hist(df['error'], bins=20, edgecolor='black')
    axes[1, 1].set_xlabel('Error (RPM)')
    axes[1, 1].set_title('Error Distribution')
    
    plt.tight_layout()
    plt.show()

ser.close()
```

### Using Excel/Spreadsheet

1. Copy serial output to a text file
2. Import into Excel/LibreOffice Calc
3. Use "Text to Columns" with comma delimiter
4. Create charts:
   - **Line chart**: Setpoint vs Actual vs Time
   - **Line chart**: Error vs Time
   - **Line chart**: PWM vs Time

---

## Tuning Summary Checklist

- [ ] **Step 1**: Encoder reads correctly, RPM stable
- [ ] **Step 2**: P-gain tuned, square wave response acceptable
- [ ] **Step 3**: D-gain reduces overshoot <5%
- [ ] **All waveforms**: Response is stable (SQUARE, SINE, RAMP)
- [ ] **No oscillation**: System doesn't ring or chatter
- [ ] **Record gains**: kP, kI, kD values documented

---

## Troubleshooting Common Issues

### Motor won't start
- Check PWM pin connection to Cytron
- Verify 12V power supply to motor driver
- Confirm DIR pin is high for forward

### RPM reading incorrect
- Verify encoder connections A & B
- Check COUNTS_PER_REV matches your motor/gearbox
- Motor speed correct? Measure physical RPM

### Oscillation/overshoot
- Reduce kP value (too aggressive)
- Increase D-gain (if not already tuned)
- Check for loose mechanical connections

### Sluggish response
- Increase kP value
- Reduce D-gain (might be too high)
- Check if motor has sufficient torque

### Noisy encoder readings
- Add 100nF capacitors close to encoder pins
- Use shielded encoder cable
- Keep encoder wires away from power cables

---

## Performance Tips

1. **Bandwidth**: Higher loop rate (50Hz+) = better control
2. **Filtering**: RPM_FILTER_ALPHA=0.7 balances responsiveness vs noise
3. **Anti-windup**: Integral limits prevent unbounded growth
4. **Derivative filtering**: Smooths derivative term to reduce noise
5. **Feedforward**: Base PWM from setpoint improves transient response

---

## Next Steps

Once PID is tuned:
1. Log motor current for efficiency analysis
2. Add trajectory following (ramps, S-curves)
3. Implement multiple speed profiles
4. Add fault detection (stalled rotor, open encoder)
5. Optimize power consumption

---

## Reference Documentation

- **REV Robotics Docs**: https://docs.revrobotics.com/duo-build
- **Cytron MDD10**: https://www.cytron.io/products/mdd10-10-amp-dc-motor-driver
- **PID Control**: See below

---

## PID Control Fundamentals

### The PID Equation
```
u(t) = kP × e(t) + kI × ∫e(t)dt + kD × de(t)/dt
```

Where:
- `e(t)` = error (setpoint - actual)
- `u(t)` = control output (0-255 PWM)
- `kP` = proportional gain (fast response)
- `kI` = integral gain (steady-state error)
- `kD` = derivative gain (damping)

### Ziegler-Nichols Quick Method

1. Set kI=0, kD=0
2. Increase kP until system oscillates (critical gain kP_crit)
3. Measure oscillation period (T_osc)

```
Tuned values:
  kP = 0.6 × kP_crit
  kI = 1.2 × kP_crit / T_osc
  kD = 0.075 × kP_crit × T_osc
```

---

Good luck with your PID tuning! Feel free to iterate and experiment.
