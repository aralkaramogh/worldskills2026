# ESP32-S3 PID DC Motor Control - Project Overview

## What You're Getting

This is a **complete, step-by-step implementation** for controlling a DC motor with PID on ESP32-S3. The approach is educational and practical, using three progressive sketches:

### 📁 Files Included

```
01_EncoderTest.ino          → Step 1: Verify encoder and RPM reading
02_P_Tuning.ino             → Step 2: Tune proportional gain with square/sine waves
03_PID_Complete.ino         → Step 3: Full PID with D and optional I gains
PID_SETUP_GUIDE.md          → Complete hardware setup and tuning methodology
QUICK_REFERENCE.md          → Commands, pins, and quick troubleshooting
pid_monitor.py              → Python real-time graphing tool (optional but recommended)
```

---

## The 3-Step Tuning Process

### **Step 1: Encoder Verification (10 minutes)**
```
Goal: Confirm encoder reads correctly and RPM calculation is accurate
Upload: 01_EncoderTest.ino
Test: Change PWM, verify RPM increases smoothly
Commands: PWM:128, PWM:200, FWD, REV, STOP
```

**Success Criteria:**
- ✓ RPM increases with PWM
- ✓ No erratic jumps in readings
- ✓ Symmetric forward/reverse

---

### **Step 2: P-Tuning (30-45 minutes)**
```
Goal: Find optimal proportional gain (kP) for fast, stable response
Upload: 02_P_Tuning.ino
Test: Use square wave with gradually increasing kP
Commands: SQUARE, kP:0.05, kP:0.06, etc.
Output: CSV data for graphing
```

**Success Criteria:**
- ✓ Response reaches setpoint in 2-3 seconds
- ✓ Overshoot <15%
- ✓ No oscillation around target
- ✓ Record your best kP value

**Typical kP Range:** 0.04 - 0.08

---

### **Step 3: D-Tuning (20-30 minutes)**
```
Goal: Add derivative gain to reduce overshoot and improve stability
Upload: 03_PID_Complete.ino
Set: Your kP from Step 2
Test: Square and sine waves with increasing kD
Commands: SQUARE, kD:0.005, kD:0.008, SINE, RAMP
Output: CSV data for graphing
```

**Success Criteria:**
- ✓ Overshoot reduced to <5%
- ✓ Response still fast (not sluggish)
- ✓ Smooth tracking with sine wave
- ✓ Stable ramp following

**Typical kD Range:** 0.003 - 0.012

---

## Quick Start (5 minutes)

### 1. **Install Arduino IDE for ESP32**
```
1. Open Arduino IDE
2. File → Preferences → Additional Board Managers URLs
3. Add: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
4. Tools → Board Manager → Search "esp32" → Install
5. Tools → Board → ESP32S3 Dev Module
```

### 2. **Hardware Setup**
- Connect encoder A → GPIO 4
- Connect encoder B → GPIO 5
- Connect motor PWM → GPIO 9
- Connect motor DIR → GPIO 8
- Connect common GND
- Connect 12V power to Cytron

### 3. **Upload & Test**
- Copy `01_EncoderTest.ino` into Arduino IDE
- Click Upload
- Open Serial Monitor (115200 baud)
- Type `FWD` and `PWM:128` to test

---

## Hardware Wiring Diagram

```
           ESP32-S3
      ┌────────────────┐
GPIO4 │    ENCODER A ◄─┼──── Yellow wire (encoder)
GPIO5 │    ENCODER B ◄─┼──── Green wire (encoder)
GND   │      GND    ◄─┼──── Black wire (encoder)
GPIO9 │    PWM      ──┼──► Cytron PWM
GPIO8 │    DIR      ──┼──► Cytron DIR
GND   │      GND    ──┼──► Cytron GND (common return)
      └────────────────┘
              │
              │ USB (programming)
              │
       ┌──────────────────┐
       │  Cytron MDD10    │
       │  Motor Driver    │
  ─────┤ PWM, DIR, GND   │─────
       │                  │
    +12V ─────────────────┤ +12V
       │                  │
    OUT1 ──┬──────────────┤ Motor
    OUT2 ──┘
       └──────────────────┘
```

