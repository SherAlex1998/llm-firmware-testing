#include "logic.h"
#include "test_runner.h"

TEST(init_closes_gates_and_red_led) {
  gate_init();
  ASSERT_EQ(gate_servo1_angle(), GATE_SERVO1_CLOSED);
  ASSERT_EQ(gate_servo2_angle(), GATE_SERVO2_CLOSED);
  ASSERT_PIN(2, LOW);   // green LED off
  ASSERT_PIN(4, HIGH);  // red LED on
  ASSERT_FALSE(gate_is_open());
  ASSERT_FALSE(gate_object_between());
}

TEST(gate_open_moves_servos_and_leds) {
  gate_init();
  gate_open();
  ASSERT_EQ(gate_servo1_angle(), GATE_SERVO1_OPEN);
  ASSERT_EQ(gate_servo2_angle(), GATE_SERVO2_OPEN);
  ASSERT_PIN(2, HIGH);  // green LED on
  ASSERT_PIN(4, LOW);   // red LED off
  ASSERT_TRUE(gate_is_open());
}

TEST(gate_close_moves_servos_and_leds) {
  gate_init();
  gate_open();
  gate_close();
  ASSERT_EQ(gate_servo1_angle(), GATE_SERVO1_CLOSED);
  ASSERT_EQ(gate_servo2_angle(), GATE_SERVO2_CLOSED);
  ASSERT_PIN(2, LOW);   // green LED off
  ASSERT_PIN(4, HIGH);  // red LED on
  ASSERT_FALSE(gate_is_open());
}

TEST(sensor_detection_and_update) {
  gate_init();
  // Initially no object
  ASSERT_FALSE(gate_sensor1_detected());
  ASSERT_FALSE(gate_sensor2_detected());
  ASSERT_FALSE(gate_object_between());
  
  // Simulate object detected at sensor 1
  pinMode(GATE_PIN_SENSOR1, OUTPUT);
  digitalWrite(GATE_PIN_SENSOR1, LOW);
  delay(10);
  ASSERT_TRUE(gate_sensor1_detected());
  
  // Update with sensor 1 detected should open gate
  gate_update(true, false);
  ASSERT_TRUE(gate_object_between());
  ASSERT_TRUE(gate_is_open());
  
  // Release sensor 1
  digitalWrite(GATE_PIN_SENSOR1, HIGH);
  delay(10);
  
  // Simulate object detected at sensor 2
  pinMode(GATE_PIN_SENSOR2, OUTPUT);
  digitalWrite(GATE_PIN_SENSOR2, LOW);
  delay(10);
  ASSERT_TRUE(gate_sensor2_detected());
  
  // Update with sensor 2 detected should close gate and clear object flag
  gate_update(false, true);
  ASSERT_FALSE(gate_object_between());
  ASSERT_FALSE(gate_is_open());
}

TEST(object_between_transitions) {
  gate_init();
  ASSERT_FALSE(gate_object_between());
  
  // Object enters at sensor 1
  gate_update(true, false);
  ASSERT_TRUE(gate_object_between());
  ASSERT_TRUE(gate_is_open());
  
  // Both sensors detect (object still between)
  gate_update(true, true);
  ASSERT_TRUE(gate_object_between());
  
  // Object exits at sensor 2
  gate_update(false, true);
  ASSERT_FALSE(gate_object_between());
  ASSERT_FALSE(gate_is_open());
}

TEST(gate_servo_angles_defined) {
  gate_init();
  ASSERT_EQ(GATE_SERVO1_CLOSED, 180);
  ASSERT_EQ(GATE_SERVO1_OPEN, 90);
  ASSERT_EQ(GATE_SERVO2_CLOSED, 90);
  ASSERT_EQ(GATE_SERVO2_OPEN, 180);
}

TEST(poll_reads_sensors_and_updates) {
  gate_init();
  
  // Set up sensor 1 to LOW (detected)
  pinMode(GATE_PIN_SENSOR1, OUTPUT);
  digitalWrite(GATE_PIN_SENSOR1, LOW);
  delay(10);
  
  // Poll should read sensor and open gate
  gate_poll();
  ASSERT_TRUE(gate_object_between());
  ASSERT_TRUE(gate_is_open());
  
  // Release sensor 1 and set sensor 2 to LOW
  digitalWrite(GATE_PIN_SENSOR1, HIGH);
  pinMode(GATE_PIN_SENSOR2, OUTPUT);
  digitalWrite(GATE_PIN_SENSOR2, LOW);
  delay(10);
  
  // Poll should read sensors and close gate
  gate_poll();
  ASSERT_FALSE(gate_object_between());
  ASSERT_FALSE(gate_is_open());
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
