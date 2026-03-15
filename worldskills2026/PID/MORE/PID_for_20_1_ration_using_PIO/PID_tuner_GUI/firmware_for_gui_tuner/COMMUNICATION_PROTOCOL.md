# 🔌 GUI ↔ Firmware Communication Protocol

## Quick Reference

### Baud Rate
```
115200 bps (8 bits, no parity, 1 stop bit)
```

---

## Commands: GUI → Firmware

### Start Tests
```
Command:   SQUARE
Action:    Start square wave (0 ↔ 300 RPM, 0.2 Hz, 5 sec period)
Response:  None (firmware starts publishing CSV)

Command:   SINE
Action:    Start sine wave (smooth 0-300 RPM oscillation)
Response:  None

Command:   RAMP
Action:    Start ramp (0→300 in 2.5 sec, hold 2.5 sec)
Response:  None
```

### Stop Motor
```
Command:   STOP
Action:    Immediate stop, clear all setpoints
Response:  None (motor stops immediately)
```

### Set Gains
```
Command:   kP:0.035
Action:    Set proportional gain to 0.035
Response:  None (gains take effect immediately)

Command:   kI:0.002
Action:    Set integral gain to 0.002
Response:  None

Command:   kD:0.008
Action:    Set derivative gain to 0.008
Response:  None

Format:    gain_name:value
Examples:  kP:0.02    kI:0.001    kD:0.005
```

### System Commands
```
Command:   RESET
Action:    Stop motor, reset gains to defaults (kP=0.02, kI=0, kD=0)
Response:  None

Command:   STATUS
Action:    Print current parameter values
Response:  [STATUS] kP=0.0300 kI=0.0000 kD=0.0000
```

---

## Data: Firmware → GUI

### CSV Output (Every 50ms)
```
Format:  Time(s),Setpoint(RPM),Actual(RPM),Error(RPM),PWM(%),kP,kI,kD

Example:
0.05,150.0,47.3,102.7,43,0.0300,0.0000,0.0000
0.10,150.0,85.4,64.6,61,0.0300,0.0000,0.0000
0.15,150.0,125.2,24.8,78,0.0300,0.0000,0.0000

Columns:
┌─────────┬──────────────┬────────────┬────────────┬────────┬────┬────┬────┐
│ Time(s) │ Setpoint RPM │ Actual RPM │ Error RPM  │ PWM %  │ kP │ kI │ kD │
├─────────┼──────────────┼────────────┼────────────┼────────┼────┼────┼────┤
│ 0.05    │ 150.0        │ 47.3       │ 102.7      │ 43     │0.03│0.00│0.00│
└─────────┴──────────────┴────────────┴────────────┴────────┴────┴────┴────┘
```

### Boot Message (Once at Startup)
```
=================================================================
  PID Motor Tuner - GUI Compatible Firmware v1.0
  ESP32-S3 + Cytron DD10A + 20:1 Gearbox
=================================================================
TICKS_PER_REV: 2240
MAX_RPM: 300

CSV Header (for GUI parsing):
Time(s),Setpoint(RPM),Actual(RPM),Error(RPM),PWM(%),kP,kI,kD

Ready for GUI commands...
=================================================================
```

---

## Communication Diagram

