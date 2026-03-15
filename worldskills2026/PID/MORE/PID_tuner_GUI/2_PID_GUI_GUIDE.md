# 🎛️ PID Tuning GUI - Complete User Guide

## Installation & Setup

### Prerequisites
```bash
pip install PyQt5 pyserial matplotlib numpy
```

### Launch the GUI
```bash
python pid_tuner_gui.py
```

---

## Interface Overview

### Left Panel (Controls)

#### 🔌 Serial Connection
- **Port Dropdown:** Select your COM port (e.g., COM10)
- **Refresh:** Find available ports
- **Connect/Disconnect:** Establish serial link
- **Status Indicator:** Shows connection state (✓ Green = Connected)

#### ⚙️ PID Gains
Three gain controls with **increment/decrement arrows**:

```
Proportional (kP):  ◀ [0.0300] ▶  [Send kP]
Integral (kI):      ◀ [0.0010] ▶  [Send kI]
Derivative (kD):    ◀ [0.0050] ▶  [Send kD]
```

**How to use:**
- **Click ◀ ▶:** Adjust by ±0.001 (small steps)
- **Type directly:** Enter exact values
- **Send button:** Upload to motor

#### 📈 Test Waveforms
- **SQUARE:** 0 → 300 → 0 RPM (step response, best for P-tuning)
- **SINE:** Smooth oscillation (damping analysis)
- **RAMP:** Linear 0 → 300 RPM (tracking test)

#### 💾 Presets
Save/load gain combinations:
- Enter preset name (e.g., "20-1_gearbox_best")
- Click **Save** to store current kP, kI, kD
- Click **Load** to restore previous settings
- Presets stored in `pid_presets.json`

#### 🤖 Auto-Tuning
Automatic PID optimization using Ziegler-Nichols method (see below)

#### 📊 Current Data
Live data display:
- Time, Setpoint, Actual RPM
- Error, PWM signal
- Current gain values
- Auto-updates with incoming serial data

### Right Panel (Graphs)

**4 Real-Time Plots:**

1. **Motor Speed Response** (top-left)
   - Blue = Target RPM
   - Red = Actual RPM
   - Shows tracking performance

2. **Control Error** (top-right)
   - Green = Difference (Setpoint - Actual)
   - Lower = Better

3. **PWM Control Signal** (bottom-left)
   - Orange line = Motor command (0-100%)
   - Shows how hard motor is being driven

4. **PID Gains** (bottom-right)
   - Blue = kP (proportional)
   - Green = kI (integral)
   - Red = kD (derivative)

---

## Step-by-Step Tuning Workflow

### Phase 1: Connect & Verify

```
1. Click "🔄 Refresh" to find COM ports
2. Select your port (COM10, /dev/ttyUSB0, etc.)
3. Click "✓ Connect"
4. Verify: Status shows "✓ Connected" (green)
```

### Phase 2: P-Gain Tuning (kP)

**Goal:** Find fastest response without oscillation

```
1. Set initial values:
   kP = 0.02  (conservative)
   kI = 0.00  (disable I)
   kD = 0.00  (disable D)
   
2. Send all gains:
   [Send kP] → [Send kI] → [Send kD]

3. Select "SQUARE" waveform

4. Click "▶ Start Test"

5. Watch graphs - observe response time:
   - Time to reach 150 RPM from 0
   - Any overshoot/oscillation?
   - How smooth?

6. Adjust kP using arrows:
   ◀ button = decrease kP (slower, smoother)
   ▶ button = increase kP (faster, more aggressive)

7. Increment by 0.005 each test:
   kP = 0.020 (slow, very smooth)
   kP = 0.025 (slightly faster)
   kP = 0.030 (good balance) ← LIKELY BEST
   kP = 0.035 (faster, small overshoot)
   kP = 0.040 (oscillating, too high)

8. STOP when you see oscillation
   Back off to last stable value

9. RECORD: Best kP value
```

**What you're looking for:**
```
✓ Good: Response in 2-4 seconds, <10% overshoot
✗ Bad: No response (kP too low), oscillating (kP too high)
```

### Phase 3: D-Gain Tuning (kD)

**Goal:** Dampen oscillations, smooth response

```
1. Keep best kP from Phase 2
   Set kD = 0.005 (start value)
   Keep kI = 0.000

2. [Send kP] → [Send kD]

3. Run SQUARE test

4. Observe:
   - Is overshoot reduced?
   - Response smoother?
   - Any instability?

5. Adjust kD using arrows:
   ◀ button = less damping
   ▶ button = more damping
   
   Typical range: 0.003 to 0.010

6. GOAL: No overshoot, smooth approach to target

7. RECORD: Best kD value
```

