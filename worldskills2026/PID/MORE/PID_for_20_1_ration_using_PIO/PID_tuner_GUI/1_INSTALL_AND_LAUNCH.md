# 🚀 Quick Start: PID Tuner GUI Installation & Launch

## Requirements

- **Python 3.7+**
- **Your COM port** (e.g., COM10, /dev/ttyUSB0)
- **Motor** connected and running encoder test verified

---

## 1️⃣ Install Required Packages

### One Command
```bash
pip install PyQt5 pyserial matplotlib numpy
```

### Or Step by Step
```bash
pip install PyQt5          # GUI framework
pip install pyserial       # Serial communication
pip install matplotlib     # Graphing
pip install numpy          # Math/arrays
```

### Verify Installation
```bash
python -c "import PyQt5; import serial; import matplotlib; import numpy; print('✓ All installed')"
```

Should show: `✓ All installed`

---

## 2️⃣ Launch the GUI

### Basic Launch
```bash
python pid_tuner_gui.py
```

### Expected Startup (first time takes longer):
```
Loading... (matplotlib initialization)
[app window opens with:
  - Left panel: Controls
  - Right panel: 4 blank graphs
  - Connection status: "❌ Disconnected"
]
```

---

## 3️⃣ Connect to Your Motor

### Steps:
```
1. Click "🔄 Refresh" button
   → Lists available COM ports

2. Select your port from dropdown
   (e.g., COM10, /dev/ttyUSB0)

3. Click "✓ Connect" button
   → Should turn green
   → Status changes to "✓ Connected"

4. Ready to tune!
```

### If Connection Fails:
```
❌ "No ports found"
  → Check USB cable connection
  → Close Arduino IDE (releases port)
  → Try "Refresh" again

❌ "Permission denied" (Linux/Mac)
  → sudo chmod 666 /dev/ttyUSB0
  → Or run with: sudo python pid_tuner_gui.py

❌ "Port in use"
  → Close Arduino IDE Serial Monitor
  → Close any other terminal using the port
  → Try "Refresh" and "Connect" again
```

---

## 4️⃣ Start Tuning

### Simple Test
```
1. Select waveform: "SQUARE"

2. Click "▶ Start Test"
   → Motor should start moving
   → Graphs should show blue/red lines

3. Watch the graphs update:
   - Top-left: Speed response
   - Top-right: Error curve
   - Bottom-left: PWM signal
   - Bottom-right: Gain values

4. Type in left panel:
   kP = 0.03  (using ◀ ▶ buttons or typing)
   
5. Click "Send kP"
   → Changes motor gain in real-time

6. Watch graphs adapt to new gain
```

### Next: Follow "P-Tuning Phase" in PID_GUI_GUIDE.md

---

## Common Issues & Fixes

### Issue: App Won't Start
```bash
❌ Error: No module named 'PyQt5'
→ pip install PyQt5

❌ Error: No module named 'serial'
→ pip install pyserial

❌ Error: No module named 'matplotlib'
→ pip install matplotlib
```

### Issue: App Launches but Blank
```
Possible causes:
  1. Slow first launch (matplotlib loading)
     → Wait 5 seconds
  
  2. Dark theme issues
     → Edit pid_tuner_gui.py, change:
       app.setStyle('Fusion')
       → app.setStyle('Windows')

  3. Display server issue (Linux)
     → DISPLAY=:0 python pid_tuner_gui.py
```

### Issue: Motor Won't Connect
```
❌ Port shows but won't connect:
  1. Close Arduino IDE completely
  2. Unplug/replug USB cable
  3. Restart Python terminal
  4. Click "🔄 Refresh"
  5. Try "✓ Connect" again
```

### Issue: Graphs Frozen / Not Updating
```
❌ Graphs don't show data:
  1. Check connection status (green checkmark?)
  2. Check serial monitor output is correct
  3. Try disconnecting/reconnecting
  4. Click "🗑️ Clear Data" to reset
  5. Run SQUARE test again
```

---

## File Structure

```
Your project folder:
├── pid_tuner_gui.py           ← Main application
├── pid_presets.json           ← Created automatically
├── PID_GUI_GUIDE.md           ← Usage guide
├── AUTOTUNING_EXPLAINED.md    ← Auto-tune theory
└── QUICK_FIX.md               ← Troubleshooting
```

---

## Configuration (Optional)

Edit `pid_tuner_gui.py` to customize:

```python
# Line 18-20
BUFFER_SIZE = 1000        # Max data points (1000 = 10 seconds at 100Hz)
UPDATE_INTERVAL = 100     # Graph refresh (ms, 100 = 10 Hz)
BAUD_RATE = 115200        # Serial baudrate

# Change if needed:
BUFFER_SIZE = 500         # Smaller = faster, less memory
UPDATE_INTERVAL = 50      # Faster = smoother, more CPU
BAUD_RATE = 9600          # Only if ESP32 uses different rate
```

---

## Platform-Specific Notes

### Windows
```
✓ Should work out-of-the-box
✓ COM ports show as COM1, COM2, etc.
✓ PyQt5 fully supported
⚠️ If UAC blocks: Run as Administrator
```

