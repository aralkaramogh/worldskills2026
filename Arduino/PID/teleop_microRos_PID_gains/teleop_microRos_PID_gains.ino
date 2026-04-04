// ── ADD THESE DEFINES (TOP) ──────────────────────────────────
#define CONTROL_INTERVAL 100   // ms
#define PPR 20                // adjust to your encoder

// ── PID GAINS ────────────────────────────────────────────────
float Kp = 2.80;
float Ki = 1.60;
float Kd = 0.02;

// ── ROBOT PARAMS ─────────────────────────────────────────────
#define WHEEL_RADIUS 0.4575   // meters (UPDATE)
#define BASE_WIDTH   0.15   // meters (UPDATE)

// ── PID STATE ────────────────────────────────────────────────
float targetRPM_L = 0, targetRPM_R = 0;
float currentRPM_L = 0, currentRPM_R = 0;

float eL=0, pL=0, iL=0;
float eR=0, pR=0, iR=0;

#include <Arduino.h>
#include <micro_ros_arduino.h>
#include <stdio.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <geometry_msgs/msg/twist.h>

// --- micro-ROS Variables ---
rcl_subscription_t subscriber;
geometry_msgs__msg__Twist msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}

// --- AMR Physical Properties (UPDATE THESE) ---
const float WHEEL_SEPARATION = 0.35; // Distance between left and right wheels in meters
const float WHEEL_RADIUS = 0.05;     // Radius of the wheels in meters

// --- MDD10 Pin Definitions for ESP32-S3 (UPDATE THESE) ---
const int PWM_L = 4;
const int DIR_L = 5;
const int PWM_R = 6;
const int DIR_R = 7;

// --- Encoder Pins (UPDATE THESE) ---
const int ENC_L_A = 18;
const int ENC_L_B = 17;
const int ENC_R_A = 16;
const int ENC_R_B = 15;

// --- PID Gains ---
float Kp = 2.80;
float Ki = 1.60;
float Kd = 0.02;

// --- Motor 1 (Left) PID Variables ---
float target_vel_L = 0.0; 
volatile float current_vel_L = 0.0; 
float prev_error_L = 0.0;
float integral_L = 0.0;

// --- Motor 2 (Right) PID Variables ---
float target_vel_R = 0.0;
volatile float current_vel_R = 0.0; 
float prev_error_R = 0.0;
float integral_R = 0.0;

// --- Time Tracking ---
unsigned long prev_pid_time = 0;
volatile long encoder_ticks_L = 0;
volatile long encoder_ticks_R = 0;

// --- Error Loop for micro-ROS ---
void error_loop(){
  while(1){
    // Flash an onboard LED here if you want a visual error indicator
    delay(100);
  }
}

// --- Encoder Interrupt Service Routines ---
void IRAM_ATTR readEncoderLeft() {
  // Add your specific quadrature reading logic here
  encoder_ticks_L++; 
}

void IRAM_ATTR readEncoderRight() {
  // Add your specific quadrature reading logic here
  encoder_ticks_R++;
}

// --- micro-ROS Twist Callback ---
void twist_callback(const void * msgin)
{  
  const geometry_msgs__msg__Twist * twist_msg = (const geometry_msgs__msg__Twist *)msgin;
  
  float linear_x = twist_msg->linear.x;
  float angular_z = twist_msg->angular.z;

  // Differential Drive Kinematics
  target_vel_L = linear_x - (angular_z * WHEEL_SEPARATION / 2.0);
  target_vel_R = linear_x + (angular_z * WHEEL_SEPARATION / 2.0);
}

// --- PID Computation ---
float computePID(float target, float current, float &prev_error, float &integral, float dt) {
  float error = target - current;
  
  float Pout = Kp * error;
  
  integral += error * dt;
  integral = constrain(integral, -255, 255); // Anti-windup
  float Iout = Ki * integral;
  
  float derivative = (error - prev_error) / dt;
  float Dout = Kd * derivative;
  
  prev_error = error;
  
  float output = Pout + Iout + Dout;
  return constrain(output, -255, 255); 
}

// --- Motor Actuation ---
void driveMotor(int pwm_pin, int dir_pin, float pid_output) {
  if (pid_output >= 0) {
    digitalWrite(dir_pin, HIGH); // Forward
  } else {
    digitalWrite(dir_pin, LOW);  // Reverse
  }
  
  int pwm_val = abs((int)pid_output);
  analogWrite(pwm_pin, pwm_val); 
}