---

## Typical Tuning Results

After complete tuning, expect:
- **Rise time**: 2-3 seconds to reach setpoint
- **Overshoot**: <5-10% above target
- **Settling time**: 1-2 seconds to stabilize
- **Steady-state error**: <100 RPM
- **Smooth tracking**: With sine and ramp inputs

### Example Final Gains
```
Conservative (smooth):       Balanced (recommended):    Aggressive (fast):
kP = 0.04                    kP = 0.06                  kP = 0.08
kI = 0.0                     kI = 0.0                   kI = 0.001
kD = 0.005                   kD = 0.008                 kD = 0.012
```

---

## Using the Python Graphing Tool (Optional)

Real-time monitoring and analysis:

```bash
# Install requirements (one time)
pip install pyserial matplotlib pandas numpy

# Run monitor
python pid_monitor.py COM3

# Then in Arduino serial monitor:
# Type: SQUARE
# The python script will plot in real-time
```

This shows:
- Speed response (setpoint vs actual)
- Tracking error over time
- PWM command and integral term
- Live performance metrics

---

## Troubleshooting Checklist

### Motor won't run?
- [ ] 12V power connected to Cytron?
- [ ] PWM and DIR pins connected?
- [ ] Check if DIR pin is toggling (use Serial.print)
- [ ] Motor load not too high?

### Encoder not reading?
- [ ] A & B wires on GPIO 4 & 5?
- [ ] GND connected?
- [ ] Encoder cable plugged into motor?
- [ ] Check with Step 1 sketch first

### Noisy RPM readings?
- [ ] Add 100nF capacitors to encoder inputs
- [ ] Use shielded encoder cable
- [ ] Reduce RPM_FILTER_ALPHA to 0.5
- [ ] Keep encoder wires away from motor power

### System oscillates?
- [ ] Reduce kP by 0.01
- [ ] Increase kD by 0.005
- [ ] Check for mechanical issues (loose joints)
- [ ] Verify encoder wiring is solid

---

## Code Customization

### Change Motor Encoder CPR
In all three sketches, find this line:
```cpp
#define COUNTS_PER_REV 28  // Change based on gearbox
```

Replace with your configuration:
- **28**: Direct drive (no gearbox)
- **84**: With 3:1 gearbox
- **112**: With 4:1 gearbox
- **140**: With 5:1 gearbox

### Change Pin Assignments
At the top of each sketch:
```cpp
#define ENCODER_A_PIN 4      // Change these
#define ENCODER_B_PIN 5
#define MOTOR_PWM_PIN 9
#define MOTOR_DIR_PIN 8
```

### Adjust Motor Max RPM
```cpp
#define MAX_RPM_OUTPUT 6000  // Your motor's free speed
```

---

## Performance Tuning Guide

### If Response is Too Slow
1. Increase **kP** by 0.01
2. Test with SQUARE wave
3. Repeat until you see slight overshoot
4. Back off kP by 0.005

### If Response Overshoots Too Much
1. Increase **kD** by 0.002-0.005
2. Test response again
3. Or slightly reduce **kP** by 0.005

### If There's Steady-State Error
1. Add tiny amount of **kI** (start 0.0001)
2. Increase slowly (0.0005, 0.001, etc.)
3. **Warning**: Too much kI causes oscillation

### If System Is Sluggish
1. Increase **kP** (faster response)
2. Reduce **kD** (remove damping)
3. Check motor can deliver required torque

---

## What Each Sketch Does

### 01_EncoderTest.ino
**Purpose**: Verify encoder hardware and calibration
- Reads quadrature encoder signals
- Calculates RPM continuously
- Allows manual PWM speed control
- **Output**: Time, encoder count, delta, RPM
- **Commands**: PWM, FWD, REV, STOP, RESET

