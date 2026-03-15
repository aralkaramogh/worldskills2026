#!/usr/bin/env python3
"""
PID Motor Control Monitor - NO TKINTER REQUIRED
Works without any GUI toolkit installed
Plots data in real-time (may use default system backend)

Install: pip install pyserial matplotlib
Usage: python pid_monitor_simple.py COM10
"""

import sys
import time
import warnings
from collections import deque

# Suppress warnings
warnings.filterwarnings('ignore')

try:
    import serial
except ImportError:
    print("❌ Missing: pyserial")
    print("   Install: pip install pyserial")
    sys.exit(1)

try:
    import matplotlib.pyplot as plt
    import matplotlib.animation as animation
except ImportError:
    print("❌ Missing: matplotlib")
    print("   Install: pip install matplotlib")
    sys.exit(1)

# Configuration
BUFFER_SIZE = 500
UPDATE_INTERVAL = 100
BAUD_RATE = 115200

class SimpleMonitor:
    def __init__(self, port):
        """Initialize without strict backend requirements"""
        self.port = port
        self.ser = None
        self.ani = None
        
        # Data buffers
        self.times = deque(maxlen=BUFFER_SIZE)
        self.setpoints = deque(maxlen=BUFFER_SIZE)
        self.actuals = deque(maxlen=BUFFER_SIZE)
        self.errors = deque(maxlen=BUFFER_SIZE)
        self.pwms = deque(maxlen=BUFFER_SIZE)
        
        # Connect
        try:
            self.ser = serial.Serial(port, BAUD_RATE, timeout=1)
            print(f"✓ Connected to {port} at {BAUD_RATE} baud")
            time.sleep(1)
        except Exception as e:
            print(f"✗ Cannot connect to {port}")
            print(f"  Error: {e}")
            print(f"\n  Check:")
            print(f"    - Port is correct")
            print(f"    - No other app is using this port")
            print(f"    - USB cable is connected")
            sys.exit(1)
        
        # Setup plot
        self.setup_plot()
    
    def setup_plot(self):
        """Create simple figure"""
        self.fig, self.axes = plt.subplots(2, 1, figsize=(12, 8))
        self.fig.suptitle('PID Motor Monitor (No Tkinter)', fontsize=12, fontweight='bold')
        
        # Plot 1: Speed
        ax1 = self.axes[0]
        self.line_set, = ax1.plot([], [], 'b-', label='Target', linewidth=2)
        self.line_act, = ax1.plot([], [], 'r-', label='Actual', linewidth=2)
        ax1.set_ylabel('RPM', fontsize=10)
        ax1.set_title('Motor Speed', fontsize=11)
        ax1.legend(loc='upper left')
        ax1.grid(True, alpha=0.3)
        ax1.set_ylim(-50, 350)
        self.ax1 = ax1
        
        # Plot 2: Error
        ax2 = self.axes[1]
        self.line_err, = ax2.plot([], [], 'g-', label='Error', linewidth=2)
        self.line_pwm, = ax2.plot([], [], 'orange', label='PWM %', linewidth=2, linestyle='--')
        ax2.set_xlabel('Time (s)', fontsize=10)
        ax2.set_ylabel('Error (RPM)', fontsize=10)
        ax2.set_title('Error & PWM', fontsize=11)
        ax2.legend(loc='upper left')
        ax2.grid(True, alpha=0.3)
        self.ax2 = ax2
        
        plt.tight_layout()
    
    def parse_line(self, line):
        """Parse CSV"""
        try:
            if any(x in line for x in ['Time', '====', 'Config', 'Motor', 'Waveform']):
                return None
            
            parts = line.strip().split(',')
            if len(parts) < 5:
                return None
            
            return {
                'time': float(parts[0]),
                'setpoint': float(parts[1]),
                'actual': float(parts[2]),
                'error': float(parts[3]),
                'pwm': (float(parts[4]) / 255.0) * 100,
            }
        except:
            return None
    
    def update(self, frame):
        """Update plots"""
        try:
            if self.ser.in_waiting:
                line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                data = self.parse_line(line)
                
                if data:
                    self.times.append(data['time'])
                    self.setpoints.append(data['setpoint'])
                    self.actuals.append(data['actual'])
                    self.errors.append(data['error'])
                    self.pwms.append(data['pwm'])
            
            if len(self.times) > 0:
                t = list(self.times)
                
                self.line_set.set_data(t, list(self.setpoints))
                self.line_act.set_data(t, list(self.actuals))
                self.ax1.set_xlim(0, max(t) + 1)
                
                self.line_err.set_data(t, list(self.errors))
                self.line_pwm.set_data(t, list(self.pwms))
                self.ax2.set_xlim(0, max(t) + 1)
                
                # Auto scale
                if len(self.actuals) > 5:
                    y_min = min(list(self.actuals))
                    y_max = max(list(self.actuals))
                    margin = (y_max - y_min) * 0.2 if y_max > y_min else 50
                    self.ax1.set_ylim(y_min - margin, y_max + margin)
        
        except:
            pass
        
        return self.line_set, self.line_act, self.line_err, self.line_pwm
    
    def run(self):
        """Start monitoring"""
        self.ani = animation.FuncAnimation(
            self.fig, self.update,
            interval=UPDATE_INTERVAL,
            blit=False,
            cache_frame_data=False,
            repeat=True
        )
        
        print("\n" + "="*60)
        print("📊 SIMPLE MONITOR STARTED (NO TKINTER)")
        print("="*60)
        print("\nSend commands:")
        print("  SQUARE     - Square wave test")
        print("  SINE       - Sine wave test")
        print("  kP:0.03    - Set P gain")
        print("  STOP       - Stop motor")
        print("\nClose graph window to exit")
        print("="*60 + "\n")
        
        try:
            plt.show()
        except Exception as e:
            print(f"Graph error: {e}")
        finally:
            self.cleanup()
    
    def cleanup(self):
        """Cleanup"""
        try:
            if self.ani:
                self.ani.event_source.stop()
        except:
            pass
        
        try:
            plt.close('all')
        except:
            pass
        
        if self.ser and self.ser.is_open:
            try:
                self.ser.write(b'STOP\n')
                time.sleep(0.2)
                self.ser.close()
                print("✓ Closed")
            except:
                pass


def main():
    if len(sys.argv) < 2:
        print("\n❌ Usage: python pid_monitor_simple.py <PORT>")
        print("\nExamples:")
        print("  Windows:   python pid_monitor_simple.py COM10")
        print("  Linux:     python pid_monitor_simple.py /dev/ttyUSB0")
        print("\nFind your port:")
        print("  Windows: Device Manager → Ports")
        print("  Linux:   ls /dev/ttyUSB*")
        sys.exit(1)
    
    monitor = SimpleMonitor(sys.argv[1])
    monitor.run()


if __name__ == '__main__':
    main()
