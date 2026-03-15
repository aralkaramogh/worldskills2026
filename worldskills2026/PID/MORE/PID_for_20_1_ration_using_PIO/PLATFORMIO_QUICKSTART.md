# PlatformIO Quick Start - 5 Minute Setup

## You Asked For: .cpp Files for PlatformIO ✓

You now have **3 ready-to-use .cpp files** for PlatformIO:
- `01_EncoderTest_20-1Gearbox.cpp`
- `02_P_Tuning_20-1Gearbox.cpp`
- `03_PID_Complete_20-1Gearbox.cpp`

These are **exactly the same** as the .ino files, but formatted for PlatformIO.

---

## 30-Second Setup

### 1. Install PlatformIO
**VS Code (Recommended):**
- Install VS Code
- Extensions → Search "PlatformIO" → Install

**OR CLI:**
```bash
pip install platformio
```

### 2. Create Project
```bash
platformio init --board esp32-s3-devkitc-1 --framework arduino
```

### 3. Copy Configuration
Download `platformio.ini` and copy to your project root

### 4. Copy Code
Copy `01_EncoderTest_20-1Gearbox.cpp` to `src/main.cpp`

### 5. Upload
```bash
platformio run --target upload
```

### 6. Monitor
```bash
platformio device monitor --baud 115200
```

**Done!** ✓

---

## Visual Project Structure

After setup, your folder should look like:

```
MyMotorProject/
├── platformio.ini         ← Copy the one we provided here
├── src/
│   └── main.cpp           ← Copy 01_EncoderTest_20-1Gearbox.cpp here
├── include/
├── lib/
└── test/
```

---

## Step-by-Step for VS Code

### Step 1: Install
1. Open VS Code
2. Go to Extensions (Ctrl+Shift+X)
3. Search: `platformio`
4. Click Install on "PlatformIO IDE"
5. Wait for installation (5-10 seconds)
6. Reload window

### Step 2: Create Project
1. Ctrl+Shift+P (Command Palette)
2. Type: `PlatformIO: New Project`
3. Fill in:
   - **Project Name:** `MotorPID` (your choice)
   - **Board:** Search `esp32-s3` → Select `Espressif ESP32-S3-DevKitC-1`
   - **Framework:** `Arduino`
   - **Folder:** Your workspace
4. Click Finish

### Step 3: Copy Files
1. Download `platformio.ini` (from this package)
2. Download `01_EncoderTest_20-1Gearbox.cpp`
3. In VS Code:
   - **Left sidebar** → File explorer
   - Right-click `platformio.ini` → Delete
   - Drag-drop new `platformio.ini` into project root
   - Open `src/main.cpp` (it might be empty)
   - Delete the content
   - Drag-drop `01_EncoderTest_20-1Gearbox.cpp`
   - Rename to `main.cpp`

### Step 4: Configure Serial
1. Plug in your ESP32-S3 via USB
2. Open `platformio.ini`
3. Find these lines (uncomment if needed):
   ```ini
   # monitor_port = COM3
   # upload_port = COM3
   ```
4. Replace `COM3` with your actual port (check Device Manager)

### Step 5: Upload
1. Open `src/main.cpp`
2. Bottom status bar → Click **Upload** (arrow icon)
3. Wait for compilation and upload

### Step 6: Monitor
1. Bottom status bar → Click **Serial Port Monitor** (or Ctrl+Shift+A)
2. You should see startup messages
3. Type `HELP` and press Enter

---

## Step-by-Step for CLI

### Quick Version
```bash
# Create project
platformio init --board esp32-s3-devkitc-1 --framework arduino

# Copy files
cp 01_EncoderTest_20-1Gearbox.cpp src/main.cpp
cp platformio.ini .

# Upload and monitor
platformio run --target upload --target monitor
```

### Full Version
```bash
# 1. Create folder
mkdir MotorPID
cd MotorPID

# 2. Initialize project
platformio init --board esp32-s3-devkitc-1 --framework arduino

# 3. Copy your files here
#    (manually copy the .cpp and .ini files to this directory)

# 4. Build
platformio run

# 5. Upload to board
platformio run --target upload

# 6. Open serial monitor
platformio device monitor --baud 115200

# Now type in the monitor:
# FWD
# PWM:128
# STOP
```

