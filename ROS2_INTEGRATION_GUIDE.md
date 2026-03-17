# ROS2 + ESP32 cmd_vel Integration Guide

## Overview
This setup allows ROS2 to control your ESP32 differential drive robot by publishing `cmd_vel` (Twist) messages. The Python bridge converts these velocity commands to MOVE commands your ESP32 understands.

## Prerequisites
- **ROS2** installed (Humble or newer recommended)
- **Python 3** with `pyserial`
- ESP32 running your existing teleop code (v1.3)
- Serial connection (USB cable) between PC and ESP32

## Setup Steps

### Step 1: Install Python Dependencies
```bash
pip install pyserial
```

On Ubuntu/WSL with ROS2:
```bash
sudo apt install python3-serial
```

### Step 2: Locate the Bridge Script
- Copy `esp32_ros2_bridge.py` to your ROS2 workspace:
```bash
cp esp32_ros2_bridge.py ~/ros2_ws/src/esp32_bridge/
chmod +x ~/ros2_ws/src/esp32_bridge/esp32_ros2_bridge.py
```

### Step 3: Create a ROS2 Package (Optional but Recommended)
```bash
cd ~/ros2_ws/src
ros2 pkg create esp32_bridge --build-type ament_python
```

Copy `esp32_ros2_bridge.py` into the package.

### Step 4: Update the Launch File
Create `esp32_bridge/launch/bridge.launch.py`:

```python
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='esp32_bridge',
            executable='esp32_ros2_bridge',
            name='esp32_bridge',
            parameters=[
                {'port': '/dev/ttyUSB0'},        # Windows: 'COM3', COM4', etc.
                {'baudrate': 115200},
                {'wheel_separation': 0.2},       # Meters between wheels
                {'wheel_radius': 0.05},          # Meter radius
                {'max_linear_speed': 0.8},       # m/s
                {'max_angular_speed': 2.0},      # rad/s
            ],
            output='screen',
        ),
    ])
```

### Step 5: Build and Run
```bash
cd ~/ros2_ws
colcon build --packages-select esp32_bridge
source install/setup.bash
ros2 launch esp32_bridge bridge.launch.py
```

## How It Works

### Data Flow
1. **ROS2 Node** publishes on `/cmd_vel` topic with `Twist` message:
   ```
   Twist:
     linear.x   → forward/backward velocity (m/s)
     angular.z  → rotation velocity (rad/s)
   ```

2. **Python Bridge** subscribes to `/cmd_vel` and converts to motor percentages using differential drive kinematics:
   ```
   left_motor  = linear - (angular × wheel_separation / 2)
   right_motor = linear + (angular × wheel_separation / 2)
   ```

3. **Sends MOVE Command** to ESP32 via serial:
   ```
   MOVE:<left_percent>,<right_percent>
   ```

4. **ESP32** executes `MOVE` command (already supported in your code!)

## Testing

### Test 1: Direct Serial Command
With ESP32 connected and running v1.3 code:
```bash
# Send directly to ESP32
echo "MOVE:30,30" > /dev/ttyUSB0

# Windows PowerShell:
echo "MOVE:30,30" | Out-File -NoNewline \\.\COM3
```

### Test 2: Publish cmd_vel from Command Line
```bash
# Terminal 1: Start bridge
ros2 launch esp32_bridge bridge.launch.py

# Terminal 2: Publish forward motion (0.5 m/s, no rotation)
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.5, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.0}}"

# Turning in place (0 linear, 1.0 rad/s clockwise)
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.0, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 1.0}}"

# Forward-left diagonal
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.5, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.5}}"
```

### Test 3: Use teleoperation (keyboard control)
```bash
# If you have teleop_twist_keyboard installed:
ros2 run teleop_twist_keyboard teleop_twist_keyboard
```

## Configuration

### Key Parameters (in launch file)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `port` | `/dev/ttyUSB0` | Serial port (Windows: `COM3` etc.) |
| `baudrate` | `115200` | Must match ESP32 baud rate |
| `wheel_separation` | `0.2 m` | Distance between wheel centers |
| `wheel_radius` | `0.05 m` | Wheel radius |
| `max_linear_speed` | `0.8 m/s` | Max forward speed |
| `max_angular_speed` | `2.0 rad/s` | Max rotation speed |

### Tuning Tips
- **Slow robot?** Increase `max_linear_speed` / `max_angular_speed`
- **Turning radius wrong?** Adjust `wheel_separation` to match your robot's actual dimension
- **Speed doesn't match?** Verify motor speed caps in `teleop_esp32s3_v1_3.cpp` (FWD_CAP, TURN_CAP)

## Troubleshooting

### Bridge won't connect to ESP32
```bash
# Check available ports
ls /dev/tty*              # Linux/Mac
COM port list             # Windows: Device Manager or "mode" command

# Check ESP32 is running (should see startup message)
minicom -D /dev/ttyUSB0 -b 115200
screen /dev/ttyUSB0 115200
```

### Robot doesn't move when publishing cmd_vel
1. Check bridge logs: `ros2 service call /logger_level ros2_logging ...`
2. Verify serial connection: `echo "STATUS" > /dev/ttyUSB0`
3. Ensure ESP32 code is running (press `V` for verbose, send `W` to test)

### Movement is backwards
- Swap signs of `INVERT_LEFT` / `INVERT_RIGHT` in `teleop_esp32s3_v1_3.cpp`
- Or swap motor wires

### Only one motor moves
- Check motor pin definitions (`DIR_L`, `PWM_L`, etc.)
- Test with manual `MOVE` command: `MOVE:50,0` and `MOVE:0,50`

## Advanced: Custom ROS2 Message

For better control, create a custom message instead of generic Twist:
- Include motor speed setpoints directly
- Add diagnostics (current, temperature)
- Synchronize timestamp with ESP32

See ROS2 documentation: https://docs.ros.org/en/humble/Tutorials/Beginner-Client-Libraries/Creating-A-Custom-ROS2-Message.html

## Next Steps

1. **Odometry feedback**: Add encoder support to publish `/odom` back to ROS2
2. **Safety**: Implement timeout (move stops if no cmd_vel for X ms)
3. **Diagnostics**: Publish motor state, battery voltage, etc.
4. **SLAM/Nav2**: Use with `robot_localization`, `slam_toolbox`, `nav2_stack`

---

**Files provided:**
- `esp32_ros2_bridge.py` — Main bridge node
- This guide
- Your existing `teleop_esp32s3_v1_3.cpp` already supports MOVE command!

