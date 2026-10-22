#include "logic.h"
#include "test_runner.h"

TEST(init_sets_forward_direction) {
  motor_init();
  ASSERT_TRUE(motor_direction());
}

TEST(init_sets_zero_power) {
  motor_init();
  ASSERT_EQ(motor_power(), 0);
}

TEST(init_configures_pins) {
  motor_init();
  // After init, pins should be configured as outputs
  // IN1 should be LOW, IN2 should be HIGH (forward direction)
  ASSERT_PIN(MOTOR_PIN_IN1, LOW);
  ASSERT_PIN(MOTOR_PIN_IN2, HIGH);
}

TEST(set_direction_forward) {
  motor_init();
  motor_set_direction(true);
  ASSERT_TRUE(motor_direction());
  ASSERT_PIN(MOTOR_PIN_IN1, LOW);
  ASSERT_PIN(MOTOR_PIN_IN2, HIGH);
}

TEST(set_direction_reverse) {
  motor_init();
  motor_set_direction(false);
  ASSERT_FALSE(motor_direction());
  ASSERT_PIN(MOTOR_PIN_IN1, HIGH);
  ASSERT_PIN(MOTOR_PIN_IN2, LOW);
}

TEST(set_power_clamping) {
  motor_init();
  motor_set_power(-10);
  ASSERT_EQ(motor_power(), 0);
  motor_set_power(300);
  ASSERT_EQ(motor_power(), 255);
  motor_set_power(128);
  ASSERT_EQ(motor_power(), 128);
}

TEST(power_from_reading_conversion) {
  motor_init();
  // Test boundary and middle values
  ASSERT_EQ(motor_power_from_reading(0), 0);
  ASSERT_EQ(motor_power_from_reading(1023), 255);
  ASSERT_EQ(motor_power_from_reading(512), 128);
  // Test clamping in the function
  ASSERT_EQ(motor_power_from_reading(-10), 0);
  ASSERT_EQ(motor_power_from_reading(1100), 255);
}

TEST(motor_stop) {
  motor_init();
  motor_set_power(200);
  motor_set_direction(false);
  motor_stop();
  ASSERT_PIN(MOTOR_PIN_IN1, LOW);
  ASSERT_PIN(MOTOR_PIN_IN2, LOW);
  ASSERT_EQ(motor_power(), 0);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {
}
