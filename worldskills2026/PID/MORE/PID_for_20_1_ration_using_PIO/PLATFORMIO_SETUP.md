# PlatformIO Setup Guide - .cpp Files for ESP32-S3

## Overview

PlatformIO is excellent for ESP32 development. You get:
- ✓ No board manager timeouts
- ✓ Better intellisense and code completion
- ✓ Integrated debugging
- ✓ Library management is cleaner
- ✓ Works with VS Code seamlessly

---

## Project Structure

After setting up PlatformIO, your project should look like this:

```
MyMotorProject/
├── platformio.ini          ← Edit this file
├── src/
│   └── main.cpp            ← Copy one of the .cpp files here
├── include/
├── lib/
├── test/
└── .vscode/
    └── settings.json
```

---

## Step 1: Install PlatformIO

### Option A: VS Code + PlatformIO Extension (Recommended)
1. Install **VS Code** (https://code.visualstudio.com/)
2. In VS Code, go to Extensions (Ctrl+Shift+X)
3. Search for "PlatformIO IDE"
4. Click **Install**
5. Reload VS Code

### Option B: PlatformIO CLI
```bash
pip install platformio
platformio --version  # Verify installation
```

---

## Step 2: Create New Project

### Using VS Code PlatformIO

1. Click **PlatformIO Home** icon in left sidebar (alien head)
2. Click **+ New Project**
3. Configure:
   - **Project Name**: `MotorPID` (or your choice)
   - **Board**: `Espressif ESP32-S3-DevKitC-1`
   - **Framework**: `Arduino`
   - **Location**: Choose your folder
4. Click **Finish**

### Using CLI

```bash
platformio init --board esp32-s3-devkitc-1 --framework arduino
```

---

## Step 3: Edit platformio.ini

This file controls your project settings. Find it in your project root and edit it:

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
upload_speed = 921600
build_flags = -DCORE_DEBUG_LEVEL=0

; Library dependencies (if needed)
lib_deps =

; Monitor port (auto-detect, or specify like COM3)
monitor_port = 
upload_port = 

; Optional: Parallel builds
build_flags = 
    -j4
```

**Key settings:**
- `monitor_speed`: Serial monitor baud rate (115200 is correct)
- `upload_speed`: Programming speed (921600 is faster, use 460800 if issues)
- `board`: ESP32-S3-DevKitC-1 (exact model for your board)

---

## Step 4: Copy .cpp File

Choose which sketch you want to upload and copy it:

### Step 1: Encoder Test
```
Copy: 01_EncoderTest_20-1Gearbox.cpp
To:   src/main.cpp
```

### Step 2: P-Tuning
```
Copy: 02_P_Tuning_20-1Gearbox.cpp
To:   src/main.cpp
```

### Step 3: Full PID
```
Copy: 03_PID_Complete_20-1Gearbox.cpp
To:   src/main.cpp
```

---

## Step 5: Build & Upload

### Using VS Code

1. Open **src/main.cpp** (your copied .cpp file)
2. In bottom status bar, click the **checkmark** (Build) or **arrow** (Build & Upload)
3. Or use keyboard shortcuts:
   - **Ctrl+Alt+B** = Build
   - **Ctrl+Alt+U** = Build & Upload

### Using CLI

```bash
# Build only
platformio run

# Build and upload
platformio run --target upload

# Build, upload, and open monitor
platformio run --target upload --verbose
platformio device monitor --baud 115200
```

---

## Step 6: Open Serial Monitor

### Using VS Code
1. Click **PlatformIO** icon in left sidebar
2. Under your project, expand **Devices**
3. Click your COM port or use **Platform I/O: Serial Port Monitor**
4. Or press **Ctrl+Shift+A** then select "Monitor"

### Using CLI
```bash
platformio device monitor --baud 115200
```

### Verify it works:
- You should see the startup message from the sketch
- Type `HELP` and press Enter (not Ctrl+Enter!)

---

## Common PlatformIO Issues & Fixes

### Issue: "Board not found"
```ini
; Make sure platformio.ini has:
board = esp32-s3-devkitc-1
; Not just esp32 or esp32-s3
```

### Issue: "Upload fails - cannot open port"
1. Check USB cable is connected
2. Verify COM port in Device Manager (Windows)
3. Set in platformio.ini:
```ini
upload_port = COM3
monitor_port = COM3
```

### Issue: "Import Arduino.h fails"
- Sometimes intellisense needs refresh
- Press **Ctrl+Shift+P** and type "PlatformIO: Rescan Libraries"

### Issue: "Serial monitor shows garbage"
```ini
monitor_speed = 115200  ; Must match Serial.begin(115200) in code
```

### Issue: Upload is very slow
```ini
upload_speed = 460800  ; Reduce from 921600 if having issues
```

---

## Using Multiple Sketches

You have 3 sketches to test. Here's the workflow:

```
1. Copy 01_EncoderTest_20-1Gearbox.cpp → src/main.cpp
   └─ Upload & test encoder

2. Copy 02_P_Tuning_20-1Gearbox.cpp → src/main.cpp
   └─ Upload & tune P-gain
   
3. Copy 03_PID_Complete_20-1Gearbox.cpp → src/main.cpp
   └─ Upload & tune full PID
```

**Tip:** Save copies of each sketch with different names if you want to compare:
```
src/
├── main.cpp (current)
├── encoder_test_backup.cpp
├── p_tuning_backup.cpp
└── pid_complete_backup.cpp
```

---

## Advantages of PlatformIO Over Arduino IDE

| Feature | Arduino IDE | PlatformIO |
|---------|------------|-----------|
| Board install | Slow, timeouts | Fast, offline |
| Code completion | Basic | Excellent |
| Library management | Manual | Automatic |
| Debugging | Limited | Full support |
| Multiple boards | One at a time | Easy switching |
| Compilation | Slower | Parallel builds |
| Serial monitor | Basic | Advanced |
| Build system | Arduino | CMake (flexible) |

---

## Serial Monitor in PlatformIO

### Open Monitor
- **VS Code**: Ctrl+Shift+A → search "monitor"
- **CLI**: `platformio device monitor`

### Common Commands in Monitor
```
SQUARE           → Square wave test
kP:0.02          → Set P-gain
STOP             → Stop motor
INFO             → Show status
HELP             → Show all commands
```

**Note:** End each command with Enter (Newline)

### Export Data for Graphing
In serial monitor, there's usually a save button:
1. Run your test (SQUARE, SINE, etc.)
2. Copy the CSV output
3. Save to a file: `data.csv`
4. Open in Excel or Python for graphing

---

## Example: Complete Setup from Scratch

### 1. Install
```bash
# Install PlatformIO CLI
pip install platformio

# Or install VS Code + PlatformIO extension
```

### 2. Create Project
```bash
mkdir my_motor_project
cd my_motor_project
platformio init --board esp32-s3-devkitc-1 --framework arduino
```

### 3. Copy Sketch
```bash
# Copy the encoder test to src/main.cpp
cp 01_EncoderTest_20-1Gearbox.cpp src/main.cpp
```

### 4. Configure (edit platformio.ini)
```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
upload_speed = 921600
```

### 5. Build & Upload
```bash
platformio run --target upload
```

### 6. Monitor
```bash
platformio device monitor --baud 115200
```

---

## Useful PlatformIO CLI Commands

```bash
# Build the project
platformio run

# Upload to board
platformio run --target upload

# Build and upload
platformio run --target upload --verbose

# Clean build (remove old files)
platformio run --target clean

# Monitor serial
platformio device monitor --baud 115200

# List available boards
platformio boards espressif32

# List connected devices
platformio device list

# Run specific environment
platformio run -e esp32-s3-devkitc-1

# Install library
platformio lib install "library_name"
```

---

## Debugging (Advanced)

PlatformIO supports debugging with a proper debugger. For advanced users:

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
debug_tool = esp-prog
debug_speed = 230400
```

But for basic tuning, Serial output is sufficient.

---

## VS Code Shortcuts for PlatformIO

| Shortcut | Action |
|----------|--------|
| **Ctrl+Alt+B** | Build project |
| **Ctrl+Alt+U** | Upload to board |
| **Ctrl+Alt+D** | Upload and monitor |
| **Ctrl+Shift+A** | Open PlatformIO quick access |
| **Ctrl+Shift+P** | VS Code command palette |
| **Ctrl+K, Ctrl+0** | Fold all code |
| **Ctrl+K, Ctrl+J** | Unfold all code |

---

## Tips & Tricks

### 1. Save project as template
After setting up once, save as template for future projects:
```bash
platformio project config --json-output > backup.json
```

### 2. Use environment variables
```ini
[platformio]
default_envs = esp32-s3-devkitc-1

[env:esp32-s3-devkitc-1]
; settings here
```

### 3. Faster compilation
Add to platformio.ini:
```ini
build_flags = -j4
```

### 4. Code formatting
In VS Code, right-click → Format Document (uses clang-format)

### 5. Intellisense issues
If code completion stops working:
- **Ctrl+Shift+P** → "PlatformIO: Rescan Libraries"
- Or restart VS Code

---

## Troubleshooting Upload Failures

### "espressif32: Unknown platform"
```bash
platformio platform install espressif32
```

### "Board not found"
```bash
# List available ESP32 boards
platformio boards | grep esp32-s3

# Must use: esp32-s3-devkitc-1
```

### "Cannot upload - permission denied"
**Linux/Mac:**
```bash
sudo chmod 666 /dev/ttyUSB0  # Replace with your port
```

**Windows:** Run VS Code as Administrator

### "Compilation takes forever"
- Reduce `upload_speed`
- Use `-j4` in build_flags (parallel compilation)
- Check CPU usage (might be indexing)

---

## File Sizes for Reference

```
Project size on disk: ~500 MB (includes dependencies)
Compiled firmware: ~1-2 MB
Memory usage: ~200 KB RAM, rest available for stack/heap
```

---

## Next Steps After Setup

1. ✓ Install PlatformIO
2. ✓ Create project for ESP32-S3
3. ✓ Copy encoder test → upload
4. ✓ Verify in serial monitor
5. ✓ Copy P-tuning sketch → upload
6. ✓ Adjust kP values
7. ✓ Copy PID sketch → upload
8. ✓ Full tuning

---

## Comparison: .ino vs .cpp

| Feature | .ino | .cpp |
|---------|------|------|
| Arduino IDE | ✓ | ✓ (with Arduino as IDE) |
| PlatformIO | ✓ | ✓ Preferred |
| Syntax | Same | Same |
| Preprocessing | Automatic | Requires #include <Arduino.h> |
| File location | Root | src/ folder |
| Function prototypes | Optional | Recommended |

The .cpp files are **identical** in functionality, just formatted for PlatformIO structure.

---

## Support & Resources

- **PlatformIO Docs**: https://docs.platformio.org/
- **VS Code Guide**: https://code.visualstudio.com/docs
- **ESP32 Platform**: https://docs.platformio.org/en/latest/platforms/espressif32.html
- **Arduino Framework**: https://docs.platformio.org/en/latest/frameworks/arduino.html

---

## Your PlatformIO Project is Ready!

You now have:
- ✓ 3 .cpp files ready to use
- ✓ PlatformIO configuration guide
- ✓ Upload and debugging instructions
- ✓ Serial monitor setup
- ✓ Troubleshooting guide

**Next:** Copy the encoder test .cpp file to `src/main.cpp` and upload!

Good luck with your PID tuning in PlatformIO! 🚀