```
┌──────────────────────────────────────────────────────────────┐
│                      Python GUI                              │
│  (pid_tuner_gui.py)                                          │
│                                                              │
│  ┌────────────────────────────────────────┐                │
│  │ User adjusts kP to 0.035               │                │
│  │ Clicks "Send kP" button                │                │
│  └────────────────────┬───────────────────┘                │
│                       │                                     │
│                       ├─ Serial TX (115200)  ────►          │
│                       │                                     │
│                   "kP:0.035\n"                              │
│                       │                                     │
│                       │  ◄─ Serial RX (20 Hz)  ◄────       │
│                       │                                     │
│                   CSV data stream:                          │
│              0.05,150.0,47.3,...                            │
│              0.10,150.0,85.4,...                            │
│              0.15,150.0,125.2,...                           │
│                       │                                     │
│                       ▼                                     │
│  ┌────────────────────────────────────────┐                │
│  │ Parse CSV, update 4 graphs             │                │
│  │ Display live motor response            │                │
│  │ User sees: "Response improved!"        │                │
│  └────────────────────────────────────────┘                │
│                                                              │
└──────────────────────┬───────────────────────────────────────┘
                       │
                       │ USB Cable (Virtual COM Port)
                       │
┌──────────────────────▼───────────────────────────────────────┐
│                    ESP32-S3                                  │
│  (GUI_Compatible_Firmware.cpp)                              │
│                                                              │
│  ┌────────────────────────────────────────┐                │
│  │ Serial RX: "kP:0.035"                  │                │
│  │ Parser: Extract 0.035 from string      │                │
│  │ Action: KP = 0.035f                    │                │
│  └────────────────────┬───────────────────┘                │
│                       │                                     │
│                       ▼                                     │
│  ┌────────────────────────────────────────┐                │
│  │ PID Control Loop (20 Hz)               │                │
│  │                                        │                │
│  │ 1. Read encoder:                       │                │
│  │    actual_rpm = ticks / 2240 / dt * 60│                │
│  │                                        │                │
│  │ 2. Get setpoint from waveform:         │                │
│  │    if (SQUARE): setpoint = 150 or 0   │                │
│  │                                        │                │
│  │ 3. Calculate PID:                      │                │
│  │    error = setpoint - actual_rpm       │                │
│  │    output = kP*e + kI*∫e + kD*de/dt    │                │
│  │                                        │                │
│  │ 4. Drive motor:                        │                │
│  │    PWM = output / MAX_RPM * 255        │                │
│  │    digitalWrite(DIR, direction)        │                │
│  │    ledcWrite(PWM_pin, duty)            │                │
│  │                                        │                │
│  │ 5. Publish CSV (every 50ms):           │                │
│  │    time, setpoint, actual, error, pwm, │                │
│  │    kp, ki, kd                          │                │
│  └────────────────────┬───────────────────┘                │
│                       │                                     │
│                       ▼                                     │
│  ┌────────────────────────────────────────┐                │
│  │ Hardware Control:                      │                │
│  │                                        │                │
│  │ Motor Driver (Cytron DD10A)            │                │
│  │   DIR=4, PWM=5  ──────────────►        │                │
│  │                                        │                │
│  │ Motor (with 20:1 gearbox)              │                │
│  │                          │             │                │
│  │ Encoder (28 CPR)         └──────────►  │                │
│  │   A=15, B=16            (feedback)     │                │
│  │                                        │                │
│  └────────────────────────────────────────┘                │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

---

## Data Flow Examples

### Example 1: Start Square Wave Test

**User Action:** Click "▶ Start Test" (with SQUARE selected)

**Flow:**
```
GUI sends:         "SQUARE\n"
                      ↓
Firmware receives:  testWaveform = "SQUARE"
                      ↓
Firmware publishes every 50ms:
                      ↓
Sample 1:  "0.05,0.0,0.0,0.0,0,0.02,0,0\n"
           Time=0.05s, Setpoint=0, Actual=0, Error=0
           (motor starting, not moved yet)
                      ↓
Sample 2:  "0.10,300.0,45.3,254.7,43,0.02,0,0\n"
           Time=0.10s, Setpoint=300, Actual=45.3
           (setpoint jumped to 300, motor accelerating)
                      ↓
Sample 3:  "0.15,300.0,125.4,174.6,78,0.02,0,0\n"
           (motor at 125 RPM, still accelerating)
                      ↓
Sample 4:  "0.20,300.0,250.1,49.9,98,0.02,0,0\n"
           (motor at 250 RPM, almost there)
                      ↓
Sample 5:  "0.25,300.0,295.5,4.5,101,0.02,0,0\n"
           (motor at 295 RPM, nearly settled)
                      ↓
Sample 6:  "0.30,0.0,285.3,-285.3,10,0.02,0,0\n"
           Time=0.30s (0.2 Hz, 5 sec period → 2.5s at 300)
           Setpoint dropped back to 0
           (motor starts decelerating)