### Phase 4: I-Gain Tuning (kI) [Optional]

**Goal:** Eliminate steady-state error

```
1. Keep best kP and kD from phases 2-3
   Set kI = 0.001 (very small start)

2. [Send kI]

3. Run SQUARE test

4. Observe:
   - Does error settle to zero?
   - Any ringing/oscillation?

5. Increase kI slowly:
   0.001 → 0.002 → 0.003 → ...

6. STOP if you see oscillation (kI too high)

7. TYPICAL: kI = 0.001 to 0.005 for 20:1 gearbox

8. RECORD: Final kP, kI, kD
```

---

## Auto-Tuning: What It Is & How It Works

### What is Auto-Tuning?

**Automatic PID parameter calculation without manual tweaking**

Instead of manually adjusting gains, the system:
1. Performs a special test
2. Measures motor behavior
3. Calculates optimal kP, kI, kD mathematically
4. Applies the gains automatically

### Ziegler-Nichols Method (Industry Standard)

#### The Process

```
Step 1: Relay Test
├─ Set kI = 0, kD = 0
├─ Increase kP until motor oscillates
└─ Measure oscillation period (T_u)

Step 2: Calculate Gains
├─ kP = 0.60 × K_u   (K_u = critical gain)
├─ kI = 1.2 × K_u / T_u
└─ kD = 0.075 × K_u × T_u

Step 3: Apply Gains
└─ Motor now has optimal response
```

#### Why It Works

```
Ziegler-Nichols empirically determined that if you know:
- When the system starts oscillating (critical point)
- How fast it oscillates (period)

Then you can calculate gains that give:
✓ Fast response (minimal rise time)
✓ Slight overshoot (~5%)
✓ Quick settling (low overshoot)
```

### How to Use Auto-Tune in GUI

```
1. Connect to motor
   ✓ Status shows "Connected"

2. Click "🎯 Start Auto-Tune"

3. System will:
   Step 1 (20 sec): Relay test - motor oscillates
   Step 2 (10 sec): Measure and calculate
   Step 3 (10 sec): Apply gains to motor

4. Watch progress bar

5. Done! Gains automatically set to optimal values

6. Verify with SQUARE waveform:
   Should see fast, minimal overshoot response
```

### Auto-Tune Results Example

**For 20:1 gearbox system:**

```
Before Auto-Tune:
  kP = 0.020
  kI = 0.000
  kD = 0.000
  Response: Slow (5+ seconds), no damping
  
Auto-Tune calculates:
  kP = 0.035  ← faster
  kI = 0.002  ← eliminates error
  kD = 0.006  ← adds damping
  Response: Fast (2-3 seconds), stable, minimal overshoot
```

### When to Use Auto-Tune vs Manual

| Situation | Use |
|-----------|-----|
| First time tuning | ✓ Auto-Tune (baseline) |
| Fine-tuning response | Manual adjustment |
| Different motor/load | Auto-Tune again |
| Quick setup needed | Auto-Tune |
| Special requirements | Manual (more control) |

---

## Preset Management

### Save a Preset

```
1. Adjust gains to values you like
   kP = 0.035, kI = 0.002, kD = 0.006

2. Enter preset name:
   "20-1_gearbox_optimal"

3. Click "💾 Save"

4. Popup confirms saved
```

### Load a Preset

```
1. Click dropdown under "Saved Presets"
2. Select preset name
3. Click "📂 Load"
4. Gains automatically load:
   kP field = 0.035
   kI field = 0.002
   kD field = 0.006

5. Click "Send" buttons to upload to motor
```

### Preset File

Presets stored in `pid_presets.json`:

```json
{
  "20-1_gearbox_optimal": {
    "kP": 0.035,
    "kI": 0.002,
    "kD": 0.006
  },
  "20-1_gearbox_fast": {
    "kP": 0.040,
    "kI": 0.001,
    "kD": 0.008
  },
  "20-1_gearbox_stable": {
    "kP": 0.025,
    "kI": 0.000,
    "kD": 0.005
  }
}
```

You can edit this file manually or use the GUI.

---

## Graph Interpretation

### Motor Speed Response (Top-Left)

```
GOOD response:
  - Blue and red lines close together
  - Red line smoothly follows blue
  - Minimal lag
  
BAD response:
  - Red line far below blue (slow)
  - Red line oscillates around blue (underdamped)
  - Red line overshoots blue (overshoot)
```

### Control Error (Top-Right)

