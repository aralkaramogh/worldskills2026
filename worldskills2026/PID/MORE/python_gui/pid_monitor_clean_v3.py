#!/usr/bin/env python3
"""
PID Motor Control Real-Time Monitor (Clean Version)
Plots encoder test, P-tuning, and full PID data in real-time
Requires: pyserial, matplotlib, pandas, numpy

Install: pip install pyserial matplotlib pandas numpy
Usage: python pid_monitor_clean.py COM3
       or: python pid_monitor_clean.py /dev/ttyUSB0
"""

import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import sys
import time
import warnings

# Suppress matplotlib warnings
warnings.filterwarnings('ignore', category=UserWarning, module='matplotlib')

# Configuration
BUFFER_SIZE = 500  # Number of points to display
UPDATE_INTERVAL = 100  # Update plot every 100ms
BAUD_RATE = 115200

class PIDMonitor:
    def __init__(self, port):
        """Initialize serial connection and plotting"""
        self.port = port
        self.baud_rate = BAUD_RATE
        
        # Data buffers
        self.times = deque(maxlen=BUFFER_SIZE)
        self.setpoints = deque(maxlen=BUFFER_SIZE)
        self.actuals = deque(maxlen=BUFFER_SIZE)
        self.errors = deque(maxlen=BUFFER_SIZE)
        self.pwms = deque(maxlen=BUFFER_SIZE)
        self.integrals = deque(maxlen=BUFFER_SIZE)
        
        self.ani = None  # Will be set in run()
        self.ser = None
        
        # Try to connect
        try:
            self.ser = serial.Serial(port, self.baud_rate, timeout=1)
            print(f"✓ Connected to {port} at {self.baud_rate} baud")
            time.sleep(1)  # Wait for board to initialize
        except Exception as e:
            print(f"✗ Failed to connect: {e}")
            print(f"  Check:")
            print(f"    - Port is correct (try Device Manager or 'ls /dev/tty*')")
            print(f"    - No other serial monitor is open")
            print(f"    - USB cable is connected")
            sys.exit(1)
        
        # Setup plot
        self.setup_plot()
    
    def setup_plot(self):
        """Create matplotlib figure with subplots"""
        self.fig, self.axes = plt.subplots(3, 1, figsize=(12, 10))
        self.fig.suptitle('PID Motor Control Monitor', fontsize=14, fontweight='bold')
        
        # Subplot 1: RPM Tracking
        self.ax1 = self.axes[0]
        self.line_setpoint, = self.ax1.plot([], [], 'b-', label='Setpoint', linewidth=2)
        self.line_actual, = self.ax1.plot([], [], 'r-', label='Actual', linewidth=2)
        self.ax1.set_ylabel('RPM', fontsize=10)
        self.ax1.set_title('Speed Response', fontsize=11)
        self.ax1.legend(loc='upper left')
        self.ax1.grid(True, alpha=0.3)
        self.ax1.set_ylim(-50, 350)
        
        # Subplot 2: Error & PWM
        self.ax2 = self.axes[1]
        self.line_error, = self.ax2.plot([], [], 'g-', label='Error (RPM)', linewidth=2)
        self.ax2_pwm = self.ax2.twinx()
        self.line_pwm, = self.ax2_pwm.plot([], [], 'orange', label='PWM (%)', linewidth=2, linestyle='--')
        self.ax2.set_ylabel('Error (RPM)', fontsize=10, color='g')
        self.ax2_pwm.set_ylabel('PWM (%)', fontsize=10, color='orange')
        self.ax2.set_title('Error & Control Signal', fontsize=11)
        self.ax2.grid(True, alpha=0.3)
        self.ax2.tick_params(axis='y', labelcolor='g')
        self.ax2_pwm.tick_params(axis='y', labelcolor='orange')
        
        # Subplot 3: Integral Term
        self.ax3 = self.axes[2]
        self.line_integral, = self.ax3.plot([], [], 'purple', label='Integral', linewidth=2)
        self.ax3.set_xlabel('Time (seconds)', fontsize=10)
        self.ax3.set_ylabel('Integral', fontsize=10)
        self.ax3.set_title('Integral Term (I-gain effect)', fontsize=11)
        self.ax3.legend(loc='upper left')
        self.ax3.grid(True, alpha=0.3)
        
        # Adjust layout
        plt.tight_layout()
    
    def parse_line(self, line):
        """Parse CSV data from serial output"""
        try:
            # Skip header and non-data lines
            if any(skip in line for skip in ['Time', '====', 'RPM', 'Configuration', 'Motor', 'Counts', '---', 'Direction', 'kP:', 'kI:', 'kD:', 'Waveform']):
                return None
            
            # Parse CSV format
            parts = line.strip().split(',')
            if len(parts) < 5:
                return None
            
            try:
                time_val = float(parts[0])
                setpoint = float(parts[1])
                actual = float(parts[2])
                error = float(parts[3])
                pwm = float(parts[4])
                integral = float(parts[5]) if len(parts) > 5 else 0.0
                
                return {
                    'time': time_val,
                    'setpoint': setpoint,
                    'actual': actual,
                    'error': error,
                    'pwm': pwm / 255.0 * 100,  # Convert to percentage
                    'integral': integral
                }
            except (ValueError, IndexError):
                return None
        except Exception as e:
            return None
    
    def update_plots(self, frame):
        """Update plots with new data from serial"""
        try:
            # Read available data
            if self.ser and self.ser.in_waiting:
                line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                
                data = self.parse_line(line)
                if data:
                    self.times.append(data['time'])
                    self.setpoints.append(data['setpoint'])
                    self.actuals.append(data['actual'])
                    self.errors.append(data['error'])
                    self.pwms.append(data['pwm'])
                    self.integrals.append(data['integral'])
            
            # Update line plots
            if len(self.times) > 0:
                times_list = list(self.times)
                
                # Plot 1: Speed tracking
                self.line_setpoint.set_data(times_list, list(self.setpoints))
                self.line_actual.set_data(times_list, list(self.actuals))
                if times_list:
                    self.ax1.set_xlim(0, max(times_list) + 1)
                
                # Plot 2: Error and PWM
                self.line_error.set_data(times_list, list(self.errors))
                self.line_pwm.set_data(times_list, list(self.pwms))
                if times_list:
                    self.ax2.set_xlim(0, max(times_list) + 1)
                self.ax2_pwm.set_ylim(0, 110)
                
                # Plot 3: Integral
                self.line_integral.set_data(times_list, list(self.integrals))
                if times_list:
                    self.ax3.set_xlim(0, max(times_list) + 1)
                
                # Auto-scale Y axes
                if len(self.actuals) > 5:
                    actual_vals = list(self.actuals)
                    actual_min = min(actual_vals)
                    actual_max = max(actual_vals)
                    margin = (actual_max - actual_min) * 0.2 if actual_max > actual_min else 50
                    self.ax1.set_ylim(actual_min - margin, actual_max + margin)
                
                if len(self.errors) > 5:
                    error_vals = list(self.errors)
                    error_max = max(abs(e) for e in error_vals) * 1.2
                    self.ax2.set_ylim(-error_max, error_max)
        
        except Exception as e:
            pass  # Silently ignore parse errors
        
        return self.line_setpoint, self.line_actual, self.line_error, self.line_pwm, self.line_integral
    
    def run(self):
        """Start the animation loop"""
        # Create animation and store as instance variable
        self.ani = animation.FuncAnimation(
            self.fig,
            self.update_plots,
            interval=UPDATE_INTERVAL,
            blit=False,
            cache_frame_data=False,
            repeat=True
        )
        
        print("\n" + "="*60)
        print("📊 PID MONITOR STARTED")
        print("="*60)
        print("\nSend commands from serial terminal:")
        print("  SQUARE          - Square wave test")
        print("  SINE            - Sine wave test")
        print("  RAMP            - Ramp wave test")
        print("  kP:0.03         - Set P gain")
        print("  kD:0.005        - Set D gain")
        print("  kI:0.001        - Set I gain")
        print("  STOP            - Stop motor")
        print("  HELP            - Show all commands")
        print("\nClose the plot window to exit.")
        print("="*60 + "\n")
        
        try:
            plt.show()
        except KeyboardInterrupt:
            print("\n\nStopped by user")
        finally:
            self.cleanup()
    
    def cleanup(self):
        """Cleanup before exit"""
        # Stop animation gracefully
        if self.ani is not None:
            try:
                self.ani.event_source.stop()
            except:
                pass
        
        # Close plot
        try:
            plt.close(self.fig)
        except:
            pass
        
        # Send stop command and close serial
        if self.ser and self.ser.is_open:
            try:
                self.ser.write(b'STOP\n')
                time.sleep(0.2)
                self.ser.close()
                print("✓ Motor stopped and connection closed")
            except:
                pass


def main():
    """Main entry point"""
    if len(sys.argv) < 2:
        print("\n❌ Usage: python pid_monitor_clean.py <PORT>")
        print("\nExamples:")
        print("  Windows:  python pid_monitor_clean.py COM10")
        print("  Linux:    python pid_monitor_clean.py /dev/ttyUSB0")
        print("  Mac:      python pid_monitor_clean.py /dev/cu.usbserial-xxxxx")
        print("\nFind your port:")
        print("  Windows: Device Manager → Ports (COM & LPT)")
        print("  Linux:   ls /dev/ttyUSB*")
        print("  Mac:     ls /dev/cu.*")
        print()
        sys.exit(1)
    
    port = sys.argv[1]
    monitor = PIDMonitor(port)
    monitor.run()


if __name__ == '__main__':
    main()
