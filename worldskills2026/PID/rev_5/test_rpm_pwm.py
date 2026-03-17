#!/usr/bin/env python3
"""
Motor PWM -> RPM characterization test
Reads encoder and logs RPM at different PWM levels
"""
import time
import RPi.GPIO as GPIO
from collections import deque

# CONFIG
ENCODER_PPR = 360  # Change to your encoder PPR
GEAR_RATIO = 20
PWM_PIN = 12
PWM_FREQ = 20000

def init_encoder(channelA, channelB):
    """Initialize quadrature encoder on pins A, B"""
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(channelA, GPIO.IN)
    GPIO.setup(channelB, GPIO.IN)
    
    counter = [0]
    prev_state = [GPIO.input(channelA) << 1 | GPIO.input(channelB)]
    
    def update_counter(channel):
        state = GPIO.input(channelA) << 1 | GPIO.input(channelB)
        # Simple Gray code decoder
        if (prev_state[0] ^ state) == 1 or (prev_state[0] ^ state) == 2:
            counter[0] += 1 if (prev_state[0] ^ state) == 1 else -1
        prev_state[0] = state
    
    GPIO.add_event_detect(channelA, GPIO.BOTH, callback=update_counter)
    GPIO.add_event_detect(channelB, GPIO.BOTH, callback=update_counter)
    
    return counter

def get_rpm(counter, dt=1.0):
    """Calculate RPM from encoder counter over dt seconds"""
    initial = counter[0]
    time.sleep(dt)
    final = counter[0]
    
    counts_per_sec = (final - initial) / dt
    rpm = (counts_per_sec / ENCODER_PPR) / 4 * 60 / GEAR_RATIO
    return rpm, final - initial

def test_pwm_response():
    """Test motor response at different PWM levels"""
    counter = init_encoder(23, 24)  # GPIO23, GPIO24 for encoder A, B
    
    GPIO.setup(PWM_PIN, GPIO.OUT)
    pwm = GPIO.PWM(PWM_PIN, PWM_FREQ)
    pwm.start(0)
    
    print("PWM(%) | Counts/sec | RPM")
    print("-" * 35)
    
    try:
        for pwm_val in range(0, 101, 10):
            pwm.ChangeDutyCycle(pwm_val)
            time.sleep(2)  # Settle time
            
            rpm, counts = get_rpm(counter, dt=2.0)
            counts_per_sec = counts / 2.0
            
            print(f"{pwm_val:6} | {counts_per_sec:9.1f} | {rpm:7.1f}")
            
    finally:
        pwm.stop()
        GPIO.cleanup()

if __name__ == "__main__":
    test_pwm_response()
