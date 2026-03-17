# Quick Start: ESP32 + ROS2 cmd_vel

## Your Setup (Simplified)

```
ROS2 Publisher (any node)
        ↓
    /cmd_vel (Twist)
        ↓
    esp32_ros2_bridge.py
        ↓
    Serial (MOVE:<L>,<R>)
        ↓
    ESP32 teleop_esp32s3_v1_3.cpp
        ↓
        Cytron DD10A Motors
```

## 5-Minute Setup

### 1. Install Python dependencies
```bash
pip install pyserial
# On ROS2 Ubuntu: sudo apt install python3-serial
```

### 2. Copy the bridge
- Copy `esp32_ros2_bridge.py` to your ROS2 workspace

### 3. Update serial port
Edit in `esp32_ros2_bridge.py` or pass as parameter:
```bash
# Linux/Mac
ros2 run esp32_bridge esp32_ros2_bridge --ros-args -p port:=/dev/ttyUSB0

# Windows
ros2 run esp32_bridge esp32_ros2_bridge --ros-args -p port:=COM3
```

### 4. Run the bridge
```bash
ros2 run esp32_bridge esp32_ros2_bridge
```

### 5. Publish cmd_vel
```bash
# Terminal 2:
ros2 topic pub --rate 10 /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.5, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.0}}"
```

**Your robot moves forward!** ✓

---

## What the Bridge Does

| ROS2 cmd_vel | → | ESP32 MOVE Command |
|---|---|---|
| `linear.x = 0.5 m/s` | → | Both motors 60% forward |
| `linear.x = 0.0, angular.z = 1.0 rad/s` | → | Left motor -40%, Right motor +40% (spin) |
| `linear.x = 0.3, angular.z = 0.5 rad/s` | → | Differential drive (forward-left arc) |

**Your existing `MOVE:<L>,<R>` command already works!** No ESP32 code changes needed.

---

## Complete Message Format

Your ESP32 already supports:
```
MOVE:<left_percent>,<right_percent>
```

Where:
- `left_percent` and `right_percent`: -100 to 100
- `-100` = full backward
- `0` = stop
- `100` = full forward

The bridge calculates these from linear + angular velocity using **differential drive kinematics**.

---

## Example Commands

### Straight (no turn)
```
MOVE:50,50      ← Both motors forward
```

### Spin right
```
MOVE:50,-50     ← Left forward, right backward
```

### Arc forward-left
```
MOVE:30,50      ← Left slower, right faster → curves left
```

**All already supported by your code!**

---

## Optional: Add ROS2 Native (Advanced)

For direct ROS2 on ESP32 (no bridge):
  - Use **micro-ROS** framework
  - Add `rclc` library to platformio.ini
  - Subscribe directly to `/cmd_vel` on ESP32
  - Requires network or serial bridge at lower level

*Not recommended unless you need distributed real-time control.*

Your current bridge approach is simpler and more reliable.

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| `Connection refused` | Check serial port with `ls /dev/tty*` or Device Manager |
| Robot doesn't move | Send `STATUS` command to ESP32 to verify connection |
| Backward motion | Swap motor polarities or toggle `INVERT_LEFT`/`INVERT_RIGHT` |
| Only one motor | Check individual pins with `MOVE:50,0` and `MOVE:0,50` |

---

## Files Provided

1. **esp32_ros2_bridge.py** — Drop this in your ROS2 package
2. **ROS2_INTEGRATION_GUIDE.md** — Full documentation
3. **Your teleop_esp32s3_v1_3.cpp** — Already supports MOVE! No changes needed.

