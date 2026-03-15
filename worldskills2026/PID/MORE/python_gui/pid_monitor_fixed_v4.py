#!/usr/bin/env python3
"""
PID Motor Control Real-Time Monitor (Fixed Backend Version)
Plots encoder test, P-tuning, and full PID data in real-time
Requires: pyserial, matplotlib, pandas, numpy

Install: pip install pyserial matplotlib pandas numpy
Usage: python pid_monitor_fixed.py COM3
       or: python pid_monitor_fixed.py /dev/ttyUSB0
"""

import sys
import time
import warnings

# CRITICAL: Set interactive backend BEFORE importing pyplot
import matplotlib
print("Setting up matplotlib backend...")
try:
    # Try TkAgg first (most reliable cross-platform)
    matplotlib.use('TkAgg')
except:
    try:
        # Fallback to Qt5Agg
        matplotlib.use('Qt5Agg')
    except:
        try:
            # Last resort
            matplotlib.use('Agg')
        except:
            pass

import matplotlib.pyplot as plt
import matplotlib.animation as animation
import serial
from collections import deque

# Suppress non-critical warnings
warnings.filterwarnings('ignore', category=UserWarning)
warnings.filterwarnings('ignore', category=DeprecationWarning)

# Configuration
BUFFER_SIZE = 500
UPDATE_INTERVAL = 100
BAUD_RATE = 115200

