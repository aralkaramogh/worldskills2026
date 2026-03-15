# PID Tuning Guide - REV HD Hex Motor (REV-41-1291) with 20:1 Gearbox

## Motor Specifications

| Parameter | Value |
|-----------|-------|
| Motor Model | REV HD Hex Motor (REV-41-1291) |
| Base Motor Speed | 6000 RPM |
| Gear Ratio | 20:1 |
| **Output Speed Max** | **300 RPM** |
| Encoder Type | Magnetic Quadrature |
| Motor Encoder PPR | 28 counts/revolution |
| **Output Encoder PPR** | **560 counts/revolution** |
| Stall Torque | 0.105 Nm (motor) |
| Max Power | 15W |
| Voltage | Typical robotics (12V) |

## Velocity Display

✅ **The code displays velocity in RPM at the OUTPUT SHAFT**

This means:
- Setting `v=150` = 150 RPM at the output
- Motor will run at 150 × 20 = 3000 RPM internally
- Maximum safe velocity: **300 RPM output** (6000 RPM motor)

## Recommended Starting PID Values

Based on the REV motor characteristics with 20:1 reduction:

```
Kp = 0.8    (was 0.5)
Ki = 0.15   (was 0.1)
Kd = 0.12   (was 0.05)
```

**Why higher values?**
- 20:1 gearing provides mechanical inertia
- Motor responds well to stronger proportional control
- Need more damping (Kd) due to higher effective mass

## Quick Start Tuning

### Step 1: Verify Everything Works
1. Upload code to ESP32-S3
2. Open Serial Monitor (115200 baud)
3. Send command: `status`
4. Check output shows RPM values and encoder counts

### Step 2: Test Basic Response
```
Send: v=100
Expected: Motor spins at ~100 RPM
         Red and blue lines appear in Serial Plotter
         Motor reaches steady state in 2-3 seconds
```

### Step 3: Adjust Kp for Response Speed
```
If response is SLOW (takes > 3 seconds to reach RPM):
  Send: Kp=1.0    (increase by 0.2)
  Test: v=100
  
If response OSCILLATES (overshoots significantly):
  Send: Kp=0.6    (decrease by 0.2)
  Test: v=100
  
Continue until response is snappy without oscillation
```

### Step 4: Adjust Kd for Smoothness
```
If overshooting by more than 10% of setpoint:
  Send: Kd=0.15
  Test: v=100
  
If still overshooting:
  Send: Kd=0.20
  
If becoming sluggish:
  Reduce Kd back to 0.15
```

### Step 5: Adjust Ki for Steady-State Error
```
If motor never quite reaches setpoint (gap remains):
  Send: Ki=0.2
  Test: v=100
  
If oscillation appears after adding Ki:
  Reduce Ki back to 0.15
```

## Testing at Multiple Speeds

After tuning at 100 RPM, test these velocities:

```
Send: v=50
Observe: Response should be proportional
         Should reach 50 RPM smoothly
         
Send: v=150
Observe: Should handle higher speed without instability
         
Send: v=200
Observe: Near maximum should still be controlled
         Watch for oscillation
         
Send: v=250
Observe: Getting close to 300 RPM limit
         Should be smooth
```

## Serial Plotter Interpretation

When you open Serial Plotter, you'll see:

```
Blue line   = Desired RPM (setpoint)
Red line    = Actual RPM (feedback)
Green line  = Motor PWM power (0-255)
```

### Good Response Pattern:
```
      ▲ RPM
      │     ┌─ Blue (desired)
 300  │ ───┤
      │    └─ Red (actual) follows closely
 200  │ ▁▂▃▅▇█▇▅▃▂▁
      │
 100  │ 
      └─────────────► Time (seconds)
```

### Bad Response Patterns:

**Too Slow (Low Kp):**
```
      │ ════════
      │ ▂▃▄▅▆▇ (lags behind)
```

**Too Oscillatory (High Kp):**
```
      │ ╱╲╱╲╱╲╱╲
      │ ─┤ (bounces above/below)
      │ ╲╱╲╱╲╱╲
```

## Common Issues and Solutions

### Issue: Motor Doesn't Reach Setpoint
**Possible Causes:**
- Kp too low
- Mechanical friction/binding
- Insufficient power supply

**Solutions:**
```
Try: Kp=1.0 (increase from 0.8)
Then: Kp=1.2 if still too slow
Then: Kp=1.5 if needed

Also check:
- Motor spins freely by hand
- Power supply voltage is stable
- No mechanical binding
```

### Issue: Motor Oscillates/Hunts
**Possible Causes:**
- Kp too high
- Kd too low
- Encoder noise

**Solutions:**
```
Try: Kd=0.15 (increase damping)
Then: Kd=0.20 if still oscillating
Then: Reduce Kp to 0.6

Or reduce encoder noise:
- Check wiring is secure
- Keep encoder cables away from motor power
- Add capacitor across encoder signals
```

### Issue: Velocity = 0
**Possible Causes:**
- Encoder not connected
- Wrong GPIO pins
- Encoder damaged

**Solutions:**
```
Send: status
Look for "Encoder Count" value
Manually rotate motor
Send: status again
Count should change

If count doesn't change:
- Check GPIO 11 (Encoder A)
- Check GPIO 12 (Encoder B)
- Verify connection to encoder
- Check encoder cable for damage
```

