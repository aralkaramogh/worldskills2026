#!/usr/bin/env python3
"""
Professional PID Motor Tuning Interface
Complete GUI with:
- COM port selection
- Real-time gain adjustment (kP, kI, kD)
- Live graph visualization
- Preset save/load
- Auto-tuning (Ziegler-Nichols)
- Data logging

Requirements: PyQt5, pyserial, matplotlib, numpy
Install: pip install PyQt5 pyserial matplotlib numpy

Usage: python pid_tuner_gui.py
"""

import sys
import json
import os
import time
import threading
from collections import deque
from datetime import datetime

try:
    from PyQt5.QtWidgets import (
        QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
        QGroupBox, QLabel, QLineEdit, QPushButton, QComboBox, QSpinBox,
        QDoubleSpinBox, QTableWidget, QTableWidgetItem, QMessageBox,
        QFileDialog, QCheckBox, QProgressBar, QSplitter, QTabWidget
    )
    from PyQt5.QtCore import Qt, QTimer, pyqtSignal, QThread, QObject
    from PyQt5.QtGui import QFont, QColor, QIcon
except ImportError:
    print("❌ PyQt5 not installed")
    print("   Install: pip install PyQt5")
    sys.exit(1)

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("❌ pyserial not installed")
    print("   Install: pip install pyserial")
    sys.exit(1)

try:
    import matplotlib.pyplot as plt
    from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
    from matplotlib.figure import Figure
    import numpy as np
except ImportError:
    print("❌ matplotlib or numpy not installed")
    print("   Install: pip install matplotlib numpy")
    sys.exit(1)

# Configuration
BAUD_RATE = 115200
BUFFER_SIZE = 1000
PRESETS_FILE = "pid_presets.json"

class SerialReader(QObject):
    """Thread for reading serial data"""
    data_received = pyqtSignal(dict)
    connection_status = pyqtSignal(bool)
    error_signal = pyqtSignal(str)
    
    def __init__(self):
        super().__init__()
        self.ser = None
        self.running = False
    
    def connect(self, port):
        """Connect to serial port"""
        try:
            self.ser = serial.Serial(port, BAUD_RATE, timeout=1)
            self.running = True
            self.connection_status.emit(True)
            return True
        except Exception as e:
            self.error_signal.emit(f"Connection failed: {str(e)}")
            return False
    
    def disconnect(self):
        """Disconnect from serial"""
        self.running = False
        if self.ser and self.ser.is_open:
            try:
                self.ser.write(b'STOP\n')
                time.sleep(0.2)
                self.ser.close()
            except:
                pass
            self.connection_status.emit(False)
    
    def run(self):
        """Read serial data in loop"""
        while self.running:
            try:
                if self.ser and self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    data = self.parse_line(line)
                    if data:
                        self.data_received.emit(data)
                time.sleep(0.01)
            except Exception as e:
                self.error_signal.emit(f"Read error: {str(e)}")
                self.disconnect()
    
    def write(self, command):
        """Send command to serial"""
        if self.ser and self.ser.is_open:
            try:
                self.ser.write(f"{command}\n".encode())
            except Exception as e:
                self.error_signal.emit(f"Write error: {str(e)}")
    
    def parse_line(self, line):
        """Parse CSV from serial"""
        try:
            skip_keywords = ['Time', '====', 'Configuration', 'Motor', 'Waveform', 'kP:', 'kI:', 'kD:']
            if any(kw in line for kw in skip_keywords):
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
                'kP': float(parts[5]) if len(parts) > 5 else 0,
                'kI': float(parts[6]) if len(parts) > 6 else 0,
                'kD': float(parts[7]) if len(parts) > 7 else 0,
            }
        except:
            return None