```

**GUI shows:**
- Top-left graph: Blue line jumps up at 0.10s, red line follows
- Top-right graph: Green error line shows 250 RPM error, then decreases
- Bottom-left: PWM starts at ~40%, increases to ~100% as error increases
- Motor starts spinning, graphs update in real-time

---

### Example 2: Change Gain During Test

**User Action:** Adjust kP from 0.02 to 0.03, click "Send kP"

**Flow:**
```
Test running: SQUARE waveform active

GUI sends:     "kP:0.03\n"
                   ↓
Firmware receives: KP = 0.03f
                   ↓
Next PID cycle (50ms later):
   output = 0.03 * error + ...
   (higher kP means stronger response)
                   ↓
Motor responds faster!
                   ↓
CSV output changes:
   Before: "0.30,300.0,150.1,149.9,55,0.02,0,0\n"
   After:  "0.35,300.0,200.5,99.5,75,0.03,0,0\n"
           (motor accelerates faster with higher kP)
                   ↓
GUI graphs update immediately
   Red line (actual) approaches blue line (setpoint) faster
   User sees improvement in real-time!
```

---

### Example 3: Multiple Gains at Once

**User Action:** Dial in kP=0.035, kI=0.002, kD=0.008

**Flow:**
```
User adjusts kP with arrows: ◀ ◀ ◀
   (0.02 → 0.019 → 0.020 → 0.021 → ... → 0.035)

At each click, GUI displays new value:
   kP: [0.035]

User clicks "Send kP"
   GUI: "kP:0.035\n"

User adjusts kI: 0 → 0.001 → 0.002
User clicks "Send kI"
   GUI: "kI:0.002\n"

User adjusts kD: 0 → 0.005 → 0.008
User clicks "Send kD"
   GUI: "kD:0.008\n"

All three gains now active:
   output = 0.035*error + 0.002*integral + 0.008*derivative

Motor has best tuning!
```

---

## CSV Column Meaning

```
Time(s)
├─ Elapsed seconds since test start
├─ 0.05, 0.10, 0.15, 0.20, ...
└─ Used for X-axis in all 4 graphs

Setpoint(RPM)
├─ Target RPM at this moment
├─ Constant 300 during SQUARE high phase
├─ 0 during SQUARE low phase
└─ Used in top-left graph (blue line)

Actual(RPM)
├─ Measured RPM from encoder
├─ Starts at 0, ramps up during acceleration
├─ Follows setpoint with some lag
└─ Used in top-left graph (red line)

Error(RPM)
├─ Setpoint - Actual
├─ Large at start, decreases as motor catches up
├─ Ideally goes to zero
└─ Used in top-right graph (green line)

PWM(%)
├─ Motor PWM duty cycle 0-100%
├─ Starts high (motor accelerating)
├─ Decreases as error reduces
└─ Used in bottom-left graph (orange line)

kP
├─ Proportional gain (what you set)
├─ Constant during test
└─ Used in bottom-right graph (blue line)

kI
├─ Integral gain (what you set)
├─ Constant during test
└─ Used in bottom-right graph (green line)