### macOS
```
✓ Works well with PyQt5
⚠️ Port shows as /dev/cu.usbserial-xxxxx
✓ Install via: pip install PyQt5
✓ If still issues: brew install python3 && pip install PyQt5
```

### Linux (Ubuntu/Debian)
```
✓ Works with PyQt5
⚠️ Port shows as /dev/ttyUSB0 or /dev/ttyACM0
⚠️ May need: sudo python3 pid_tuner_gui.py
   or: sudo chmod 666 /dev/ttyUSB0 first

Install if needed:
sudo apt install python3-pyqt5 python3-pyqt5.qtopengl
```

---

## Typical Tuning Workflow

```
Time: 0:00
├─ Launch: python pid_tuner_gui.py
├─ Connect: Click port dropdown → Click "Connect"
│
├─ P-Tuning Phase (0:05 - 0:20)
│  ├─ Select "SQUARE"
│  ├─ Click "▶ Start Test"
│  ├─ Adjust kP: ◀ button to decrease, ▶ to increase
│  ├─ Click "Send kP"
│  ├─ Watch graphs for 5 seconds
│  ├─ Repeat for different kP values (0.020, 0.025, 0.030, 0.035)
│  └─ Record best: kP = 0.030 ✓
│
├─ D-Tuning Phase (0:20 - 0:25)
│  ├─ Keep best kP, set kD = 0.005
│  ├─ "▶ Start Test" again
│  ├─ Adjust kD using buttons
│  ├─ Look for: Smooth response, no overshoot
│  └─ Record best: kD = 0.008 ✓
│
├─ Save Preset (0:25 - 0:26)
│  ├─ Enter name: "20-1_final"
│  └─ Click "💾 Save"
│
├─ Final Verification (0:26 - 0:30)
│  ├─ Select "SINE" waveform
│  ├─ "▶ Start Test"
│  └─ Verify smooth tracking
│
└─ Done! (0:30)
   Motor is optimally tuned.
```

---

## Tips for Best Results

### Before You Start
- [ ] Arduino IDE closed
- [ ] USB cable secure
- [ ] Motor can spin freely
- [ ] Encoder working (verified in Step 1)
- [ ] Baud rate = 115200

### During Tuning
- [ ] Let each test run 10-15 seconds
- [ ] Watch graphs, not just numbers
- [ ] Take screenshots of good results
- [ ] Note your observations
- [ ] Save presets often

### After Tuning
- [ ] Save final preset with descriptive name
- [ ] Document values in spreadsheet
- [ ] Test on different motors (if applicable)
- [ ] Keep presets.json as backup

---

## Video Tour (Text Version)

### What You'll See When Connected

```
    ┌─────────────────────────────────────────────┐
    │ 🎛️ PID Motor Tuning Suite                  │
    │                                             │
    │  LEFT PANEL          │  RIGHT PANEL         │
    │  ─────────────────   │  (4 GRAPHS)          │
    │  📊 Connection       │                      │
    │  ✓ Connected         │  ┌───────────────┐   │
    │                      │  │ Speed (RPM)   │   │
    │  ⚙️ PID Gains        │  │ Blue=Target   │   │
    │  kP: [0.030] ◀ ▶     │  │ Red=Actual    │   │
    │  kI: [0.001] ◀ ▶     │  └───────────────┘   │
    │  kD: [0.008] ◀ ▶     │                      │
    │                      │  ┌───────────────┐   │
    │  📈 Test            │  │ Error (RPM)   │   │
    │  [SQUARE ▼]         │  │ Green decline │   │
    │  [▶ Start] [⏹ Stop] │  └───────────────┘   │
    │                      │                      │
    │  💾 Presets         │  ┌───────────────┐   │
    │  [Save] [Load]      │  │ PWM Signal    │   │
    │  [20-1_final ▼]     │  │ Orange line   │   │
    │                      │  └───────────────┘   │
    │  🤖 Auto-Tune       │                      │
    │  [🎯 Start]         │  ┌───────────────┐   │
    │                      │  │ Gains (kP,kI,│   │
    │  📊 Current Data    │  │ kD)           │   │
    │  Time: 5.32s        │  └───────────────┘   │
    │  Setpoint: 300.0    │                      │
    │  Actual: 295.5      │                      │
    │  PWM: 85%           │                      │
    │                      │                      │
    └─────────────────────────────────────────────┘
```

---

## Support

### If Something Goes Wrong

1. **Check this guide first** (especially Troubleshooting section)
2. **Read PID_GUI_GUIDE.md** for usage questions
3. **Read AUTOTUNING_EXPLAINED.md** for tuning theory

### Debugging Tips

```bash
# Check Python version
python --version
# Should be 3.7 or higher

# Check packages installed
pip list | grep -E "PyQt5|pyserial|matplotlib"

# Run in verbose mode (add to code):
import logging
logging.basicConfig(level=logging.DEBUG)
```

---

## Next Steps

1. ✓ Install packages
2. ✓ Launch GUI
3. ✓ Connect to motor
4. → Read **PID_GUI_GUIDE.md** (How to use)
5. → Read **AUTOTUNING_EXPLAINED.md** (Theory)
6. → Start P-tuning phase

---

**You're ready to tune like a pro!** 🎛️ 🚀

Questions? Check the markdown files for detailed guides.
