# REV HD Hex Motor (REV-41-1291) - Quick Reference Card

## Motor Specifications

```
┌──────────────────────────────────────┐
│  REV-41-1291 (HD Hex Motor)          │
│  with 20:1 Gearbox                   │
├──────────────────────────────────────┤
│  Base Motor Speed:     6000 RPM       │
│  Gear Reduction:       20:1           │
│  OUTPUT SPEED:         300 RPM max    │
│  Encoder (Motor):      28 CPR         │
│  Encoder (Output):     560 CPR        │
│  Stall Torque:         0.105 Nm      │
│  Max Power:            15W            │
│  Supply Voltage:       12V typical    │
└──────────────────────────────────────┘
```

## Key Calculations

```
OUTPUT SPEED (RPM) = Motor Speed / Gear Ratio
                   = 6000 / 20 = 300 RPM

OUTPUT ENCODER COUNTS = Motor PPR × Gear Ratio
                      = 28 × 20 = 560 CPR

ENCODER COUNTS IN 100ms = (Desired_RPM × 560) / 600
Example: For 100 RPM = (100 × 560) / 600 ≈ 93 counts
```

## Velocity Conversion

```
From Encoder Counts → RPM (at output shaft)

Counts in interval (100ms) → RPM:
  RPM = (Counts / 560) × 600
  
Example: 56 counts in 100ms
  RPM = (56 / 560) × 600 = 60 RPM

Manual Check:
  Rotate motor 1 complete revolution
  Encoder count should increase by ~560
  (exactly 560 if you can measure one perfect turn)
```

## Recommended PID Values

```
Starting Configuration:
┌────────────────────────┐
│ Kp = 0.8               │
│ Ki = 0.15              │
│ Kd = 0.12              │
└────────────────────────┘

Light Load:  Kp=1.0, Ki=0.15, Kd=0.15
Heavy Load:  Kp=1.5, Ki=0.20, Kd=0.15
Perfect?     Kp=0.8, Ki=0.18, Kd=0.12
```

## Test Velocity List

```
Safe Test Speeds (RPM at output):
  v=25    - Very slow, test encoder
  v=50    - Minimum practical speed
  v=100   - Good tuning reference
  v=150   - Medium speed test
  v=200   - High speed test
  v=250   - Near maximum
  v=300   - Absolute maximum (use with caution)

Recommended:
  Start: v=50   (safe, easy to observe)
  Tune:  v=100  (medium speed)
  Verify: v=50, v=150, v=250 (different speeds)
```

## Serial Commands

```
TUNING ADJUSTMENTS:
┌──────────────────┬──────────────────┐
│  Command         │  Effect          │
├──────────────────┼──────────────────┤
│ Kp=0.8          │ Set proportional  │
│ Ki=0.15         │ Set integral      │
│ Kd=0.12         │ Set derivative    │
│ v=100           │ Set desired RPM   │
│ status          │ Show all values   │
│ stop            │ Emergency stop    │
│ help            │ Show commands     │
└──────────────────┴──────────────────┘

Example Session:
  Kp=0.8
  Ki=0.15
  Kd=0.12
  v=100
  status
  (wait 5 seconds, observe Serial Plotter)
  stop
```

## Encoder Verification

```
To verify encoder is working:

1. Send: status
   Look for "Encoder Count: X"
   
2. Manually rotate motor ONE complete turn
   
3. Send: status again
   Look for "Encoder Count: Y"
   
4. Check: (Y - X) should equal ~560

If different:
  - Check GPIO connections (11, 12)
  - Verify encoder cable is seated
  - Test individual pins for signals
```

## Serial Plotter Guide

```
Serial Plotter Format:
Desired_RPM, Current_RPM, PWM_Output

Three lines appear:
  BLUE   = What you set (v=100)
  RED    = What you're getting (actual RPM)
  GREEN  = Motor power (0-255)

Good Tuning Indicators:
  ✓ Red closely follows blue
  ✓ No overshoot or <5%
  ✓ Smooth curve (not wiggly)
  ✓ Settles in 2-3 seconds
  ✓ Green proportional to error
```

## Troubleshooting Checklist

```
Motor Won't Move?
  ☐ Check 12V power supply
  ☐ Try v=255 (full power)
  ☐ Check motor direction pins (GPIO 14, 15)
  ☐ Verify motor shaft spins freely

Velocity Shows 0?
  ☐ Rotate motor by hand
  ☐ Send 'status' - encoder count should change
  ☐ If not: Check encoder cable on GPIO 11, 12

Motor Too Slow to Respond?
  ☐ Increase Kp: Try Kp=1.0
  ☐ Then Kp=1.2 if still slow
  ☐ Check motor isn't mechanically bound

Motor Oscillates?
  ☐ Increase Kd: Try Kd=0.15
  ☐ Then Kd=0.20 if still oscillating
  ☐ Reduce Kp: Try Kp=0.6

Never Reaches Target RPM?
  ☐ Increase Ki: Try Ki=0.2
  ☐ Check load on motor (too heavy?)
  ☐ Check power supply voltage
```

## Wiring Summary

