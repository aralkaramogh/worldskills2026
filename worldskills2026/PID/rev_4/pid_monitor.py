#!/usr/bin/env python3
"""
ESP32 PID Motor Controller - Real-Time Visualization & Data Logging
Reads serial data from ESP32 and displays live plots with matplotlib
Also saves data to CSV for later analysis
"""

import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.widgets import Slider, Button
from collections import deque
from datetime import datetime
import threading
import csv
import sys

class MotorPIDMonitor:
    def __init__(self, port='COM3', baudrate=115200, max_points=500):
        """
        Initialize the PID Monitor
        port: Serial port (e.g., 'COM3' on Windows, '/dev/ttyUSB0' on Linux)
        baudrate: Serial communication speed
        max_points: Maximum data points to display
        """
        self.port = port
        self.baudrate = baudrate
        self.max_points = max_points
        
        # Data storage
        self.time_data = deque(maxlen=max_points)
        self.desired_vel = deque(maxlen=max_points)
        self.current_vel = deque(maxlen=max_points)
        self.pid_output = deque(maxlen=max_points)
        
        self.start_time = None
        self.point_count = 0
        
        # Serial connection
        self.ser = None
        self.running = True
        self.data_lock = threading.Lock()
        
        # Data logging
        self.csv_filename = f"pid_data_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        self.csv_file = None
        self.csv_writer = None
        self.init_csv_file()
        
        # Connect to serial port
        self.connect_serial()
        
        # Start serial reading thread
        self.serial_thread = threading.Thread(target=self.read_serial, daemon=True)
        self.serial_thread.start()
        
    def connect_serial(self):
        """Establish serial connection"""
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            print(f"✓ Connected to {self.port} at {self.baudrate} baud")
            # Give ESP32 time to reset
            import time
            time.sleep(2)
        except Exception as e:
            print(f"✗ Failed to connect to {self.port}: {e}")
            print("\nAvailable ports:")
            import serial.tools.list_ports
            for port, desc, hwid in serial.tools.list_ports.comports():
                print(f"  {port}: {desc}")
            sys.exit(1)
    
    def init_csv_file(self):
        """Initialize CSV file for data logging"""
        try:
            self.csv_file = open(self.csv_filename, 'w', newline='')
            self.csv_writer = csv.writer(self.csv_file)
            self.csv_writer.writerow(['Time(s)', 'Desired_Velocity', 'Current_Velocity', 'PID_Output'])
            print(f"✓ Data logging to: {self.csv_filename}")
        except Exception as e:
            print(f"Warning: Could not open CSV file: {e}")
    
    def read_serial(self):
        """Read data from ESP32 serial port (runs in separate thread)"""
        while self.running:
            if self.ser and self.ser.in_waiting:
                try:
                    line = self.ser.readline().decode('utf-8').strip()
                    
                    if line and ',' in line:
                        # Parse: desired_vel,current_vel,pid_output
                        try:
                            parts = line.split(',')
                            if len(parts) == 3:
                                d_vel = float(parts[0])
                                c_vel = float(parts[1])
                                pid_out = float(parts[2])
                                
                                # Initialize start time on first data
                                if self.start_time is None:
                                    from time import time
                                    self.start_time = time()
                                
                                current_time = time() - self.start_time
                                
                                # Store data with thread safety
                                with self.data_lock:
                                    self.time_data.append(current_time)
                                    self.desired_vel.append(d_vel)
                                    self.current_vel.append(c_vel)
                                    self.pid_output.append(pid_out)
                                    self.point_count += 1
                                    
                                    # Log to CSV
                                    if self.csv_writer:
                                        self.csv_writer.writerow([current_time, d_vel, c_vel, pid_out])
                                
                        except ValueError:
                            pass  # Ignore parsing errors
                    
                    elif line:  # Other serial output
                        if any(x in line for x in ['===', 'GPIO', 'Status', 'updated', 'Error']):
                            print(f"ESP32: {line}")
                
                except UnicodeDecodeError:
                    pass  # Ignore decoding errors
            else:
                import time
                time.sleep(0.01)  # Prevent CPU spinning
    
    def send_command(self, command):
        """Send command to ESP32"""
        if self.ser:
            self.ser.write(f"{command}\n".encode())
            print(f"→ Sent: {command}")
    
    def create_plots(self):
        """Create figure with subplots and controls"""
        self.fig, (self.ax1, self.ax2) = plt.subplots(2, 1, figsize=(14, 10))
        
        # Main velocity plot
        self.ax1.set_ylabel('Velocity (units/sec)', fontsize=11, fontweight='bold')
        self.ax1.set_title('PID Motor Velocity Control - Real-Time', fontsize=13, fontweight='bold')
        self.ax1.grid(True, alpha=0.3)
        self.line_desired, = self.ax1.plot([], [], 'b-', label='Desired Velocity', linewidth=2)
        self.line_current, = self.ax1.plot([], [], 'r-', label='Current Velocity', linewidth=2)
        self.ax1.legend(loc='upper left', fontsize=10)
        self.ax1.set_ylim(-50, 150)
        
        # PID Output plot
        self.ax2.set_xlabel('Time (seconds)', fontsize=11, fontweight='bold')
        self.ax2.set_ylabel('Motor Output (PWM 0-255)', fontsize=11, fontweight='bold')
        self.ax2.set_title('PID Controller Output', fontsize=12, fontweight='bold')
        self.ax2.grid(True, alpha=0.3)
        self.line_output, = self.ax2.plot([], [], 'g-', label='PID Output', linewidth=2)
        self.ax2.legend(loc='upper left', fontsize=10)
        self.ax2.set_ylim(-300, 300)
        
        # Control panel
        ax_kp = plt.axes([0.2, 0.35, 0.3, 0.02])
        ax_ki = plt.axes([0.2, 0.30, 0.3, 0.02])
        ax_kd = plt.axes([0.2, 0.25, 0.3, 0.02])
        ax_vel = plt.axes([0.2, 0.20, 0.3, 0.02])
        
        self.slider_kp = Slider(ax_kp, 'Kp', 0.1, 5.0, valinit=0.5, color='orange')
        self.slider_ki = Slider(ax_ki, 'Ki', 0.0, 1.0, valinit=0.1, color='orange')
        self.slider_kd = Slider(ax_kd, 'Kd', 0.0, 1.0, valinit=0.05, color='orange')
        self.slider_vel = Slider(ax_vel, 'Velocity', -100, 200, valinit=50, color='skyblue')
        
        # Slider callbacks
        self.slider_kp.on_changed(lambda val: self.send_command(f"Kp={val:.3f}"))
        self.slider_ki.on_changed(lambda val: self.send_command(f"Ki={val:.3f}"))
        self.slider_kd.on_changed(lambda val: self.send_command(f"Kd={val:.3f}"))
        self.slider_vel.on_changed(lambda val: self.send_command(f"v={val:.1f}"))
        
        # Buttons
        ax_stop = plt.axes([0.2, 0.10, 0.08, 0.04])
        ax_status = plt.axes([0.35, 0.10, 0.08, 0.04])
        ax_clear = plt.axes([0.50, 0.10, 0.08, 0.04])
        
        btn_stop = Button(ax_stop, 'STOP', color='lightcoral')
        btn_status = Button(ax_status, 'Status', color='lightblue')
        btn_clear = Button(ax_clear, 'Clear', color='lightgray')
        
        btn_stop.on_clicked(lambda event: self.send_command("stop"))
        btn_status.on_clicked(lambda event: self.send_command("status"))
        btn_clear.on_clicked(lambda event: self.clear_data())
        
        # Stats text
        self.text_stats = self.fig.text(0.65, 0.35, '', fontfamily='monospace', fontsize=9)
        
        plt.subplots_adjust(bottom=0.45)
        
        return self.fig
    
    def clear_data(self):
        """Clear all stored data"""
        with self.data_lock:
            self.time_data.clear()
            self.desired_vel.clear()
            self.current_vel.clear()
            self.pid_output.clear()
        print("✓ Data cleared")
    
    def update_plot(self, frame):
        """Update plot data (called repeatedly by animation)"""
        with self.data_lock:
            if len(self.time_data) > 0:
                time_list = list(self.time_data)
                desired_list = list(self.desired_vel)
                current_list = list(self.current_vel)
                output_list = list(self.pid_output)
                
                # Update velocity plot
                self.line_desired.set_data(time_list, desired_list)
                self.line_current.set_data(time_list, current_list)
                
                # Update output plot
                self.line_output.set_data(time_list, output_list)
                
                # Auto-scale x-axis
                if len(time_list) > 0:
                    max_time = max(time_list)
                    self.ax1.set_xlim(max(0, max_time - 30), max_time + 2)
                    self.ax2.set_xlim(max(0, max_time - 30), max_time + 2)
                
                # Calculate and display statistics
                if len(current_list) > 20:
                    recent_current = current_list[-20:]
                    recent_desired = desired_list[-20:]
                    
                    steady_error = abs(recent_current[-1] - recent_desired[-1])
                    avg_error = sum(abs(c - d) for c, d in zip(recent_current, recent_desired)) / len(recent_current)
                    
                    stats_text = (
                        f"Stats (last 20 points):\n"
                        f"Current Vel: {current_list[-1]:.1f}\n"
                        f"Desired Vel: {desired_list[-1]:.1f}\n"
                        f"Steady Error: {steady_error:.1f}\n"
                        f"Avg Error: {avg_error:.1f}\n"
                        f"Data Points: {self.point_count}\n"
                        f"File: {self.csv_filename}"
                    )
                    self.text_stats.set_text(stats_text)
        
        return self.line_desired, self.line_current, self.line_output, self.text_stats
    
    def run(self):
        """Create and display the plot"""
        self.create_plots()
        
        # Create animation
        ani = animation.FuncAnimation(
            self.fig, self.update_plot, interval=100, blit=True, cache_frame_data=False
        )
        
        print("\n" + "="*60)
        print("ESP32 PID Monitor Ready!")
        print("="*60)
        print("Controls:")
        print("  • Use sliders to adjust Kp, Ki, Kd, and desired velocity")
        print("  • Buttons: STOP (emergency), Status (serial info), Clear (data)")
        print("  • Serial commands also work (type in serial monitor)")
        print("="*60 + "\n")
        
        try:
            plt.show()
        except KeyboardInterrupt:
            print("\nShutting down...")
        finally:
            self.running = False
            if self.csv_file:
                self.csv_file.close()
                print(f"✓ Data saved to: {self.csv_filename}")
            if self.ser:
                self.ser.close()

def main():
    """Main entry point"""
    # Determine port (modify for your system)
    port = None
    
    # Try common ports
    import serial.tools.list_ports
    ports = [p.device for p in serial.tools.list_ports.comports()]
    
    if len(ports) == 0:
        print("No serial ports found!")
        sys.exit(1)
    elif len(ports) == 1:
        port = ports[0]
        print(f"Auto-detected port: {port}")
    else:
        print("Available ports:")
        for i, p in enumerate(ports):
            print(f"  {i}: {p}")
        port_idx = int(input("Select port number: "))
        port = ports[port_idx]
    
    # Create and run monitor
    monitor = MotorPIDMonitor(port=port)
    monitor.run()

if __name__ == "__main__":
    main()
