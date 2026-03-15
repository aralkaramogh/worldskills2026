# 📋 PlatformIO Files Index - Quick Reference

## You Asked For .cpp Files for PlatformIO ✓

This document lists **exactly what you need** for PlatformIO.

---

## Essential Files (Download These 4)

### 1. Code Files (.cpp format)

#### `01_EncoderTest_20-1Gearbox.cpp`
- **Purpose:** Test encoder and RPM reading
- **When to use:** First - verify hardware working
- **Time needed:** ~10 minutes
- **Upload to:** `src/main.cpp`

#### `02_P_Tuning_20-1Gearbox.cpp`
- **Purpose:** Tune proportional gain (kP)
- **When to use:** Second - after encoder verification
- **Time needed:** ~30-45 minutes
- **Upload to:** `src/main.cpp`

#### `03_PID_Complete_20-1Gearbox.cpp`
- **Purpose:** Full PID with D-gain and optional I-gain
- **When to use:** Third - complete tuning
- **Time needed:** ~20-30 minutes
- **Upload to:** `src/main.cpp`

**Note:** All three are identical in format, just different content. Use them in order.

### 2. Configuration File

#### `platformio.ini`
- **Purpose:** Configure PlatformIO settings
- **Settings included:**
  - ✓ Correct board (ESP32-S3-DevKitC-1)
  - ✓ Serial baud rate (115200)
  - ✓ Upload speed (921600)
  - ✓ Build optimization
  - ✓ Parallel compilation
- **Install location:** Project root directory
- **Action:** Copy as-is, no changes needed

---

## Quick Start Documentation (Read These)

### `PLATFORMIO_QUICKSTART.md` ⭐ START HERE
- **What:** 5-minute setup guide
- **Read time:** 5 minutes
- **Contains:**
  - Installation steps
  - Project creation
  - File copying
  - First upload
  - Troubleshooting basics
- **Why:** Fastest way to get running

### `CHEAT_SHEET.md`
- **What:** 1-page quick reference
- **Use:** Keep visible while tuning
- **Contains:**
  - Pin configuration
  - All serial commands
  - Expected RPM values
  - Common mistakes
  - Performance targets

### `INO_vs_CPP.md`
- **What:** Explains .cpp vs .ino format
- **Read when:** Understanding why you use .cpp
- **Contains:**
  - Format comparison
  - Why .cpp for PlatformIO
  - Conversion examples
  - Common mistakes

---

## Reference Documentation (For Details)

### `PLATFORMIO_SETUP.md`
- **What:** Detailed PlatformIO guide
- **Use:** When you need more information
- **Contains:**
  - Complete setup instructions
  - VS Code integration
  - CLI usage
  - Troubleshooting
  - Project structure
  - Serial monitor guide

### `SUMMARY_AND_ANSWERS.md`
- **What:** Answers to your 2 questions
- **Read:** For understanding your setup
- **Contains:**
  - Arduino installation solutions
  - 20:1 gearbox explanation
  - Code changes needed
  - Performance expectations

---

## Optional but Useful

### `COMPLETE_PACKAGE_SUMMARY.md`
- **What:** Overview of all 30 files
- **Use:** Understanding what you have
- **Contains:** File inventory, purposes, recommendations

### `pid_monitor.py`
- **What:** Python real-time graphing tool
- **Use:** Visualize PID response in real-time
- **Install:** `pip install pyserial matplotlib pandas`
- **Run:** `python pid_monitor.py COM3`

---

## Files NOT Needed for PlatformIO

These are for **Arduino IDE** users (different IDE):

```
✗ 01_EncoderTest_20-1Gearbox.ino
✗ 02_P_Tuning_20-1Gearbox.ino
✗ 03_PID_Complete_20-1Gearbox.ino
✗ ESP32_INSTALLATION_FIXES.md (Arduino IDE specific)
```

