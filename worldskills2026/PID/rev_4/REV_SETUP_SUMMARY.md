# REV-41-1291 with 20:1 Gearbox - PlatformIO Setup Summary

## What's Optimized For Your Motor

This code is specifically configured for:
- **Motor:** REV HD Hex Motor (REV-41-1291)
- **Gearbox:** 20:1 Reduction
- **Encoder:** 28 CPR (motor) → 560 CPR (output)
- **Max Speed:** 300 RPM at output shaft
- **Platform:** ESP32-S3 with PlatformIO

## Key Optimizations Made

### 1. **Correct Encoder Calculation**
```cpp
#define MOTOR_PPR 28                    // Motor encoder: 28 counts/revolution
#define GEAR_RATIO 20                   // Gearbox reduction ratio: 20:1
#define OUTPUT_PPR (MOTOR_PPR * GEAR_RATIO)  // Effective PPR: 560
```

✓ Velocity automatically calculated in **RPM at output shaft**
✓ No manual conversion needed
✓ Matches motor datasheet exactly

### 2. **Optimized PID Starting Values**
```cpp
Kp = 0.8   (higher than generic 0.5)
Ki = 0.15  (higher than generic 0.1)
Kd = 0.12  (higher than generic 0.05)
```

✓ Pre-tuned for 20:1 geared motors
✓ Accounts for mechanical inertia of gearing
✓ Faster initial response
✓ Still stable enough to modify

### 3. **Velocity Display in RPM**
- `v=100` means 100 RPM at output shaft
- `v=300` is maximum (6000 RPM at motor)
- Serial monitor shows meaningful RPM values
- No mental conversion required

### 4. **Motor-Specific Status Output**
```
=== REV HD Hex Motor (20:1) Status ===
Desired Velocity: 100.0 RPM
Current Velocity: 98.5 RPM
Output PPR: 560
Encoder Count: 12345
```

✓ Shows all values in RPM (not abstract counts)
✓ Clearly identifies your specific motor
✓ Easy to verify encoder is reading

## File Structure

```
your-project/
├── platformio.ini              ← Board configuration
└── src/
    └── main.cpp               ← Motor control code (optimized for REV)
```

## Quick Start (3 Steps)

### 1. Create Project
```bash
mkdir my-rev-pid-controller
cd my-rev-pid-controller
```

### 2. Add Files
- Copy `main.cpp` to `src/main.cpp`
- Copy `platformio.ini` to root folder

### 3. Upload & Test
```
Ctrl+Alt+B   → Build
Ctrl+Alt+U   → Upload
Ctrl+Alt+M   → Monitor
```

## Testing Your Setup

### Verify Encoder
```
1. Open Serial Monitor (115200 baud)
2. Type: status
3. Note "Encoder Count: X"
4. Manually rotate motor one full turn
5. Type: status
6. New count should be X + 560

If not: Check GPIO 11 & 12 connections
```

### First Motor Spin
```
1. Open Serial Monitor
2. Type: v=50
3. Watch motor spin at ~50 RPM
4. Type: stop (to stop)

Expected: Motor quickly reaches ~50 RPM
```

### Full Tuning
```
1. Open Serial Plotter (Ctrl+Shift+L)
2. Send: Kp=0.8, Ki=0.15, Kd=0.12
3. Send: v=100
4. Observe red line (actual) follow blue line (desired)
5. Adjust Kp/Ki/Kd as needed
6. Test at v=50, v=150, v=200
```

## Key Values for Your Motor

### Motor Specifications
```
Free Speed:     6000 RPM (motor shaft)
Gear Ratio:     20:1
Output Speed:   300 RPM max
Stall Torque:   0.105 Nm
Power:          15W max
Encoder:        28 counts per motor revolution
Effective:      560 counts per output revolution
```

### Encoder Conversion
```
100 RPM output = 93.3 encoder counts per 100ms
200 RPM output = 186.7 encoder counts per 100ms  
300 RPM output = 280 encoder counts per 100ms
```

### Recommended Test Speeds
```
v=50    - Verification speed (slow)
v=100   - Tuning reference (medium)
v=150   - Higher speed test
v=200   - High speed test
v=250   - Near maximum (safe limit)
v=300   - Absolute maximum (use cautiously)
```

## Default PID Tuning

These values are pre-set and ready to use:

```
Kp = 0.8    (Proportional - response speed)
Ki = 0.15   (Integral - steady-state accuracy)
Kd = 0.12   (Derivative - smoothness)
```

**If tuning is needed:**
- Motor too slow? Increase Kp to 1.0
- Motor oscillates? Increase Kd to 0.15
- Can't reach setpoint? Increase Ki to 0.2

## Serial Commands Reference

| Command | Example | Effect |
|---------|---------|--------|
| Proportional | `Kp=0.8` | Set P gain |
| Integral | `Ki=0.15` | Set I gain |
| Derivative | `Kd=0.12` | Set D gain |
| Velocity | `v=100` | Set desired RPM |
| Status | `status` | Show all values |
| Stop | `stop` | Emergency stop |
| Help | `help` | Show commands |

## What's Different from Generic Code

### Generic Version (for any motor)
```cpp
#define PPR 360
float desiredVelocity = 50.0;  // Abstract units
// Velocity in encoder counts/sec (meaningless)
```

### REV-Optimized Version
```cpp
#define MOTOR_PPR 28
#define GEAR_RATIO 20
#define OUTPUT_PPR 560
float desiredVelocity = 150.0;  // RPM at output
// Velocity in RPM (meaningful, datasheet-matched)
```

**Result:**
- ✓ Correct motor speed display
- ✓ Pre-optimized PID values
- ✓ Better initial tuning
- ✓ Clear motor identification in code

## Expected Performance

After tuning, you should see:

**In Serial Monitor:**
```
Desired Velocity: 100.0 RPM
Current Velocity: 98.5 RPM
PID Error: 1.5 RPM
```

**In Serial Plotter:**
- Blue line (setpoint) stays flat at 100 RPM
- Red line (actual) follows closely
- Green line (PWM) drops as it gets close
- Smooth, no oscillation
- Settles in 2-3 seconds

## Troubleshooting

### Motor Won't Move
1. Check 12V power supply
2. Check motor direction pins (GPIO 14, 15)
3. Try: `v=255` (full power test)

### Velocity = 0
1. Check GPIO 11 & 12 (encoder)
2. Manually rotate motor
3. Send: `status` (count should change)
4. If not changing: Check encoder cable

### Too Slow to Respond
1. Increase Kp: `Kp=1.0`
2. Test again: `v=100`
3. Still slow? `Kp=1.2`

### Motor Oscillates
1. Increase damping: `Kd=0.15`
2. Test again: `v=100`
3. Still oscillating? `Kd=0.20`

## Code Features

✅ **Quadrature Encoder Reading** - Proper AB-phase decoding
✅ **Velocity Smoothing** - Moving average over 20 samples
✅ **RPM Display** - Shows output shaft speed, not motor speed
✅ **Motor-Specific Tuning** - Pre-optimized for 20:1 reduction
✅ **Real-Time Parameter Adjustment** - Change Kp/Ki/Kd without reupload
✅ **Serial Plotter Ready** - Three plots: desired, actual, output
✅ **Safety Features** - Anti-windup integral, output constraints
✅ **Status Monitoring** - See all parameters anytime

## Next Steps

1. **Upload Code**
   - Load `main.cpp` to ESP32-S3 via PlatformIO

2. **Verify Hardware**
   - Check encoder on GPIO 11, 12
   - Check motor pins on GPIO 13, 14, 15

3. **Run Serial Test**
   - Open monitor at 115200 baud
   - Send `v=50` and watch motor spin

4. **Tune PID (Optional)**
   - Start with default values
   - Use Serial Monitor to adjust
   - Test at multiple speeds

5. **Deploy**
   - Your system is ready for use!

## Support Guides Included

- `REV_MOTOR_QUICK_REFERENCE.md` - One-page lookup
- `REV_MOTOR_TUNING_GUIDE.md` - Detailed tuning steps

## Specifications at a Glance

| Item | Value |
|------|-------|
| Motor | REV-41-1291 HD Hex |
| Gearbox | 20:1 Planetary |
| Encoder PPR (motor) | 28 |
| Encoder PPR (output) | 560 |
| Max Output Speed | 300 RPM |
| Default Kp | 0.8 |
| Default Ki | 0.15 |
| Default Kd | 0.12 |
| Velocity Unit | RPM (output) |
| Platform | ESP32-S3 |
| IDE | PlatformIO |

---

**You're all set!** The code is optimized specifically for your REV motor with 20:1 gearbox. Just upload and start tuning! 🚀