```
ENCODER CONNECTIONS:
  Phase A → GPIO 11 (with pull-up)
  Phase B → GPIO 12 (with pull-up)
  GND     → GND

MOTOR CONNECTIONS:
  PWM     → GPIO 13
  Dir1    → GPIO 14
  Dir2    → GPIO 15
  GND     → GND
  +12V    → Motor driver +12V

MOTOR DRIVER (H-Bridge):
  IN1 (GPIO14) | IN2 (GPIO15) | PWM (GPIO13) | Direction
  ────────────────────────────────────────────────────────
       1        |      0       |    PWM      | Forward
       0        |      1       |    PWM      | Reverse
       1        |      1       |    PWM      | Brake
       0        |      0       |    PWM      | Coast/Idle
```

## Maximum Values

```
ABSOLUTE MAXIMUM (Don't exceed):
  Desired Velocity: 300 RPM (motor would rev to 6000 RPM)
  PWM Output:       255 (100% power)
  Supply Voltage:   ~14V max (12V nominal)
  Current:          ~2A max (motor limited)
  
RECOMMENDED LIMITS:
  Test Velocity:    250 RPM (safe testing)
  Continuous:       200 RPM (thermal limit)
  Stall Duration:   < 2 seconds (avoid burning out)
```

## Thermal Considerations

```
Motor Temperature Check:
  ✓ Room temp + 10°C  = OK
  ✓ Room temp + 30°C  = Warm (monitor)
  ✗ Room temp + 50°C+ = TOO HOT (stop immediately)

If motor gets hot:
  - Reduce desired velocity
  - Reduce test duration
  - Check for mechanical binding
  - Reduce load on motor
  - Check for short circuits
```

## Performance Targets

```
GOOD SYSTEM INDICATORS:
┌────────────────────────────────────┐
│ Response Time:      1-2 seconds    │
│ Steady-State Error: < 5 RPM        │
│ Overshoot:          < 5%           │
│ Settling Time:      2-3 seconds    │
│ Oscillation:        None           │
│ Temperature Rise:   < 30°C         │
│ Smooth operation:   Yes            │
└────────────────────────────────────┘

Acceptable but not ideal:
  Response: 3 seconds
  Error: 10 RPM
  Overshoot: 15%
  Settling: 5 seconds

Unacceptable:
  Response: > 5 seconds
  Error: > 20 RPM
  Overshoot: > 25%
  Oscillation: Multiple cycles
```

## Expected Output

```
SERIAL MONITOR STARTUP:
  === ESP32-S3 PID Motor Controller ===
  Motor: REV HD Hex Motor (REV-41-1291)
  Gearbox: 20:1 Reduction
  Max Output RPM: 300
  Encoder: 28 CPR (560 CPR effective at output)
  ...
  ✓ System Ready!
  Velocity is displayed in RPM at the output shaft

STATUS OUTPUT:
  === REV HD Hex Motor (20:1) Status ===
  PID Parameters:
    Kp: 0.8000
    Ki: 0.1500
    Kd: 0.1200
  
  Motor Control:
    Desired Velocity: 100.0 RPM
    Current Velocity: 98.5 RPM
    PID Error: 1.5 RPM
    PID Output: 120 (PWM 0-255)
  ...
```

## Encoder Signal Check

```
If encoder seems noisy or unreliable:

1. Verify connections:
   - GPIO 11 & 12 properly seated
   - No loose wires
   - Shielded cable if possible

2. Test encoder isolation:
   - Check there's no loose metal touching pins
   - Motor power separate from encoder power

3. Add capacitive filter (if needed):
   - 0.1µF cap across GND-A
   - 0.1µF cap across GND-B
   - Use 10kΩ pull-ups (already in ESP32)

4. In code, increase smoothing:
   - Change: #define VELOCITY_SAMPLES 20
   - To:     #define VELOCITY_SAMPLES 40
   - Doubles noise rejection
```

## Quick Calculation Reference

```
Convert Motor RPM to Output RPM:
  Output RPM = Motor RPM / 20
  
  Motor at 6000 → Output at 300
  Motor at 4000 → Output at 200
  Motor at 2000 → Output at 100

Convert Output RPM to Encoder Counts/100ms:
  Counts = (RPM × 560) / 600
  
  100 RPM → 93 counts/100ms
  200 RPM → 187 counts/100ms
  300 RPM → 280 counts/100ms

Reverse (from encoder to RPM):
  RPM = (Counts × 600) / 560
  
  100 counts → 107 RPM
  280 counts → 300 RPM
```

## Summary Table

| Parameter | Value | Note |
|-----------|-------|------|
| Motor Model | REV-41-1291 | HD Hex Motor |
| Gear Ratio | 20:1 | Planetary |
| Max Output | 300 RPM | At 12V supply |
| Encoder PPR (motor) | 28 | Quadrature |
| Encoder PPR (output) | 560 | After gearing |
| Default Kp | 0.8 | Tunable |
| Default Ki | 0.15 | Tunable |
| Default Kd | 0.12 | Tunable |
| Velocity Unit | RPM (output) | At gearbox output |
| PWM Range | 0-255 | From ESP32 |
| Test Speed | 100 RPM | Good reference point |

---

**Motor: REV-41-1291 | Gear: 20:1 | Code: Optimized for this config**