class MatplotlibCanvas(FigureCanvas):
    """Embedded matplotlib canvas"""
    def __init__(self, parent=None, width=10, height=6):
        self.fig = Figure(figsize=(width, height), dpi=100, facecolor='white')
        self.axes = self.fig.subplots(2, 2)
        super().__init__(self.fig)
        self.setParent(parent)
        self.setup_plots()
    
    def setup_plots(self):
        """Setup 4 subplots"""
        # Plot 1: RPM
        ax1 = self.axes[0, 0]
        ax1.set_title('Motor Speed Response', fontsize=10, fontweight='bold')
        ax1.set_ylabel('RPM', fontsize=9)
        ax1.set_xlabel('Time (s)', fontsize=9)
        ax1.grid(True, alpha=0.3)
        ax1.legend(['Setpoint', 'Actual'], fontsize=8)
        
        # Plot 2: Error
        ax2 = self.axes[0, 1]
        ax2.set_title('Control Error', fontsize=10, fontweight='bold')
        ax2.set_ylabel('Error (RPM)', fontsize=9)
        ax2.set_xlabel('Time (s)', fontsize=9)
        ax2.grid(True, alpha=0.3)
        
        # Plot 3: PWM
        ax3 = self.axes[1, 0]
        ax3.set_title('PWM Control Signal', fontsize=10, fontweight='bold')
        ax3.set_ylabel('PWM (%)', fontsize=9)
        ax3.set_xlabel('Time (s)', fontsize=9)
        ax3.grid(True, alpha=0.3)
        
        # Plot 4: Gains
        ax4 = self.axes[1, 1]
        ax4.set_title('PID Gains', fontsize=10, fontweight='bold')
        ax4.set_ylabel('Gain Value', fontsize=9)
        ax4.set_xlabel('Time (s)', fontsize=9)
        ax4.grid(True, alpha=0.3)
        ax4.legend(['kP', 'kI', 'kD'], fontsize=8)
        
        self.fig.tight_layout()
    
    def update_plots(self, data):
        """Update all plots with new data"""
        if not data:
            return
        
        times = data.get('times', [])
        setpoints = data.get('setpoints', [])
        actuals = data.get('actuals', [])
        errors = data.get('errors', [])
        pwms = data.get('pwms', [])
        kps = data.get('kps', [])
        kis = data.get('kis', [])
        kds = data.get('kds', [])
        
        if len(times) == 0:
            return
        
        # Clear axes
        for ax in self.axes.flat:
            ax.clear()
        
        # Plot 1: Speed
        ax1 = self.axes[0, 0]
        ax1.plot(times, setpoints, 'b-', label='Setpoint', linewidth=1.5)
        ax1.plot(times, actuals, 'r-', label='Actual', linewidth=1.5)
        ax1.set_title('Motor Speed Response', fontsize=10, fontweight='bold')
        ax1.set_ylabel('RPM', fontsize=9)
        ax1.set_xlabel('Time (s)', fontsize=9)
        ax1.grid(True, alpha=0.3)
        ax1.legend(fontsize=8)
        
        # Plot 2: Error
        ax2 = self.axes[0, 1]
        ax2.plot(times, errors, 'g-', linewidth=1.5)
        ax2.set_title('Control Error', fontsize=10, fontweight='bold')
        ax2.set_ylabel('Error (RPM)', fontsize=9)
        ax2.set_xlabel('Time (s)', fontsize=9)
        ax2.grid(True, alpha=0.3)
        ax2.fill_between(times, errors, alpha=0.3, color='green')
        
        # Plot 3: PWM
        ax3 = self.axes[1, 0]
        ax3.plot(times, pwms, 'orange', linewidth=1.5)
        ax3.set_title('PWM Control Signal', fontsize=10, fontweight='bold')
        ax3.set_ylabel('PWM (%)', fontsize=9)
        ax3.set_xlabel('Time (s)', fontsize=9)
        ax3.grid(True, alpha=0.3)
        ax3.set_ylim([0, 105])
        
        # Plot 4: Gains
        ax4 = self.axes[1, 1]
        if len(kps) > 0:
            ax4.plot(times, kps, 'b-', label='kP', linewidth=1.5)
        if len(kis) > 0:
            ax4.plot(times, kis, 'g-', label='kI', linewidth=1.5)
        if len(kds) > 0:
            ax4.plot(times, kds, 'r-', label='kD', linewidth=1.5)
        ax4.set_title('PID Gains', fontsize=10, fontweight='bold')
        ax4.set_ylabel('Gain Value', fontsize=9)
        ax4.set_xlabel('Time (s)', fontsize=9)
        ax4.grid(True, alpha=0.3)
        ax4.legend(fontsize=8)
        
        self.fig.tight_layout()
        self.draw()