kD
├─ Derivative gain (what you set)
├─ Constant during test
└─ Used in bottom-right graph (red line)
```

---

## Typical Response Sequence

### Good Response (kP=0.030)
```
Time  │ Setpoint │ Actual │ Error  │ PWM │ Observation
──────┼──────────┼────────┼────────┼─────┼─────────────────────────
0.00  │ 0        │ 0      │ 0      │ 0   │ Starting
0.05  │ 300      │ 10     │ 290    │ 50  │ Just started accelerating
0.10  │ 300      │ 45     │ 255    │ 60  │ Still accelerating
0.15  │ 300      │ 125    │ 175    │ 75  │ Halfway to setpoint
0.20  │ 300      │ 220    │ 80     │ 90  │ Getting close
0.25  │ 300      │ 290    │ 10     │ 95  │ Almost there
0.30  │ 300      │ 298    │ 2      │ 98  │ Settled at target ✓
0.35  │ 0        │ 285    │ -285   │ 20  │ Setpoint dropped
0.40  │ 0        │ 200    │ -200   │ 10  │ Decelerating
0.45  │ 0        │ 100    │ -100   │ 0   │ Slowing down
0.50  │ 0        │ 0      │ 0      │ 0   │ Stopped ✓
```

✓ Good: Smooth response, error decreases, settles quickly

---

### Poor Response (kP=0.01, too low)
```
Time  │ Setpoint │ Actual │ Error  │ PWM │ Observation
──────┼──────────┼────────┼────────┼─────┼─────────────────────────
0.00  │ 0        │ 0      │ 0      │ 0   │ Starting
0.05  │ 300      │ 5      │ 295    │ 30  │ Very slow to respond ❌
0.10  │ 300      │ 15     │ 285    │ 32  │ Creeping up
0.15  │ 300      │ 30     │ 270    │ 35  │ Still far from target ❌
0.20  │ 300      │ 50     │ 250    │ 38  │ Takes forever
0.25  │ 300      │ 75     │ 225    │ 40  │ At 3 seconds, only 25% RPM ❌
0.30  │ 300      │ 105    │ 195    │ 42  │ Never reaches 300!
0.35  │ 300      │ 140    │ 160    │ 45  │ Still climbing slowly
0.40  │ 300      │ 175    │ 125    │ 48  │ But setpoint was at 2.5s...
```

❌ Bad: Response too slow, kP too low, error never reaches zero

---

### Oscillating Response (kP=0.06, too high)
```
Time  │ Setpoint │ Actual │ Error  │ PWM │ Observation
──────┼──────────┼────────┼────────┼─────┼─────────────────────────
0.00  │ 0        │ 0      │ 0      │ 0   │ Starting
0.05  │ 300      │ 20     │ 280    │ 80  │ Fast acceleration
0.10  │ 300      │ 100    │ 200    │ 100 │ Aggressive
0.15  │ 300      │ 280    │ 20     │ 98  │ Zoomed past target! ⚠️
0.20  │ 300      │ 320    │ -20    │ 15  │ OVERSHOT! Oscillating ❌
0.25  │ 300      │ 250    │ 50     │ 80  │ Bounced back down
0.30  │ 300      │ 310    │ -10    │ 20  │ Up again ❌
0.35  │ 300      │ 290    │ 10     │ 75  │ Down again ❌
0.40  │ 300      │ 305    │ -5     │ 15  │ Still oscillating!
0.45  │ 300      │ 298    │ 2      │ 60  │ Finally settled (?)
0.50  │ 300      │ 303    │ -3     │ 10  │ Small ripple remains
```

❌ Bad: Overshoots target, oscillates, unstable response, kP too high

---

## Common Firmware Issues

### Issue: No CSV Output
```
Firmware connected but no data coming in

Causes:
  1. Motor not spinning (encoder = 0)
     Fix: Check motor connections, spin manually
  
  2. Encoder not connected
     Fix: Verify pins A=15, B=16, pullups on
  
  3. Test not started
     Fix: Send "SQUARE" command to firmware
```

### Issue: CSV Output but Wrong Values
```
Values don't make sense (e.g., RPM = 10000)

Causes:
  1. Wrong TICKS_PER_REV value
     Fix: Check gear ratio (20:1 = 2240 ticks)
  
  2. Time calculation wrong
     Fix: Check millis() starts fresh each test
  
  3. Bad encoder reading
     Fix: Verify encoder wiring
```

### Issue: Gains Don't Change
```
Send "kP:0.035" but CSV still shows kP=0.02

Causes:
  1. Command not received
     Fix: Check serial connection, baud rate 115200
  
  2. Parser error
     Fix: Ensure format is exact: "kP:0.035" (no spaces)
  
  3. GUI sending wrong format
     Fix: Check GUI code sends: f"kP:{value:.4f}\n"
```

---

## Quick Checklist

### Before GUI Connection
- [ ] Firmware uploaded (see boot message)
- [ ] Motor spins with "SQUARE" command
- [ ] Encoder counts increasing (check serial monitor)
- [ ] CSV data streaming (20 Hz)

### During GUI Connection
- [ ] Port selected correctly
- [ ] "✓ Connected" status shown
- [ ] Click "SQUARE" → Motor starts
- [ ] Graphs updating in real-time

### During Tuning
- [ ] Adjust kP with arrows
- [ ] Click "Send kP" to apply
- [ ] Watch graphs respond
- [ ] Graphs smoother = better tuning

---

**Communication is simple: Commands in → CSV out → Graphs update!** 🚀