---

## The 3 Sketches in Order

### Step 1️⃣: Encoder Test
```bash
cp 01_EncoderTest_20-1Gearbox.cpp src/main.cpp
platformio run --target upload
```
**Test:** Type `FWD`, `PWM:128`, verify ~150 RPM appears

### Step 2️⃣: P-Tuning
```bash
cp 02_P_Tuning_20-1Gearbox.cpp src/main.cpp
platformio run --target upload
```
**Test:** Type `SQUARE`, then `kP:0.02`, `kP:0.025`, etc.

### Step 3️⃣: Full PID
```bash
cp 03_PID_Complete_20-1Gearbox.cpp src/main.cpp
platformio run --target upload
```
**Test:** Type `SQUARE`, `kD:0.005`, `SINE`, `RAMP`

---

## VS Code Shortcuts (Fastest!)

```
Ctrl+Alt+B     Build
Ctrl+Alt+U     Upload
Ctrl+Alt+D     Upload + Monitor
Ctrl+Shift+A   Open terminal
Ctrl+~         Open terminal
```

**Fastest workflow:**
1. Edit code
2. Press **Ctrl+Alt+U** (uploads automatically)
3. Then **Ctrl+Alt+D** next time if you want monitor

---

## Troubleshooting

### "Board not found"
```ini
# In platformio.ini, verify:
board = esp32-s3-devkitc-1
# NOT just esp32
```

### "Cannot open port COM3"
```bash
# List available ports
platformio device list

# Update platformio.ini with correct port
upload_port = /dev/ttyUSB0  # Linux/Mac
upload_port = COM3           # Windows
```

### "Upload too slow"
```ini
# In platformio.ini, reduce:
upload_speed = 460800  # from 921600
```

### "Serial monitor shows garbage"
```ini
monitor_speed = 115200  # MUST match code
```

### "Module not found"
```bash
platformio lib install  # Auto-installs required libs
platformio run --target clean  # Clean and rebuild
```

---

## What You Get

With PlatformIO + your .cpp files:

✓ Professional development environment
✓ Faster compilation (parallel builds enabled)
✓ Better error messages
✓ Integrated serial monitor
✓ File organization (src/, include/, lib/)
✓ Library management
✓ Debugging capability
✓ VS Code integration
✓ Works on Windows/Mac/Linux

---

## File Locations

### What to download:
```
FROM THIS PACKAGE, DOWNLOAD:
├── 01_EncoderTest_20-1Gearbox.cpp      ← Use this
├── 02_P_Tuning_20-1Gearbox.cpp         ← Use this
├── 03_PID_Complete_20-1Gearbox.cpp     ← Use this
├── platformio.ini                      ← Use this
├── PLATFORMIO_SETUP.md                 ← Read for detailed guide
├── INO_vs_CPP.md                       ← Read to understand formats
└── (other guides optional)
```

### Where to put them:
```
YourProject/
├── platformio.ini                 ← Copy here (replaces default)
└── src/
    └── main.cpp                   ← Copy one .cpp file here
```

---

## Quick Reference: All Commands

```bash
# Setup
platformio init --board esp32-s3-devkitc-1 --framework arduino

# Building
platformio run                              # Build only
platformio run --target clean              # Clean old builds
platformio run --target upload             # Upload
platformio run --target upload --verbose   # Upload with details

# Monitoring
platformio device monitor --baud 115200    # Serial monitor
platformio device list                     # List available ports

# Libraries
platformio lib install "library_name"      # Install library
platformio lib list                        # List installed libraries

# Info
platformio boards esp32                    # List ESP32 boards
platformio platforms                       # List installed platforms
```

---

## Your Config File (platformio.ini)

The file we provided is pre-configured with:
- ✓ Correct baud rate (115200)
- ✓ Correct upload speed (921600)
- ✓ Optimizations enabled
- ✓ Parallel compilation (faster)
- ✓ Correct board selection
- ✓ Serial filtering

