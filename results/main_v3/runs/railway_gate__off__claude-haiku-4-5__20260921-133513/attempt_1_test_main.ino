#include "logic.h"
#include "test_runner.h"

TEST(init_closes_gates) {
  gate_init();
  ASSERT_EQ(gate_servo1_angle(), GATE_SERVO1_CLOSED);
  ASSERT_EQ(gate_servo2_angle(), GATE_SERVO2_CLOSED);
  ASSERT_PIN(GATE_PIN_LED_RED, HIGH);
  ASSERT_PIN(GATE_PIN_LED_GREEN, LOW);
  ASSERT_FALSE(gate_is_open());
  ASSERT_FALSE(gate_object_between());
}

TEST(open_gates_and_leds) {
  gate_init();
  gate_open();
  ASSERT_EQ(gate_servo1_angle(), GATE_SERVO1_OPEN);
  ASSERT_EQ(gate_servo2_angle(), GATE_SERVO2_OPEN);
  ASSERT_PIN(GATE_PIN_LED_GREEN, HIGH);
  ASSERT_PIN(GATE_PIN_LED_RED, LOW);
  ASSERT_TRUE(gate_is_open());
}

TEST(close_gates_and_leds) {
  gate_init();
  gate_open();
  gate_close();
  ASSERT_EQ(gate_servo1_angle(), GATE_SERVO1_CLOSED);
  ASSERT_EQ(gate_servo2_angle(), GATE_SERVO2_CLOSED);
  ASSERT_PIN(GATE_PIN_LED_GREEN, LOW);
  ASSERT_PIN(GATE_PIN_LED_RED, HIGH);
  ASSERT_FALSE(gate_is_open());
}

TEST(object_detection_sensor_values) {
  gate_init();
  pinMode(GATE_PIN_SENSOR1, INPUT_PULLUP);
  pinMode(GATE_PIN_SENSOR2, INPUT_PULLUP);
  digitalWrite(GATE_PIN_SENSOR1, HIGH);
  digitalWrite(GATE_PIN_SENSOR2, HIGH);
  delay(5);
  ASSERT_FALSE(gate_sensor1_detected());
  ASSERT_FALSE(gate_sensor2_detected());
  digitalWrite(GATE_PIN_SENSOR1, LOW);
  delay(5);
  ASSERT_TRUE(gate_sensor1_detected());
}

TEST(state_machine_sensor1_opens_gate) {
  gate_init();
  pinMode(GATE_PIN_SENSOR1, INPUT_PULLUP);
  pinMode(GATE_PIN_SENSOR2, INPUT_PULLUP);
  digitalWrite(GATE_PIN_SENSOR1, HIGH);
  digitalWrite(GATE_PIN_SENSOR2, HIGH);
  delay(5);
  ASSERT_FALSE(gate_object_between());
  gate_update(true, false);
  ASSERT_TRUE(gate_object_between());
  ASSERT_TRUE(gate_is_open());
}

TEST(state_machine_sensor2_closes_gate) {
  gate_init();
  gate_update(true, false);
  ASSERT_TRUE(gate_object_between());
  ASSERT_TRUE(gate_is_open());
  gate_update(false, true);
  ASSERT_FALSE(gate_object_between());
  ASSERT_FALSE(gate_is_open());
  ASSERT_PIN(GATE_PIN_LED_RED, HIGH);
  ASSERT_PIN(GATE_PIN_LED_GREEN, LOW);
}

TEST(servo_angles_after_operations) {
  gate_init();
  ASSERT_EQ(gate_servo1_angle(), GATE_SERVO1_CLOSED);
  ASSERT_EQ(gate_servo2_angle(), GATE_SERVO2_CLOSED);
  gate_open();
  ASSERT_EQ(gate_servo1_angle(), GATE_SERVO1_OPEN);
  ASSERT_EQ(gate_servo2_angle(), GATE_SERVO2_OPEN);
  gate_close();
  ASSERT_EQ(gate_servo1_angle(), GATE_SERVO1_CLOSED);
  ASSERT_EQ(gate_servo2_angle(), GATE_SERVO2_CLOSED);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