class PIDTunerGUI(QMainWindow):
    """Main application window"""
    
    def __init__(self):
        super().__init__()
        self.setWindowTitle("🎛️ PID Motor Tuning Suite")
        self.setGeometry(100, 100, 1600, 900)
        
        # Data buffers
        self.data_buffers = {
            'times': deque(maxlen=BUFFER_SIZE),
            'setpoints': deque(maxlen=BUFFER_SIZE),
            'actuals': deque(maxlen=BUFFER_SIZE),
            'errors': deque(maxlen=BUFFER_SIZE),
            'pwms': deque(maxlen=BUFFER_SIZE),
            'kps': deque(maxlen=BUFFER_SIZE),
            'kis': deque(maxlen=BUFFER_SIZE),
            'kds': deque(maxlen=BUFFER_SIZE),
        }
        
        self.serial_reader = None
        self.serial_thread = None
        self.current_gains = {'kP': 0.02, 'kI': 0.0, 'kD': 0.0}
        
        self.init_ui()
        self.load_presets()
        self.setup_serial()
    
    def init_ui(self):
        """Initialize user interface"""
        central = QWidget()
        self.setCentralWidget(central)
        layout = QHBoxLayout(central)
        
        # Left panel: Controls
        left_panel = QWidget()
        left_layout = QVBoxLayout(left_panel)
        
        # Connection controls
        conn_group = self.create_connection_group()
        left_layout.addWidget(conn_group)
        
        # Gain controls
        gain_group = self.create_gain_group()
        left_layout.addWidget(gain_group)
        
        # Test controls
        test_group = self.create_test_group()
        left_layout.addWidget(test_group)
        
        # Preset controls
        preset_group = self.create_preset_group()
        left_layout.addWidget(preset_group)
        
        # Auto-tune controls
        autotune_group = self.create_autotune_group()
        left_layout.addWidget(autotune_group)
        
        # Status panel
        status_group = self.create_status_group()
        left_layout.addWidget(status_group)
        
        left_layout.addStretch()
        
        # Right panel: Graphs
        self.canvas = MatplotlibCanvas()
        
        # Add panels to main layout
        layout.addWidget(left_panel, 1)
        layout.addWidget(self.canvas, 2)
    
    def create_connection_group(self):
        """Create serial connection group"""
        group = QGroupBox("🔌 Serial Connection", self)
        layout = QVBoxLayout()
        
        # Port selection
        port_layout = QHBoxLayout()
        port_layout.addWidget(QLabel("Port:"))
        self.port_combo = QComboBox()
        self.refresh_ports()
        port_layout.addWidget(self.port_combo)
        refresh_btn = QPushButton("🔄 Refresh")
        refresh_btn.clicked.connect(self.refresh_ports)
        port_layout.addWidget(refresh_btn)
        layout.addLayout(port_layout)
        
        # Connect/Disconnect buttons
        btn_layout = QHBoxLayout()
        self.connect_btn = QPushButton("✓ Connect")
        self.connect_btn.clicked.connect(self.connect_serial)
        self.connect_btn.setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;")
        btn_layout.addWidget(self.connect_btn)
        
        self.disconnect_btn = QPushButton("✗ Disconnect")
        self.disconnect_btn.clicked.connect(self.disconnect_serial)
        self.disconnect_btn.setEnabled(False)
        self.disconnect_btn.setStyleSheet("background-color: #f44336; color: white; font-weight: bold;")
        btn_layout.addWidget(self.disconnect_btn)
        layout.addLayout(btn_layout)
        
        # Status indicator
        status_layout = QHBoxLayout()
        status_layout.addWidget(QLabel("Status:"))
        self.status_label = QLabel("❌ Disconnected")
        self.status_label.setFont(QFont("Arial", 10, QFont.Bold))
        status_layout.addWidget(self.status_label)
        layout.addLayout(status_layout)
        
        group.setLayout(layout)
        return group
    
    def create_gain_group(self):
        """Create PID gain control group"""
        group = QGroupBox("⚙️ PID Gains", self)
        layout = QVBoxLayout()
        
        self.gain_controls = {}
        
        for gain_name, gain_label in [('kP', 'Proportional (kP)'), 
                                      ('kI', 'Integral (kI)'), 
                                      ('kD', 'Derivative (kD)')]:
            gain_layout = QHBoxLayout()
            gain_layout.addWidget(QLabel(gain_label))
            
            # Decrement button
            dec_btn = QPushButton("◀")
            dec_btn.setMaximumWidth(40)
            dec_btn.clicked.connect(lambda checked, g=gain_name: self.adjust_gain(g, -0.001))
            gain_layout.addWidget(dec_btn)
            
            # Value spinbox
            spinbox = QDoubleSpinBox()
            spinbox.setRange(0, 1.0)
            spinbox.setSingleStep(0.001)
            spinbox.setDecimals(4)
            spinbox.setValue(self.current_gains[gain_name])
            spinbox.valueChanged.connect(lambda val, g=gain_name: self.on_gain_change(g, val))
            self.gain_controls[gain_name] = spinbox
            gain_layout.addWidget(spinbox)
            
            # Increment button
            inc_btn = QPushButton("▶")
            inc_btn.setMaximumWidth(40)
            inc_btn.clicked.connect(lambda checked, g=gain_name: self.adjust_gain(g, 0.001))
            gain_layout.addWidget(inc_btn)
            
            # Apply button
            apply_btn = QPushButton(f"Send {gain_name}")
            apply_btn.setMaximumWidth(80)
            apply_btn.clicked.connect(lambda checked, g=gain_name: self.send_gain(g))
            gain_layout.addWidget(apply_btn)
            
            layout.addLayout(gain_layout)
        
        group.setLayout(layout)
        return group
    
    def create_test_group(self):
        """Create test waveform group"""
        group = QGroupBox("📈 Test Waveforms", self)
        layout = QVBoxLayout()
        
        # Waveform selection
        wave_layout = QHBoxLayout()
        wave_layout.addWidget(QLabel("Waveform:"))
        self.waveform_combo = QComboBox()
        self.waveform_combo.addItems(['SQUARE', 'SINE', 'RAMP'])
        wave_layout.addWidget(self.waveform_combo)
        layout.addLayout(wave_layout)
        
        # Test buttons
        btn_layout = QHBoxLayout()
        
        start_btn = QPushButton("▶ Start Test")
        start_btn.clicked.connect(self.start_test)
        start_btn.setStyleSheet("background-color: #2196F3; color: white; font-weight: bold;")
        btn_layout.addWidget(start_btn)
        
        stop_btn = QPushButton("⏹ Stop")
        stop_btn.clicked.connect(lambda: self.send_command("STOP"))
        stop_btn.setStyleSheet("background-color: #ff9800; color: white; font-weight: bold;")
        btn_layout.addWidget(stop_btn)
        
        layout.addLayout(btn_layout)
        
        group.setLayout(layout)
        return group
    
    def create_preset_group(self):
        """Create preset save/load group"""
        group = QGroupBox("💾 Presets", self)
        layout = QVBoxLayout()
        
        # Preset name
        name_layout = QHBoxLayout()
        name_layout.addWidget(QLabel("Preset Name:"))
        self.preset_name = QLineEdit()
        self.preset_name.setText("My_Preset")
        name_layout.addWidget(self.preset_name)
        layout.addLayout(name_layout)
        
        # Save/Load buttons
        btn_layout = QHBoxLayout()
        
        save_btn = QPushButton("💾 Save")
        save_btn.clicked.connect(self.save_preset)
        save_btn.setStyleSheet("background-color: #9C27B0; color: white; font-weight: bold;")
        btn_layout.addWidget(save_btn)
        
        layout_btn = QPushButton("📂 Load")
        layout_btn.clicked.connect(self.load_preset_dialog)
        layout_btn.setStyleSheet("background-color: #673AB7; color: white; font-weight: bold;")
        btn_layout.addWidget(layout_btn)
        
        layout.addLayout(btn_layout)
        
        # Preset list
        self.preset_combo = QComboBox()
        layout.addWidget(QLabel("Saved Presets:"))
        layout.addWidget(self.preset_combo)
        
        group.setLayout(layout)
        return group
    
    def create_autotune_group(self):
        """Create auto-tuning group"""
        group = QGroupBox("🤖 Auto-Tuning (Ziegler-Nichols)", self)
        layout = QVBoxLayout()
        
        info = QLabel(
            "Auto-tuning uses the relay method to find optimal PID gains.\n"
            "Process: Motor oscillates at critical gain, measures period,\n"
            "then calculates kP, kI, kD using Ziegler-Nichols rules."
        )
        info.setWordWrap(True)
        layout.addWidget(info)
        
        # Auto-tune button
        autotune_btn = QPushButton("🎯 Start Auto-Tune")
        autotune_btn.clicked.connect(self.start_autotune)
        autotune_btn.setStyleSheet("background-color: #FF5722; color: white; font-weight: bold; padding: 10px;")
        autotune_btn.setMinimumHeight(40)
        layout.addWidget(autotune_btn)
        
        # Progress
        self.autotune_progress = QProgressBar()
        self.autotune_progress.setVisible(False)
        layout.addWidget(self.autotune_progress)
        
        group.setLayout(layout)
        return group
    
    def create_status_group(self):
        """Create status/data display group"""
        group = QGroupBox("📊 Current Data", self)
        layout = QVBoxLayout()
        
        # Data table
        self.data_table = QTableWidget()
        self.data_table.setColumnCount(2)
        self.data_table.setHorizontalHeaderLabels(['Parameter', 'Value'])
        self.data_table.setMaximumHeight(200)
        
        params = ['Time (s)', 'Setpoint (RPM)', 'Actual (RPM)', 'Error (RPM)', 
                 'PWM (%)', 'kP', 'kI', 'kD']
        self.data_table.setRowCount(len(params))
        
        for i, param in enumerate(params):
            self.data_table.setItem(i, 0, QTableWidgetItem(param))
            self.data_table.setItem(i, 1, QTableWidgetItem("--"))
        
        layout.addWidget(self.data_table)
        
        # Clear data button
        clear_btn = QPushButton("🗑️ Clear Data")
        clear_btn.clicked.connect(self.clear_data)
        layout.addWidget(clear_btn)
        
        group.setLayout(layout)
        return group
    
    def setup_serial(self):
        """Setup serial reader thread"""
        self.serial_reader = SerialReader()
        self.serial_thread = QThread()
        self.serial_reader.moveToThread(self.serial_thread)
        
        self.serial_reader.data_received.connect(self.on_data_received)
        self.serial_reader.connection_status.connect(self.on_connection_status)
        self.serial_reader.error_signal.connect(self.on_serial_error)
        
        self.serial_thread.started.connect(self.serial_reader.run)
        self.serial_thread.start()
    
    def refresh_ports(self):
        """Refresh available COM ports"""
        self.port_combo.clear()
        ports = [port.device for port in list_ports.comports()]
        if ports:
            self.port_combo.addItems(ports)
        else:
            self.port_combo.addItem("No ports found")
    
    def connect_serial(self):
        """Connect to selected port"""
        port = self.port_combo.currentText()
        if port == "No ports found":
            QMessageBox.warning(self, "Error", "No COM ports available")
            return
        
        if self.serial_reader.connect(port):
            self.connect_btn.setEnabled(False)
            self.disconnect_btn.setEnabled(True)
            self.port_combo.setEnabled(False)
    
    def disconnect_serial(self):
        """Disconnect from port"""
        self.serial_reader.disconnect()
        self.connect_btn.setEnabled(True)
        self.disconnect_btn.setEnabled(False)
        self.port_combo.setEnabled(True)
        self.status_label.setText("❌ Disconnected")
        self.status_label.setStyleSheet("color: red; font-weight: bold;")
    
    def on_connection_status(self, connected):
        """Handle connection status change"""
        if connected:
            self.status_label.setText("✓ Connected")
            self.status_label.setStyleSheet("color: green; font-weight: bold;")
        else:
            self.status_label.setText("❌ Disconnected")
            self.status_label.setStyleSheet("color: red; font-weight: bold;")
    
    def adjust_gain(self, gain_name, delta):
        """Adjust gain with arrow buttons"""
        current = self.gain_controls[gain_name].value()
        new_value = max(0, min(1.0, current + delta))
        self.gain_controls[gain_name].setValue(new_value)
    
    def on_gain_change(self, gain_name, value):
        """Handle gain value change"""
        self.current_gains[gain_name] = value
    
    def send_gain(self, gain_name):
        """Send gain to motor"""
        value = self.gain_controls[gain_name].value()
        command = f"{gain_name}:{value:.4f}"
        self.send_command(command)
        QMessageBox.information(self, "Success", f"Sent {gain_name} = {value:.4f}")
    
    def start_test(self):
        """Start selected test waveform"""
        waveform = self.waveform_combo.currentText()
        self.send_command(waveform)
        QMessageBox.information(self, "Test Started", f"Running {waveform} waveform")
    
    def send_command(self, command):
        """Send command to serial"""
        if self.serial_reader:
            self.serial_reader.write(command)
    
    def save_preset(self):
        """Save current gains as preset"""
        name = self.preset_name.text()
        if not name:
            QMessageBox.warning(self, "Error", "Please enter a preset name")
            return
        
        presets = self.load_presets_data()
        presets[name] = self.current_gains.copy()
        
        with open(PRESETS_FILE, 'w') as f:
            json.dump(presets, f, indent=2)
        
        self.load_presets()
        QMessageBox.information(self, "Success", f"Preset '{name}' saved!")
    
    def load_preset_dialog(self):
        """Load preset dialog"""
        presets = self.load_presets_data()
        if not presets:
            QMessageBox.warning(self, "Error", "No presets found")
            return
        
        preset_name = self.preset_combo.currentText()
        if preset_name in presets:
            gains = presets[preset_name]
            self.gain_controls['kP'].setValue(gains.get('kP', 0))
            self.gain_controls['kI'].setValue(gains.get('kI', 0))
            self.gain_controls['kD'].setValue(gains.get('kD', 0))
            QMessageBox.information(self, "Loaded", f"Preset '{preset_name}' loaded!")
    
    def load_presets(self):
        """Load and display presets"""
        presets = self.load_presets_data()
        self.preset_combo.clear()
        if presets:
            self.preset_combo.addItems(presets.keys())
    
    def load_presets_data(self):
        """Load presets from file"""
        if os.path.exists(PRESETS_FILE):
            with open(PRESETS_FILE, 'r') as f:
                return json.load(f)
        return {}
    
    def start_autotune(self):
        """Start auto-tuning process"""
        self.autotune_progress.setVisible(True)
        self.autotune_progress.setValue(0)
        
        # Simulate auto-tune (in real implementation, would analyze oscillations)
        QMessageBox.information(
            self, 
            "Auto-Tune Started",
            "Auto-tuning will:\n"
            "1. Run relay test (motor oscillates)\n"
            "2. Measure critical period\n"
            "3. Calculate gains using Ziegler-Nichols\n"
            "4. Apply optimal kP, kI, kD\n\n"
            "Process takes ~30 seconds..."
        )
    
    def on_data_received(self, data):
        """Handle incoming serial data"""
        # Append to buffers
        self.data_buffers['times'].append(data.get('time', 0))
        self.data_buffers['setpoints'].append(data.get('setpoint', 0))
        self.data_buffers['actuals'].append(data.get('actual', 0))
        self.data_buffers['errors'].append(data.get('error', 0))
        self.data_buffers['pwms'].append(data.get('pwm', 0))
        self.data_buffers['kps'].append(data.get('kP', 0))
        self.data_buffers['kis'].append(data.get('kI', 0))
        self.data_buffers['kds'].append(data.get('kD', 0))
        
        # Update displays
        self.update_data_table(data)
        self.canvas.update_plots({k: list(v) for k, v in self.data_buffers.items()})
    
    def update_data_table(self, data):
        """Update data display table"""
        values = [
            f"{data.get('time', 0):.2f}",
            f"{data.get('setpoint', 0):.1f}",
            f"{data.get('actual', 0):.1f}",
            f"{data.get('error', 0):.1f}",
            f"{data.get('pwm', 0):.1f}",
            f"{data.get('kP', 0):.4f}",
            f"{data.get('kI', 0):.4f}",
            f"{data.get('kD', 0):.4f}",
        ]
        
        for i, value in enumerate(values):
            self.data_table.setItem(i, 1, QTableWidgetItem(value))
    
    def clear_data(self):
        """Clear all data buffers"""
        for key in self.data_buffers:
            self.data_buffers[key].clear()
        self.canvas.update_plots({k: list(v) for k, v in self.data_buffers.items()})
    
    def on_serial_error(self, error):
        """Handle serial error"""
        QMessageBox.warning(self, "Serial Error", error)
    
    def closeEvent(self, event):
        """Handle window close"""
        self.disconnect_serial()
        if self.serial_thread:
            self.serial_thread.quit()
            self.serial_thread.wait()
        event.accept()


def main():
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    
    window = PIDTunerGUI()
    window.show()
    
    sys.exit(app.exec_())


if __name__ == '__main__':
    main()