```
GOOD:
  - Error starts high, quickly goes to zero
  - Smooth decline, no oscillation
  - Area under curve is small (total integrated error)

BAD:
  - Error stays high (system not responding)
  - Error oscillates (ringing)
  - Error doesn't return to zero (system can't catch up)
```

### PWM Signal (Bottom-Left)

```
Healthy PWM:
  - Starts high, ramps down
  - Stays <100% (not saturated)
  - Smooth curve

Problem:
  - Constant at 100% (motor maxed out)
  - Oscillating (on/off cycling)
  - Noisy/choppy
```

### PID Gains (Bottom-Right)

```
Should show:
  - kP as main contributor (blue line highest)
  - kI small (green line lower)
  - kD tiny (red line near zero)
  
Example values over time:
  kP = 0.035 (constant, shows what you set)
  kI = 0.002 (constant, what you set)
  kD = 0.006 (constant, what you set)
```

---

## Troubleshooting

### Issue: Motor Doesn't Move

**Causes:**
- Not connected to serial
- kP is zero
- Motor disabled

**Fix:**
1. Check status: "✓ Connected"?
2. Check kP value (should be > 0.01)
3. Click "▶ Start Test" to enable motor

### Issue: Motor Oscillates at Low kP

**Cause:** Encoder noise or wrong CPR setting

**Fix:**
1. Run encoder test (Step 1) separately
2. Check COUNTS_PER_REV = 560
3. Re-upload firmware

### Issue: Graphs Not Updating

**Cause:** Serial data not arriving

**Fix:**
1. Check serial connection
2. Disconnect/reconnect
3. Verify baudrate (115200)

### Issue: Auto-Tune Fails

**Cause:** Motor can't oscillate properly

**Fix:**
1. Manually set kP = 0.05 first
2. Run SQUARE test to verify motor responds
3. Then try Auto-Tune again

---

## Best Practices

### Do's ✓
- Start with low kP (0.02)
- Increment by 0.005 each test
- Wait for motor to stabilize
- Save good presets
- Test multiple waveforms (SQUARE, SINE, RAMP)
- Record which gains work best

### Don'ts ✗
- Don't jump kP by 0.01+ (too aggressive)
- Don't run test without stopping previous
- Don't leave motor running (clear buffer)
- Don't use I-gain if you don't understand it
- Don't over-saturate PWM (keep <100%)

---

## Example Tuning Session

```
TIME: 0:00 → Connected to COM10

TIME: 0:30 → P-Tuning Phase
  kP: 0.020 → SQUARE test → Response: SLOW (5 sec)
  kP: 0.025 → SQUARE test → Response: Good (3.5 sec)
  kP: 0.030 → SQUARE test → Response: Very good (3 sec)
  kP: 0.035 → SQUARE test → Response: Fast (2.5 sec), slight overshoot
  kP: 0.040 → SQUARE test → Response: Oscillating (TOO HIGH)
  BEST kP = 0.030 ✓

TIME: 10:30 → D-Tuning Phase
  kD: 0.000 → SQUARE test → Underdamped, overshoot 15%
  kD: 0.005 → SQUARE test → Overshoot 8%, good
  kD: 0.010 → SQUARE test → No overshoot, smooth
  BEST kD = 0.008 ✓

TIME: 20:00 → Save Preset
  Name: "20-1_gearbox_final"
  Gains: kP=0.030, kI=0.000, kD=0.008
  💾 Save ✓

TIME: 20:15 → Verify with Sine Test
  SINE waveform → Smooth tracking
  Response: Excellent ✓

TIME: 20:30 → Done!
  Final Settings:
  kP = 0.030
  kI = 0.000  (not needed)
  kD = 0.008
```

---

## Performance Targets

### For 20:1 Gearbox

| Metric | Target | Status |
|--------|--------|--------|
| Rise time | 2-3 sec | - |
| Overshoot | <10% | - |
| Settling time | <2 sec | - |
| Oscillation | None | - |
| Steady-state error | <2% | - |

### Good Final Values

```
kP: 0.025 - 0.035  (most important)
kI: 0.000 - 0.005  (optional, use if error doesn't settle)
kD: 0.005 - 0.010  (fine-tuning)
```

---

## Summary

| Step | What to Do | Time |
|------|-----------|------|
| 1 | Install & launch GUI | 2 min |
| 2 | Connect to serial | 1 min |
| 3 | P-tune (find best kP) | 15 min |
| 4 | D-tune (dampen response) | 5 min |
| 5 | Optional: I-tune (error) | 5 min |
| 6 | Save preset | 1 min |
| 7 | Verify all waveforms | 5 min |
| **Total** | **~35 minutes** | |

---

**Now you can tune your motors with a professional interface!** 🎛️ 🚀
