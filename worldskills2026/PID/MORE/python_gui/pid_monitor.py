#!/usr/bin/env python3
"""
Real-time PID Motor Control Monitor and Tuner
Reads CSV data from Arduino serial port and plots in real-time

Requirements:
    pip install pyserial matplotlib pandas numpy
"""

import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import numpy as np
from datetime import datetime
import sys

class PIDMonitor:
    def __init__(self, port='COM3', baudrate=115200, max_points=500):
        """Initialize serial connection and plot"""
        self.port = port
        self.baudrate = baudrate
        self.max_points = max_points
        self.ser = None
        
        # Data storage (use deque for efficient FIFO)
        self.times = deque(maxlen=max_points)
        self.setpoints = deque(maxlen=max_points)
        self.actuals = deque(maxlen=max_points)
        self.errors = deque(maxlen=max_points)
        self.pwms = deque(maxlen=max_points)
        self.integrals = deque(maxlen=max_points)
        
        self.tuning_params = {'kP': 0, 'kI': 0, 'kD': 0}
        
        self.setup_plots()
        self.connect_serial()
        
    def connect_serial(self):
        """Connect to Arduino serial port"""
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            print(f"Connected to {self.port} at {self.baudrate} baud")
            
            # Wait for Arduino to initialize
            import time
            time.sleep(2)
            
            # Clear buffer
            while self.ser.in_waiting:
                self.ser.readline()
                
            print("Type 'HELP' on Arduino to see commands")
            print("Type 'SQUARE', 'SINE', or 'RAMP' to start test")
            
        except Exception as e:
            print(f"Error connecting to {self.port}: {e}")
            sys.exit(1)
            
    def setup_plots(self):
        """Setup matplotlib figure with subplots"""
        self.fig, self.axes = plt.subplots(2, 2, figsize=(14, 9))
        self.fig.suptitle('Real-Time PID Motor Control Monitor', fontsize=14, fontweight='bold')
        
        # Plot 1: Speed response (setpoint vs actual)
        self.ax1 = self.axes[0, 0]
        self.line1_set, = self.ax1.plot([], [], 'r-', linewidth=2, label='Setpoint')
        self.line1_act, = self.ax1.plot([], [], 'b-', linewidth=2, label='Actual')
        self.ax1.set_ylabel('Speed (RPM)', fontsize=10)
        self.ax1.set_title('Motor Speed Response', fontsize=11, fontweight='bold')
        self.ax1.legend(loc='upper left')
        self.ax1.grid(True, alpha=0.3)
        self.ax1.set_ylim(-500, 5000)
        
        # Plot 2: Error
        self.ax2 = self.axes[0, 1]
        self.line2, = self.ax2.plot([], [], 'g-', linewidth=2)
        self.ax2.set_ylabel('Error (RPM)', fontsize=10)
        self.ax2.set_title('Tracking Error', fontsize=11, fontweight='bold')
        self.ax2.grid(True, alpha=0.3)
        self.ax2.axhline(y=0, color='k', linestyle='--', alpha=0.3)
        self.ax2.set_ylim(-2500, 2500)
        
        # Plot 3: PWM and Integral
        self.ax3 = self.axes[1, 0]
        self.line3_pwm, = self.ax3.plot([], [], 'purple', linewidth=2, label='PWM')
        self.ax3_twin = self.ax3.twinx()
        self.line3_int, = self.ax3_twin.plot([], [], 'orange', linewidth=1.5, alpha=0.7, label='Integral')
        self.ax3.set_xlabel('Time (s)', fontsize=10)
        self.ax3.set_ylabel('PWM (0-255)', fontsize=10, color='purple')
        self.ax3_twin.set_ylabel('Integral Term', fontsize=10, color='orange')
        self.ax3.set_title('Motor Command & Integral Term', fontsize=11, fontweight='bold')
        self.ax3.grid(True, alpha=0.3)
        self.ax3.set_ylim(0, 260)
        
        # Plot 4: Performance metrics (text box)
        self.ax4 = self.axes[1, 1]
        self.ax4.axis('off')
        self.metrics_text = self.ax4.text(0.05, 0.95, '', transform=self.ax4.transAxes,
                                         fontsize=10, verticalalignment='top', family='monospace',
                                         bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
        
        self.fig.tight_layout()
        
    def parse_data(self, line):
        """Parse CSV line from Arduino"""
        try:
            parts = [float(x.strip()) for x in line.split(',')]
            if len(parts) >= 9:
                return {
                    'time': parts[0],
                    'setpoint': parts[1],
                    'actual': parts[2],
                    'error': parts[3],
                    'pwm': parts[4],
                    'integral': parts[5],
                    'kP': parts[6],
                    'kI': parts[7],
                    'kD': parts[8]
                }
        except (ValueError, IndexError):
            pass
        return None
        
    def read_serial_data(self):
        """Read and parse incoming serial data"""
        while self.ser.in_waiting:
            try:
                line = self.ser.readline().decode('utf-8').strip()
                if line and line[0].isdigit():  # CSV data starts with number
                    data = self.parse_data(line)
                    if data:
                        self.times.append(data['time'])
                        self.setpoints.append(data['setpoint'])
                        self.actuals.append(data['actual'])
                        self.errors.append(data['error'])
                        self.pwms.append(data['pwm'])
                        self.integrals.append(data['integral'])
                        self.tuning_params = {'kP': data['kP'], 'kI': data['kI'], 'kD': data['kD']}
            except Exception as e:
                pass
                
    def calculate_metrics(self):
        """Calculate performance metrics from data"""
        if len(self.actuals) < 10:
            return {}
        
        metrics = {}
        
        # Convert deques to lists for analysis
        errors = list(self.errors)
        actuals = list(self.actuals)
        setpoints = list(self.setpoints)
        
        # Steady-state error (last 10% of data)
        last_n = max(5, int(len(errors) * 0.1))
        metrics['SS_Error'] = np.mean(errors[-last_n:])
        
        # Max error
        metrics['Max_Error'] = np.max(np.abs(errors))
        
        # RMS error
        metrics['RMS_Error'] = np.sqrt(np.mean(np.array(errors)**2))
        
        # Overshoot (if setpoint is constant)
        if len(setpoints) > 10:
            recent_set = np.mean(setpoints[-10:])
            recent_act = np.max(actuals[-10:])
            overshoot = ((recent_act - recent_set) / max(abs(recent_set), 1)) * 100
            metrics['Overshoot'] = max(0, overshoot)  # Only positive overshoot
        
        # Rise time (simplistic: time to reach 90% of setpoint)
        # This is complex, so we'll skip for now
        
        return metrics
        
    def update_plots(self, frame):
        """Update plot data (called by animation)"""
        self.read_serial_data()
        
        if len(self.times) > 0:
            # Update line data
            self.line1_set.set_data(list(self.times), list(self.setpoints))
            self.line1_act.set_data(list(self.times), list(self.actuals))
            self.line2.set_data(list(self.times), list(self.errors))
            self.line3_pwm.set_data(list(self.times), list(self.pwms))
            self.line3_int.set_data(list(self.times), list(self.integrals))
            
            # Auto-scale x-axis
            if self.times:
                max_time = max(self.times)
                self.ax1.set_xlim(max(0, max_time - 10), max_time + 0.5)
                self.ax2.set_xlim(max(0, max_time - 10), max_time + 0.5)
                self.ax3.set_xlim(max(0, max_time - 10), max_time + 0.5)
            
            # Calculate and display metrics
            metrics = self.calculate_metrics()
            metrics_str = f"""TUNING PARAMETERS
kP = {self.tuning_params['kP']:.4f}
kI = {self.tuning_params['kI']:.6f}
kD = {self.tuning_params['kD']:.4f}

PERFORMANCE METRICS
Data Points: {len(self.times)}
Time Elapsed: {max(self.times):.1f} s

Steady-State Error: {metrics.get('SS_Error', 0):.1f} RPM
Max Error: {metrics.get('Max_Error', 0):.1f} RPM
RMS Error: {metrics.get('RMS_Error', 0):.1f} RPM
Overshoot: {metrics.get('Overshoot', 0):.1f} %

Last RPM: {self.actuals[-1]:.1f} if len(self.actuals) > 0 else '---'
Last Setpoint: {self.setpoints[-1]:.1f} if len(self.setpoints) > 0 else '---'
"""
            self.metrics_text.set_text(metrics_str)
        
        return self.line1_set, self.line1_act, self.line2, self.line3_pwm, self.line3_int
        
    def run(self):
        """Start real-time monitoring"""
        ani = animation.FuncAnimation(self.fig, self.update_plots, interval=100,
                                     blit=False, cache_frame_data=False)
        
        # Create control instructions
        print("\n" + "="*60)
        print("REAL-TIME PID MONITOR STARTED")
        print("="*60)
        print("\nCommands to send to Arduino (via serial terminal):")
        print("  SQUARE            - Square wave test")
        print("  SINE              - Sine wave test")
        print("  RAMP              - Ramp wave test")
        print("  kP:0.05           - Set P gain")
        print("  kD:0.008          - Set D gain")
        print("  kI:0.001          - Set I gain")
        print("  STOP              - Stop motor")
        print("  INFO              - Show status")
        print("  HELP              - Show Arduino commands")
        print("\nClose the plot window to exit")
        print("="*60 + "\n")
        
        plt.show()
        self.cleanup()
        
    def cleanup(self):
        """Close serial connection and cleanup"""
        if self.ser and self.ser.is_open:
            print("\nStopping motor...")
            self.ser.write(b'STOP\n')
            import time
            time.sleep(0.5)
            self.ser.close()
            print("Connection closed")


def main():
    # Try to find the correct COM port
    port = 'COM3'  # Change this to your port (COM3, /dev/ttyUSB0, etc.)
    
    if len(sys.argv) > 1:
        port = sys.argv[1]
    
    print(f"Connecting to {port}...")
    monitor = PIDMonitor(port=port)
    monitor.run()


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("\nInterrupted by user")
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