void setup() {
  // Setup Motor Pins
  pinMode(PWM_L, OUTPUT);
  pinMode(DIR_L, OUTPUT);
  pinMode(PWM_R, OUTPUT);
  pinMode(DIR_R, OUTPUT);
  
  // Setup Encoder Pins & Interrupts
  pinMode(ENC_L_A, INPUT_PULLUP);
  pinMode(ENC_R_A, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_L_A), readEncoderLeft, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_R_A), readEncoderRight, RISING);

  // --- micro-ROS Serial Transport Setup ---
  // This uses the default USB/Serial connection
  set_microros_serial_transports(Serial);
  
  delay(2000);

  allocator = rcl_get_default_allocator();

  // Create init_options
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // Create node
  RCCHECK(rclc_node_init_default(&node, "esp32_amr_node", "", &support));

  // Create subscriber
  RCCHECK(rclc_subscription_init_default(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
    "cmd_vel"));

  // Create executor
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &msg, &twist_callback, ON_NEW_DATA));
}

void loop() {
  // 1. Process micro-ROS callbacks
  RCCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10)));
  
  unsigned long current_time = millis();
  float dt = (current_time - prev_pid_time) / 1000.0;
  
  // 2. Run PID Loop at ~50Hz (every 20ms)
  if (dt >= 0.02) {
    
    // Convert encoder ticks to velocity (meters per second)
    // You will need to adjust this math based on your motor's CPR (Counts Per Revolution)
    // Example: current_vel_L = (ticks / CPR) * (2 * PI * WHEEL_RADIUS) / dt;
    
    // 3. Compute PID outputs
    float output_L = computePID(target_vel_L, current_vel_L, prev_error_L, integral_L, dt);
    float output_R = computePID(target_vel_R, current_vel_R, prev_error_R, integral_R, dt);
    
    // 4. Drive the motors
    driveMotor(PWM_L, DIR_L, output_L);
    driveMotor(PWM_R, DIR_R, output_R);
    
    // Reset ticks and time for next loop
    encoder_ticks_L = 0;
    encoder_ticks_R = 0;
    prev_pid_time = current_time;
  }
}

// ── TIMING ───────────────────────────────────────────────────
unsigned long lastControlMs = 0;

// ================================================================
// RPM CALCULATION
// ================================================================
void updateRPM() {
  static int32_t prevL = 0, prevR = 0;

  int32_t currL, currR;

  noInterrupts();
  currL = leftTicks;
  currR = rightTicks;
  interrupts();

  int32_t dL = currL - prevL;
  int32_t dR = currR - prevR;

  prevL = currL;
  prevR = currR;

  // 100ms → multiply by 600 to get RPM
  currentRPM_L = (dL * 600.0) / PPR;
  currentRPM_R = (dR * 600.0) / PPR;
}

// ================================================================
// PID FUNCTION
// ================================================================
int computePID(float target, float current, float &e, float &p, float &i) {
  e = target - current;
  i += e;

  float d = e - p;
  p = e;

  float out = Kp*e + Ki*i + Kd*d;

  return constrain((int)out, -255, 255);
}

// ================================================================
// UPDATED CALLBACK (IMPORTANT CHANGE)
// ================================================================
void twist_callback(const void * msgin) {
  const geometry_msgs__msg__Twist * twist =
      (const geometry_msgs__msg__Twist *)msgin;

  float v = twist->linear.x;     // m/s
  float w = twist->angular.z;    // rad/s

  // Differential drive kinematics
  float vL = v - (w * BASE_WIDTH / 2.0);
  float vR = v + (w * BASE_WIDTH / 2.0);

  // Convert to RPM
  targetRPM_L = (vL * 60.0) / (2 * PI * WHEEL_RADIUS);
  targetRPM_R = (vR * 60.0) / (2 * PI * WHEEL_RADIUS);
}

// ================================================================
// MODIFY LOOP()
// ================================================================
void loop() {
  // micro-ROS spin
  RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10)));

  unsigned long now = millis();

  // ── PID CONTROL LOOP ───────────────────────────────────────
  if (now - lastControlMs >= CONTROL_INTERVAL) {
    lastControlMs = now;

    updateRPM();

    int pwmL_out = computePID(targetRPM_L, currentRPM_L, eL, pL, iL);
    int pwmR_out = computePID(targetRPM_R, currentRPM_R, eR, pR, iR);

    setMotors(pwmL_out, pwmR_out);

    // Debug (VERY useful for tuning)
    Serial.printf("T: %.1f %.1f | C: %.1f %.1f\n",
                  targetRPM_L, targetRPM_R,
                  currentRPM_L, currentRPM_R);
  }

  // ── TICK PUBLISH ───────────────────────────────────────────
  if (now - lastTickPublishMs >= TICK_PUBLISH_MS) {
    lastTickPublishMs = now;

    noInterrupts();
    int32_t snap_left  = leftTicks;
    int32_t snap_right = rightTicks;
    interrupts();

    ticks_msg.x = (double)snap_left;
    ticks_msg.y = (double)snap_right;
    ticks_msg.z = 0.0;

    RCSOFTCHECK(rcl_publish(&pub_ticks, &ticks_msg, NULL));
  }

  delay(5);
}