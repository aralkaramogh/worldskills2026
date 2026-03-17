#!/usr/bin/env python3
"""
ROS2 Bridge Node for ESP32 Differential Drive
Subscribes to cmd_vel and sends MOVE commands via serial
"""

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
import serial
import threading

class ESP32ROS2Bridge(Node):
    def __init__(self):
        super().__init__('esp32_ros2_bridge')
        
        # Declare parameters
        self.declare_parameter('port', '/dev/ttyUSB0')  # Change for Windows: COM3, COM4, etc.
        self.declare_parameter('baudrate', 115200)
        self.declare_parameter('timeout', 1.0)
        
        # Robot parameters (adjust to match your robot)
        self.declare_parameter('wheel_separation', 0.2)  # Distance between wheels in meters
        self.declare_parameter('wheel_radius', 0.05)     # Wheel radius in meters
        self.declare_parameter('max_linear_speed', 0.8)   # m/s
        self.declare_parameter('max_angular_speed', 2.0)  # rad/s
        
        # Get parameters
        port = self.get_parameter('port').value
        baudrate = self.get_parameter('baudrate').value
        
        # Serial connection
        try:
            self.ser = serial.Serial(port, baudrate, timeout=1)
            self.get_logger().info(f'Connected to ESP32 on {port} @ {baudrate} baud')
        except serial.SerialException as e:
            self.get_logger().error(f'Failed to connect to ESP32: {e}')
            self.ser = None
        
        # Subscribe to cmd_vel
        self.subscription = self.create_subscription(
            Twist,
            'cmd_vel',
            self.cmd_vel_callback,
            10
        )
        
        self.get_logger().info('ROS2 Bridge initialized. Listening on /cmd_vel')
    
    def cmd_vel_callback(self, msg: Twist):
        """
        Convert cmd_vel Twist to motor speeds and send to ESP32
        
        Twist contains:
        - linear.x: forward/backward velocity (m/s)
        - angular.z: rotation velocity (rad/s)
        """
        if self.ser is None:
            self.get_logger().warn('Serial connection not available')
            return
        
        # Get robot parameters
        wheel_sep = self.get_parameter('wheel_separation').value
        min_linear = self.get_parameter('max_linear_speed').value
        max_angular = self.get_parameter('max_angular_speed').value
        
        linear_vel = msg.linear.x      # m/s (forward/backward)
        angular_vel = msg.angular.z    # rad/s (rotation)
        
        # Differential drive kinematics
        # v_left  = (v - w*L/2) / R
        # v_right = (v + w*L/2) / R
        # where v = linear velocity, w = angular velocity, L = wheel separation, R = wheel radius
        
        left_speed_cmd = linear_vel - (angular_vel * wheel_sep / 2.0)
        right_speed_cmd = linear_vel + (angular_vel * wheel_sep / 2.0)
        
        # Convert m/s to percentage (-100 to 100)
        # Scale by max speeds
        left_percent = self.speed_to_percent(left_speed_cmd, min_linear)
        right_percent = self.speed_to_percent(right_speed_cmd, min_linear)
        
        # Send MOVE command to ESP32
        # Format: MOVE:<L>,<R>\n
        cmd = f"MOVE:{int(left_percent)},{int(right_percent)}\n"
        
        try:
            self.ser.write(cmd.encode())
            self.get_logger().debug(
                f'cmd_vel → MOVE | lin={linear_vel:.2f} m/s, ang={angular_vel:.2f} rad/s '
                f'→ L={int(left_percent)}%, R={int(right_percent)}%'
            )
        except serial.SerialException as e:
            self.get_logger().error(f'Serial write failed: {e}')
    
    @staticmethod
    def speed_to_percent(speed_m_s, max_speed):
        """Convert m/s to motor percentage (-100 to 100)"""
        percent = (speed_m_s / max_speed) * 100.0
        return max(-100, min(100, percent))  # Clamp to [-100, 100]
    
    def destroy_node(self):
        if self.ser and self.ser.is_open:
            self.ser.close()
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = ESP32ROS2Bridge()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