### Issue: Motor Twitches/Jerky Motion
**Possible Causes:**
- Noisy encoder reading
- Velocity calculation interval too short
- PWM frequency mismatch

**Solutions:**
```
Increase velocity smoothing:
Change: #define VELOCITY_SAMPLES 20
To:     #define VELOCITY_SAMPLES 40

This double-smooths the velocity feedback
Makes PID less sensitive to noise
```

## Expected Performance Metrics

These are realistic targets for this motor:

| Metric | Target | Acceptable Range |
|--------|--------|------------------|
| Response Time | 1-2 seconds | 0.5 - 3 seconds |
| Steady-State Error | < 5 RPM | < 10 RPM |
| Overshoot | < 5% | < 15% |
| Settling Time | 2-3 seconds | 2 - 5 seconds |
| Oscillation | None | < 2 cycles |

## Load Considerations

The tuning assumes moderate load on the motor.

**For different loads:**

**Light Load (spinning freely):**
- Can use higher Kp: 1.0-1.2
- Reduce Kd slightly: 0.08-0.10
- Faster response possible

**Heavy Load (high friction/inertia):**
- Need higher Kp: 1.5-2.0
- Increase Kd: 0.15-0.25
- Add Ki: 0.2-0.3
- Slower response expected

**No Load (wheel only):**
- Kp = 1.2, Ki = 0.15, Kd = 0.15
- Very responsive

## Verification Steps

Before declaring tuning complete:

- [ ] Motor reaches setpoint within 2-3 seconds
- [ ] No overshoot or < 5% overshoot
- [ ] Smooth settling without oscillation
- [ ] Works at v=50, v=100, v=150, v=200, v=250
- [ ] Error < 5 RPM at steady state
- [ ] Stable for 30+ seconds continuous
- [ ] Motor temperature normal
- [ ] No mechanical noise/grinding

## Using Serial Commands

### Quick Testing Session

```bash
# Set initial values
Kp=0.8
Ki=0.15
Kd=0.12

# Test at low speed
v=50
# Wait 5 seconds, observe Serial Plotter

# Test at medium speed
v=100
# Wait 5 seconds, observe

# Test at high speed
v=200
# Wait 5 seconds, observe

# Check status
status

# If good, try different Kp
Kp=0.9
v=100
# Wait and observe

# Stop when done
stop
```

### Parameter Adjustment Guide

```
ADJUSTMENT          COMMAND         EFFECT
────────────────────────────────────────────
Increase Kp         Kp=1.0          Faster response
Decrease Kp         Kp=0.7          Reduce overshoot
Increase Kd         Kd=0.15         Smoother (less overshoot)
Decrease Kd         Kd=0.08         Let it respond faster
Increase Ki         Ki=0.2          Better steady-state
Decrease Ki         Ki=0.1          Less oscillation
Change speed        v=150           Test different RPM
Check status        status          See all values
Stop motor          stop            Safety stop
```

## Advanced Tuning: Ziegler-Nichols Method

If basic tuning doesn't work well:

1. **Set Ki=0, Kd=0** (proportional only)
2. **Increase Kp** until motor oscillates consistently
3. **Note the oscillation frequency** and Kp value
4. **Calculate new values:**
   - Kp_new = 0.6 × Kp_oscillating
   - Ki_new = 1.2 × Kp_new / Period
   - Kd_new = 0.075 × Kp_new × Period

Where Period is oscillation period in seconds.

## Motor-Specific Notes

### REV HD Hex Motor Characteristics:
- **Brushed DC motor** = Simple control
- **28 PPR encoder** = Good resolution with gearing
- **20:1 reduction** = Increases torque, reduces speed and smoothness
- **High gear ratio** = Can be sluggish, needs responsive tuning

### Comparison with Ungeared Motor:
- More stable (less twitchy)
- Requires higher gains (mechanical inertia)
- Better at maintaining speed under load
- Maximum speed is 1/20th of motor

## Power Supply Notes

This motor typically runs on:
- **12V** (common in robotics)
- **5V** (lower power, may limit performance)

If running on lower voltage, expect:
- Lower maximum speed (less available power)
- Reduced responsiveness
- May need slightly higher Kp

## Safety Reminders

⚠️ **Before running at high speed:**
1. Clear area around motor
2. Have emergency stop command ready: `stop`
3. Start with low velocities (v=50)
4. Gradually increase
5. Watch for overheating (motor shouldn't get hot)

## Summary

**For REV HD Hex Motor (20:1):**

| Phase | Kp | Ki | Kd | Status |
|-------|----|----|-----|--------|
| Initial | 0.8 | 0.15 | 0.12 | ✓ Start here |
| Slow? | 1.0 | 0.15 | 0.12 | Increase Kp |
| Oscillating? | 0.7 | 0.15 | 0.15 | Reduce Kp, increase Kd |
| Error remaining? | 0.8 | 0.20 | 0.12 | Increase Ki |
| Tuned! | 0.8-1.0 | 0.15-0.20 | 0.12-0.15 | ✓ Done |

**Velocity range:** 0-300 RPM (output shaft)
**Test speeds:** 50, 100, 150, 200 RPM

---

**Happy Tuning! 🎯**