**You can use it as-is!** Just copy to project root.

---

## Switching Between Sketches

The easiest workflow:

```bash
# Currently at: Encoder Test

# Switch to P-Tuning
cp 02_P_Tuning_20-1Gearbox.cpp src/main.cpp
platformio run --target upload

# Switch to Full PID
cp 03_PID_Complete_20-1Gearbox.cpp src/main.cpp
platformio run --target upload
```

Or in VS Code:
1. Open file explorer (Ctrl+Shift+E)
2. Drag new .cpp onto `src/main.cpp`
3. Select "Replace"
4. Hit Ctrl+Alt+U to upload

---

## Hardware Check Before Testing

Before uploading, verify:
- ✓ USB cable connected to ESP32-S3
- ✓ Motor power (12V) connected to Cytron
- ✓ Encoder A/B on GPIO 4/5
- ✓ Motor PWM on GPIO 9
- ✓ Motor DIR on GPIO 8
- ✓ Common GND connections

---

## Serial Monitor in PlatformIO

### Open Monitor
**VS Code:** Ctrl+Shift+A → Search "Monitor"
**CLI:** `platformio device monitor --baud 115200`

### Send Commands
1. Type your command: `SQUARE`
2. Press **Enter** (Newline!)
3. You'll see the response

### Example Session
```
[Connected on COM3 at 115200]
===== ENCODER & RPM TEST =====
Configuration: 20:1 Gearbox (5:1 + 4:1)
Motor CPM: 560
Max RPM: 300

FWD
Direction: Forward

PWM:128
PWM set to: 128 (50.2%)

[Data starts appearing...]
100    45    17    769.4    50.2
200    78    33    771.2    50.2
...
```

---

## Pro Tips

### Tip 1: Keep Monitor Open While Developing
Reload changes while monitor stays connected. Very convenient!

### Tip 2: Export Data to CSV
1. Run test (e.g., `SQUARE`)
2. Let it run for 10 seconds
3. Select all output (Ctrl+A in monitor)
4. Copy (Ctrl+C)
5. Paste into Excel
6. Graph the data!

### Tip 3: Safe Edits
Keep a backup of each working sketch:
```
src/
├── main.cpp (current)
├── encoder_test_backup.cpp
├── p_tuning_backup.cpp
└── pid_complete_backup.cpp
```

### Tip 4: Batch Multiple Tests
```bash
# Run all three sketches in sequence
for file in 01_EncoderTest 02_P_Tuning 03_PID_Complete
do
  cp ${file}_20-1Gearbox.cpp src/main.cpp
  platformio run --target upload
  sleep 5
  echo "Uploaded ${file}"
done
```

### Tip 5: Keep Serial Monitor Logs
```bash
platformio device monitor --baud 115200 > logfile.txt
# Now run your tests
# Ctrl+C to stop
# Data saved in logfile.txt
```

---

## Next Steps

1. **Immediately:**
   - Install PlatformIO (5 min)
   - Create project (2 min)
   - Copy files (1 min)

2. **Upload & Test:**
   - Upload encoder test (1 min)
   - Verify RPM readings (5 min)

3. **Tuning:**
   - P-tuning with square wave (30 min)
   - Full PID tuning (30 min)
   - Record final gains

**Total: ~2-3 hours to full working system**

---

## Support

- **PlatformIO Docs**: https://docs.platformio.org/
- **VS Code Guide**: https://code.visualstudio.com/docs
- **ESP32 Setup**: https://docs.platformio.org/en/latest/platforms/espressif32.html
- **Arduino Framework**: https://docs.platformio.org/en/latest/frameworks/arduino.html

---

## You're Ready!

You have:
✓ 3 ready-to-use .cpp files
✓ Configuration file (platformio.ini)
✓ Setup guide (this file)
✓ Detailed documentation

**Start with the 30-second setup above.**

If anything doesn't work, check PLATFORMIO_SETUP.md for detailed troubleshooting.

Happy tuning! 🚀