class PIDMonitor:
    def __init__(self, port):
        """Initialize serial connection and plotting"""
        self.port = port
        self.baud_rate = BAUD_RATE
        self.ani = None
        self.ser = None
        
        # Data buffers
        self.times = deque(maxlen=BUFFER_SIZE)
        self.setpoints = deque(maxlen=BUFFER_SIZE)
        self.actuals = deque(maxlen=BUFFER_SIZE)
        self.errors = deque(maxlen=BUFFER_SIZE)
        self.pwms = deque(maxlen=BUFFER_SIZE)
        self.integrals = deque(maxlen=BUFFER_SIZE)
        
        # Connect to serial
        try:
            self.ser = serial.Serial(port, self.baud_rate, timeout=1)
            print(f"✓ Connected to {port} at {self.baud_rate} baud")
            time.sleep(1)
        except Exception as e:
            print(f"✗ Failed to connect to {port}")
            print(f"  Error: {e}")
            print(f"\n  Troubleshooting:")
            print(f"    1. Close any other serial monitors (Arduino IDE, etc.)")
            print(f"    2. Check Device Manager (Windows) or 'ls /dev/tty*' (Linux)")
            print(f"    3. Try: python pid_monitor_fixed.py COM10  (adjust port)")
            sys.exit(1)
        
        # Setup plotting
        self.setup_plot()
    
    def setup_plot(self):
        """Create matplotlib figure with subplots"""
        self.fig = plt.figure(figsize=(14, 10))
        self.fig.suptitle('PID Motor Control Monitor', fontsize=14, fontweight='bold')
        
        # Subplot 1: RPM Tracking
        self.ax1 = self.fig.add_subplot(3, 1, 1)
        self.line_setpoint, = self.ax1.plot([], [], 'b-', label='Setpoint', linewidth=2)
        self.line_actual, = self.ax1.plot([], [], 'r-', label='Actual', linewidth=2)
        self.ax1.set_ylabel('RPM', fontsize=10, fontweight='bold')
        self.ax1.set_title('Motor Speed Response', fontsize=11, fontweight='bold')
        self.ax1.legend(loc='upper left', fontsize=10)
        self.ax1.grid(True, alpha=0.3, linestyle='--')
        self.ax1.set_ylim(-50, 350)
        
        # Subplot 2: Error & PWM
        self.ax2 = self.fig.add_subplot(3, 1, 2)
        self.line_error, = self.ax2.plot([], [], 'g-', label='Error (RPM)', linewidth=2)
        self.ax2.set_ylabel('Error (RPM)', fontsize=10, color='g', fontweight='bold')
        self.ax2.tick_params(axis='y', labelcolor='g')
        self.ax2.grid(True, alpha=0.3, linestyle='--')
        
        # Twin axis for PWM
        self.ax2_pwm = self.ax2.twinx()
        self.line_pwm, = self.ax2_pwm.plot([], [], 'orange', label='PWM (%)', 
                                           linewidth=2, linestyle='--')
        self.ax2_pwm.set_ylabel('PWM (%)', fontsize=10, color='orange', fontweight='bold')
        self.ax2_pwm.tick_params(axis='y', labelcolor='orange')
        self.ax2.set_title('Error & Control Signal', fontsize=11, fontweight='bold')
        
        # Combined legend
        lines1 = [self.line_error, self.line_pwm]
        labels1 = [l.get_label() for l in lines1]
        self.ax2.legend(lines1, labels1, loc='upper left', fontsize=10)
        
        # Subplot 3: Integral
        self.ax3 = self.fig.add_subplot(3, 1, 3)
        self.line_integral, = self.ax3.plot([], [], 'purple', label='Integral', linewidth=2)
        self.ax3.set_xlabel('Time (seconds)', fontsize=10, fontweight='bold')
        self.ax3.set_ylabel('Integral', fontsize=10, fontweight='bold')
        self.ax3.set_title('Integral Term (I-gain accumulation)', fontsize=11, fontweight='bold')
        self.ax3.legend(loc='upper left', fontsize=10)
        self.ax3.grid(True, alpha=0.3, linestyle='--')
        
        # Tight layout
        plt.tight_layout()
    
    def parse_line(self, line):
        """Parse CSV from serial"""
        try:
            # Skip non-data lines
            skip_keywords = ['Time', '====', 'Configuration', 'Motor', 'Counts', 
                           '---', 'Direction', 'Waveform', 'kP:', 'kI:', 'kD:']
            if any(kw in line for kw in skip_keywords):
                return None
            
            parts = line.strip().split(',')
            if len(parts) < 5:
                return None
            
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
                'pwm': (pwm / 255.0) * 100,  # Convert to %
                'integral': integral
            }
        except:
            return None
    
    def update_plots(self, frame):
        """Update plots with incoming serial data"""
        try:
            # Read serial data
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
            
            # Update plots if we have data
            if len(self.times) > 0:
                times_list = list(self.times)
                
                # Plot 1: Speed
                self.line_setpoint.set_data(times_list, list(self.setpoints))
                self.line_actual.set_data(times_list, list(self.actuals))
                self.ax1.set_xlim(0, max(times_list) + 1)
                
                # Auto-scale Y for plot 1
                if len(self.actuals) > 5:
                    actual_vals = list(self.actuals)
                    y_min = min(actual_vals)
                    y_max = max(actual_vals)
                    margin = (y_max - y_min) * 0.2 if y_max > y_min else 50
                    self.ax1.set_ylim(y_min - margin, y_max + margin)
                
                # Plot 2: Error & PWM
                self.line_error.set_data(times_list, list(self.errors))
                self.line_pwm.set_data(times_list, list(self.pwms))
                self.ax2.set_xlim(0, max(times_list) + 1)
                self.ax2_pwm.set_ylim(0, 110)
                
                # Auto-scale Y for error
                if len(self.errors) > 5:
                    error_vals = list(self.errors)
                    err_max = max(abs(e) for e in error_vals) * 1.2
                    self.ax2.set_ylim(-err_max, err_max)
                
                # Plot 3: Integral
                self.line_integral.set_data(times_list, list(self.integrals))
                self.ax3.set_xlim(0, max(times_list) + 1)
        
        except Exception as e:
            pass  # Silently ignore errors
        
        return self.line_setpoint, self.line_actual, self.line_error, self.line_pwm, self.line_integral
    
    def run(self):
        """Start monitoring"""
        # Create animation
        self.ani = animation.FuncAnimation(
            self.fig,
            self.update_plots,
            interval=UPDATE_INTERVAL,
            blit=False,
            cache_frame_data=False,
            repeat=True
        )
        
        # Print instructions
        print("\n" + "="*70)
        print("  📊 PID MONITOR RUNNING")
        print("="*70)
        print("\n  Send these commands from your serial terminal:\n")
        print("    SQUARE          Test square wave (0 → 300 → 0 RPM)")
        print("    SINE            Test sine wave (smooth 0-300 RPM)")
        print("    RAMP            Test ramp response")
        print("    kP:0.03         Set proportional gain")
        print("    kD:0.005        Set derivative gain")
        print("    kI:0.001        Set integral gain")
        print("    STOP            Stop motor")
        print("    HELP            Show all commands")
        print("\n  Graph window:")
        print("    - Blue line   = Target RPM")
        print("    - Red line    = Actual RPM")
        print("    - Green line  = Error (difference)")
        print("    - Orange line = PWM signal")
        print("\n  Close the graph window to exit.")
        print("="*70 + "\n")
        
        # Show plot
        try:
            plt.show()
        except Exception as e:
            print(f"\nPlot closed or error: {e}")
        finally:
            self.cleanup()
    
    def cleanup(self):
        """Cleanup on exit"""
        print("\nCleaning up...")
        
        # Stop animation
        if self.ani:
            try:
                self.ani.event_source.stop()
            except:
                pass
        
        # Close figure
        try:
            plt.close('all')
        except:
            pass
        
        # Stop motor and close serial
        if self.ser and self.ser.is_open:
            try:
                self.ser.write(b'STOP\n')
                time.sleep(0.3)
                self.ser.close()
                print("✓ Motor stopped")
                print("✓ Serial connection closed")
            except:
                pass
        
        print("✓ Monitor closed")


def main():
    """Entry point"""
    if len(sys.argv) < 2:
        print("\n" + "="*70)
        print("  PID MONITOR - Real-time Motor Control Visualization")
        print("="*70)
        print("\n  USAGE:")
        print("    python pid_monitor_fixed.py <PORT>\n")
        print("  EXAMPLES:")
        print("    Windows:   python pid_monitor_fixed.py COM10")
        print("    Linux:     python pid_monitor_fixed.py /dev/ttyUSB0")
        print("    Mac:       python pid_monitor_fixed.py /dev/cu.usbserial-xxxxx\n")
        print("  FIND YOUR PORT:")
        print("    Windows:   Device Manager → Ports (COM & LPT)")
        print("    Linux:     ls /dev/ttyUSB*  or  dmesg | grep tty")
        print("    Mac:       ls /dev/cu.*\n")
        print("="*70 + "\n")
        sys.exit(1)
    
    port = sys.argv[1]
    monitor = PIDMonitor(port)
    monitor.run()


if __name__ == '__main__':
    main()