### 02_P_Tuning.ino
**Purpose**: Tune proportional gain
- Generates square and sine wave setpoints
- Implements basic PID (with I and D at 0)
- **Output**: CSV format for graphing
- **Commands**: SQUARE, SINE, kP, AMP, FREQ
- **Best for**: Finding optimal P gain

### 03_PID_Complete.ino
**Purpose**: Full PID implementation with all three terms
- Robust quadrature decoding
- Anti-windup integral limiting
- Derivative filtering
- Multiple test waveforms
- Real-time parameter adjustment
- **Output**: CSV with all terms
- **Commands**: All previous + kI, kD, INFO

---

## Next Steps After Tuning

Once your PID is tuned:

1. **Save your gains** in a comment in the code
2. **Test in real conditions** with your actual load
3. **Implement safety checks** (stall detection, etc.)
4. **Add trajectory generation** (S-curves, ramps)
5. **Monitor motor current** for efficiency
6. **Log performance data** for analysis

---

## Common Questions

**Q: What's a good starting kP value?**
A: Start with 0.05. If response is slow, increase to 0.07. If it oscillates, reduce to 0.03.

**Q: Do I need kI?**
A: Usually not. Only add if there's persistent steady-state error after P and D are tuned.

**Q: Why does D-gain make it smoother?**
A: D responds to the rate of change of error. High error rate = high correction. This damps oscillations.

**Q: How do I know if my encoder is working?**
A: Run Step 1, increase PWM, and verify RPM increases smoothly and proportionally.

**Q: Can I use different GPIO pins?**
A: Yes, but interrupt-capable pins are recommended for encoder (most ESP32 pins are). PWM can be any pin.

---

## Resources

- **ESP32 Datasheet**: https://www.espressif.com/
- **Cytron MDD10**: https://www.cytron.io/products/mdd10-10-amp-dc-motor-driver
- **REV Robotics Docs**: https://docs.revrobotics.com/
- **PID Control Tutorial**: https://en.wikipedia.org/wiki/PID_controller
- **Arduino Reference**: https://www.arduino.cc/reference/en/

---

## Estimated Timeline

| Phase | Time | Activity |
|-------|------|----------|
| Setup | 15 min | Install IDE, connect hardware |
| Step 1 | 10 min | Upload encoder test, verify reads |
| Step 2 | 45 min | P-tuning with square waves |
| Step 3 | 30 min | D-tuning with multiple waveforms |
| Verification | 15 min | Final performance check |
| **TOTAL** | **~2.5 hours** | Complete PID system |

---

## File Sizes & Memory

All three sketches fit easily on ESP32-S3:
- **Flash**: <30 KB (plenty of room)
- **RAM**: <10 KB for all variables
- **Loop rate**: 50 Hz (20ms timing)

---

## Version History

```
v1.0 - Initial release
- 3-step tuning methodology
- Complete PID with anti-windup
- Python monitoring tool
- Comprehensive documentation
```

---

## Support & Debugging

If something doesn't work:

1. **Check the QUICK_REFERENCE.md** for commands
2. **Review PID_SETUP_GUIDE.md** for detailed setup
3. **Enable debugging** in the sketches (uncomment Serial.println lines)
4. **Test components individually** (encoder first, then motor control)
5. **Use the Python tool** to visualize what's happening

---

## Final Notes

- **Safety first**: Always check motor spins correctly before leaving it running
- **Power supply**: Use adequate 12V power supply (3A minimum recommended)
- **Cable management**: Keep encoder cables away from motor power to reduce noise
- **Iteration**: PID tuning is iterative - small changes make big differences
- **Documentation**: Write down your final gains for future reference

---

Good luck with your PID tuning! The methodology here is battle-tested and should work well for your setup.

Questions? Check the detailed guides or the code comments!
