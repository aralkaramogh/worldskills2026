# ROS2 + ESP32 Testing Checklist

## Phase 1: Verify ESP32 is Working Standalone

```bash
# 1. Connect ESP32 via USB
# 2. Identify serial port:
#    Linux/Mac:   ls /dev/tty* | grep -E "USB|ACM"
#    Windows:     Check Device Manager > Ports (COM#)

# 3. Connect with serial monitor
#    Option A—picocom:
picocom -b 115200 /dev/ttyUSB0

#    Option B—minicom:
minicom -D /dev/ttyUSB0 -b 115200

#    Option C—screen:
screen /dev/ttyUSB0 115200

#    Option D—Windows PowerShell:
$port = New-Object System.IO.Ports.SerialPort("COM3", 115200)
$port.Open()
$port.WriteLine("STATUS")
$port.Close()

# 4. You should see startup banner:
#    ╔══════════════════════════════════════════╗
#    ║  Diff Drive Teleop  |  ESP32-S3 + DD10A  ║
#    ...

# 5. Test manual commands:
MOVE:30,30    ← Both motors 30% forward
MOVE:0,0      ← Stop
MOVE:50,-50   ← Spin right
STATUS        ← Show current state
V             ← Toggle verbose
```

**If ESP32 responds, proceed to Phase 2.**

---

## Phase 2: Install Bridge Dependencies

```bash
# Install pyserial
pip install pyserial

# Or on ROS2 Ubuntu:
sudo apt install python3-serial

# Verify:
python3 -c "import serial; print('✓ pyserial installed')"
```

---

## Phase 3: Test Bridge Standalone (No ROS2)

```bash
# 1. Edit esp32_ros2_bridge.py, change line ~60 to your port:
#    Windows: self.ser = serial.Serial("COM3", 115200, timeout=1)
#    Linux:   self.ser = serial.Serial("/dev/ttyUSB0", 115200, timeout=1)

# 2. Run bridge script directly (outside ROS2):
python3 esp32_ros2_bridge.py

# Output should show:
# Connected to ESP32 on /dev/ttyUSB0 @ 115200 baud
# ROS2 Bridge initialized. Listening on /cmd_vel
# [FATAL] RclInitError: Failed to initialize...  ← OK, ROS2 not running yet

# 3. Stop with Ctrl+C
```

**If no serial errors, connection works!**

---

## Phase 4: Start ROS2 Bridge

```bash
# Terminal 1: Start bridge node
ros2 run esp32_bridge esp32_ros2_bridge --ros-args -p port:=/dev/ttyUSB0

# Expected output:
# [INFO] [esp32_ros2_bridge]: Connected to ESP32 on /dev/ttyUSB0 @ 115200 baud
# [INFO] [esp32_ros2_bridge]: ROS2 Bridge initialized. Listening on /cmd_vel
```

---

## Phase 5: Publish Test Commands

### Method A: Command Line (Simplest)

```bash
# Terminal 2: Forward motion
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.5}, angular: {z: 0.0}}"

# → Robot moves forward
# → Bridge logs show: L=60%, R=60%
```

### Method B: Continuous Motion

```bash
# Terminal 2: Keep publishing (robot moves continuously)
ros2 topic pub --rate 10 /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.5}, angular: {z: 0.0}}"

# Press Ctrl+C to stop
```

### Method C: Test Different Motions

```bash
# Forward (simple)
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.5, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.0}}"

# Backward
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: -0.5, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.0}}"

# Turn right (no forward motion)
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.0, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: -1.0}}"

# Turn left
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.0, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 1.0}}"

# Arc forward-left (forward + left turn)
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.5, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.5}}"

# Tight arc
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.2, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 1.5}}"
```

---

## Phase 6: Monitor and Debug

```bash
# Terminal 3: Listen to bridge output (if logging is enabled)
ros2 node info /esp32_ros2_bridge

# Check if cmd_vel subscriber is active
ros2 topic info /cmd_vel
# Should show: "Publishers: 1" when you pub

# Monitor serial traffic (advanced)
cat /dev/ttyUSB0 > /tmp/esp32_log.txt &
# Send commands, then: cat /tmp/esp32_log.txt
```

---

## Expected Results

### Test 1: Straight Forward (should work ✓)
```bash
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.5}}"
```
- **Expected**: Both motors forward at same speed
- **Log**: `L=60%, R=60%`
- **Physical**: Robot moves straight forward
- **If not**: Check motor wiring, polarity, encoder feedback

### Test 2: Spin Right (should work ✓)
```bash
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{angular: {z: -1.0}}"
```
- **Expected**: Left motor forward, right motor backward
- **Log**: `L=50%, R=-50%`
- **Physical**: Robot spins right (clockwise looking from above)
- **If reversed**: Swap motor directions or polarity

### Test 3: Arc (should work ✓)
```bash
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.5}, angular: {z: 0.5}}"
```
- **Expected**: Right motor faster than left
- **Log**: `L=40%, R=60%`
- **Physical**: Robot curves to the left
- **If wrong**: Adjust `wheel_separation` parameter

---

## Troubleshooting Matrix

| Symptom | Likely Cause | Fix |
|---------|--------------|-----|
| Bridge won't start | Wrong serial port | Check `ls /dev/tty*` → update port in code |
| "No subscribers" | Bridge not running | Start bridge in Terminal 1 |
| Robot doesn't move | Serial connection lost | Reconnect USB, restart bridge |
| Only one motor moves | GPIO pin issue | Check pin assignments in ESP32 code |
| Moves backward | Motor polarity inverted | Swap wires or toggle `INVERT_LEFT/RIGHT` |
| Turns wrong direction | Encoder/driver logic | Verify with manual `MOVE:50,-50` |
| Jerky motion | Watchdog timeout | Check watchdog timing in code |
| No serial output | baud mismatch | Verify 115200 in both ESP32 and bridge |

---

## Phase 7: Next Level—Teleop Keyboard

```bash
# Install teleop package
sudo apt install ros-humble-teleop-twist-keyboard

# Terminal 2: Keyboard control (after bridge running)
ros2 run teleop_twist_keyboard teleop_twist_keyboard

# Use arrow keys (U/J/I/K/O/L) to move robot
# See terminal output for key mapping
```

---

## Phase 8: Production Ready

Once all tests pass:

1. Create ROS2 launch file (see ROS2_INTEGRATION_GUIDE.md)
2. Add to systemd or startup script
3. Integrate with navigation stack (Nav2, SLAM)
4. Add odometry feedback for closed-loop control