(Don't download these if using PlatformIO)

---

## Download Checklist

### Essential (Must Have) ✓
- [ ] `01_EncoderTest_20-1Gearbox.cpp`
- [ ] `02_P_Tuning_20-1Gearbox.cpp`
- [ ] `03_PID_Complete_20-1Gearbox.cpp`
- [ ] `platformio.ini`

### Quick Start (Should Read) ✓
- [ ] `PLATFORMIO_QUICKSTART.md`
- [ ] `CHEAT_SHEET.md`

### Reference (Keep Handy) ✓
- [ ] `INO_vs_CPP.md`
- [ ] `SUMMARY_AND_ANSWERS.md`

### Detailed (For Reference) ○
- [ ] `PLATFORMIO_SETUP.md`
- [ ] `COMPLETE_PACKAGE_SUMMARY.md`

### Tools (Optional) ○
- [ ] `pid_monitor.py`

---

## Usage Flow

### Initial Setup (One-Time)
```
1. Read: PLATFORMIO_QUICKSTART.md (5 min)
2. Install: PlatformIO (if needed)
3. Create: ESP32-S3 project
4. Copy: platformio.ini to project root
```

### Step 1: Encoder Test
```
1. Copy: 01_EncoderTest_20-1Gearbox.cpp → src/main.cpp
2. Upload: platformio run --target upload
3. Test: Type FWD, PWM:128, STOP
4. Record: "Encoder working!" or identify issues
```

### Step 2: P-Tuning
```
1. Copy: 02_P_Tuning_20-1Gearbox.cpp → src/main.cpp
2. Upload: platformio run --target upload
3. Tune: Type SQUARE, then kP:0.02, kP:0.025, etc.
4. Record: Best kP value (probably 0.02-0.03)
```

### Step 3: Full PID
```
1. Copy: 03_PID_Complete_20-1Gearbox.cpp → src/main.cpp
2. Upload: platformio run --target upload
3. Tune: Your kP value, then adjust kD from 0.005 to 0.010
4. Verify: Test SQUARE, SINE, RAMP
5. Record: Final kP, kI, kD values
```

---

## File Locations

### Where They Come From
```
Download location: All from this project package
```

### Where They Go
```
Project Root:
├── platformio.ini              ← Copy here
├── src/
│   └── main.cpp                ← Copy one .cpp here at a time
├── include/
├── lib/
└── test/
```

### How to Copy

**Windows:**
```
1. Download the file
2. Drag-drop to your project folder
3. Or: Copy (Ctrl+C) and Paste (Ctrl+V)
```

**Linux/Mac:**
```bash
cp 01_EncoderTest_20-1Gearbox.cpp ~/MyMotorProject/src/main.cpp
cp platformio.ini ~/MyMotorProject/
```

---

## Quick Command Reference

### PlatformIO CLI Commands

```bash
# One-time setup
platformio init --board esp32-s3-devkitc-1 --framework arduino

# Build
platformio run

# Upload
platformio run --target upload

# Upload + Monitor
platformio run --target upload --target monitor

# Monitor only
platformio device monitor --baud 115200

# Clean rebuild
platformio run --target clean
```

### Serial Monitor Commands (Type These)

```
FWD              → Motor forward
REV              → Motor reverse
PWM:128          → Set PWM (50%)
STOP             → Stop motor
RESET            → Reset encoder

SQUARE           → Square wave test
SINE             → Sine wave test
RAMP             → Ramp wave test

kP:0.02          → Set P-gain
kD:0.005         → Set D-gain
kI:0.001         → Set I-gain

INFO             → Show current status
HELP             → Show all commands
```

---

## File Sizes

```
01_EncoderTest_20-1Gearbox.cpp      ~8 KB
02_P_Tuning_20-1Gearbox.cpp         ~9 KB
03_PID_Complete_20-1Gearbox.cpp     ~10 KB
platformio.ini                       ~2 KB
All documentation                    ~400 KB
pid_monitor.py                       ~3 KB
────────────────────────────────────────
Total download size                  ~432 KB
```

---

## Success Indicators

### Encoder Test Works When:
✓ Serial monitor shows boot message
✓ Motor runs with FWD command
✓ RPM values increase smoothly with PWM
✓ No erratic jumps in readings

### P-Tuning Works When:
✓ Square wave appears in output
✓ Setpoint and actual RPM both displayed
✓ Motor responds to kP changes
✓ Finding stable kP value

### Full PID Works When:
✓ Response is smooth and stable
✓ Overshoot is <10%
✓ SINE wave is smooth
✓ RAMP tracking is good

---

## Troubleshooting Quick Links

| Problem | Solution File | Section |
|---------|---|---|
| Board not detected | PLATFORMIO_QUICKSTART.md | Troubleshooting |
| Upload fails | PLATFORMIO_SETUP.md | Upload failures |
| Serial shows garbage | CHEAT_SHEET.md | Troubleshooting |
| Motor won't run | CHEAT_SHEET.md | Motor won't start |
| RPM reading wrong | SUMMARY_AND_ANSWERS.md | Your setup |
| Don't understand .cpp | INO_vs_CPP.md | All of it |

---

## Support Resources

### Official Documentation
- **PlatformIO:** https://docs.platformio.org/
- **ESP32:** https://docs.platformio.org/en/latest/platforms/espressif32.html
- **Arduino Framework:** https://docs.platformio.org/en/latest/frameworks/arduino.html

### VS Code
- **VS Code Docs:** https://code.visualstudio.com/docs

### Hardware
- **REV Robotics:** https://docs.revrobotics.com/
- **Cytron MDD10:** https://www.cytron.io/products/mdd10

---

## Key Points to Remember

✓ Use **`.cpp` files, not `.ino` files** for PlatformIO
✓ Copy **one .cpp file to `src/main.cpp`** at a time
✓ **Copy `platformio.ini`** to project root (no changes needed)
✓ Use **`platformio run --target upload`** to compile and upload
✓ Use **`platformio device monitor`** to see output
✓ All 3 files are **pre-configured for your 20:1 gearbox**
✓ **No manual edits needed** - just copy and run

---

## Your Setup (Already Configured)

All .cpp files have these values already set:

```cpp
#define COUNTS_PER_REV 560      // ✓ Correct for 20:1
#define MOTOR_MAX_RPM 300       // ✓ Correct for 20:1
#define SAMPLE_INTERVAL 20      // 50 Hz loop rate
float kP = 0.02;                // ✓ Conservative start for inertia
```

**No changes needed!** Upload and test immediately.

---

## Expected Timeline

| Phase | Time | Activity |
|-------|------|----------|
| Setup | 15 min | Install + create project |
| Step 1 | 10 min | Upload encoder test |
| Step 2 | 45 min | P-gain tuning |
| Step 3 | 30 min | Full PID tuning |
| **Total** | **~2.5 hours** | Complete working system |

---

## You're Ready!

Download these 4 files:
1. ✅ `01_EncoderTest_20-1Gearbox.cpp`
2. ✅ `02_P_Tuning_20-1Gearbox.cpp`
3. ✅ `03_PID_Complete_20-1Gearbox.cpp`
4. ✅ `platformio.ini`

And these 2 guides:
5. 📖 `PLATFORMIO_QUICKSTART.md`
6. 📖 `CHEAT_SHEET.md`

Then follow `PLATFORMIO_QUICKSTART.md` for setup.

**You'll be running your first test in 20 minutes!** ✓

Good luck with PlatformIO! 🚀
