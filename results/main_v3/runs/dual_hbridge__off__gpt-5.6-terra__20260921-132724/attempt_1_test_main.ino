#include "logic.h"
#include "test_runner.h"

TEST(init_stops_both_motors) {
  hb_init();
  ASSERT_PIN(HB_PIN_IN1, LOW);
  ASSERT_PIN(HB_PIN_IN2, LOW);
  ASSERT_PIN(HB_PIN_ENA, LOW);
  ASSERT_PIN(HB_PIN_IN3, LOW);
  ASSERT_PIN(HB_PIN_IN4, LOW);
  ASSERT_PIN(HB_PIN_ENB, LOW);
  ASSERT_EQ(hb_direction(HB_MOTOR_A), 0);
  ASSERT_EQ(hb_direction(HB_MOTOR_B), 0);
  ASSERT_EQ(hb_speed(HB_MOTOR_A), 0);
  ASSERT_EQ(hb_speed(HB_MOTOR_B), 0);
}

TEST(forward_motor_a_sets_direction_and_speed) {
  hb_init();
  ASSERT_TRUE(hb_forward(HB_MOTOR_A, 255));
  ASSERT_PIN(HB_PIN_IN1, HIGH);
  ASSERT_PIN(HB_PIN_IN2, LOW);
  ASSERT_PIN(HB_PIN_ENA, HIGH);
  ASSERT_EQ(hb_direction(HB_MOTOR_A), 1);
  ASSERT_EQ(hb_speed(HB_MOTOR_A), 255);
  ASSERT_EQ(hb_direction(HB_MOTOR_B), 0);
  ASSERT_EQ(hb_speed(HB_MOTOR_B), 0);
}

TEST(backward_motor_b_sets_direction_and_speed) {
  hb_init();
  ASSERT_TRUE(hb_backward(HB_MOTOR_B, 123));
  ASSERT_PIN(HB_PIN_IN3, LOW);
  ASSERT_PIN(HB_PIN_IN4, HIGH);
  ASSERT_EQ(hb_direction(HB_MOTOR_B), -1);
  ASSERT_EQ(hb_speed(HB_MOTOR_B), 123);
  ASSERT_EQ(hb_direction(HB_MOTOR_A), 0);
  ASSERT_EQ(hb_speed(HB_MOTOR_A), 0);
}

TEST(stop_motor_clears_motor_state) {
  hb_init();
  ASSERT_TRUE(hb_forward(HB_MOTOR_A, 255));
  ASSERT_TRUE(hb_stop(HB_MOTOR_A));
  ASSERT_PIN(HB_PIN_IN1, LOW);
  ASSERT_PIN(HB_PIN_IN2, LOW);
  ASSERT_PIN(HB_PIN_ENA, LOW);
  ASSERT_EQ(hb_direction(HB_MOTOR_A), 0);
  ASSERT_EQ(hb_speed(HB_MOTOR_A), 0);
}

TEST(stop_all_stops_both_motors) {
  hb_init();
  ASSERT_TRUE(hb_forward(HB_MOTOR_A, 255));
  ASSERT_TRUE(hb_backward(HB_MOTOR_B, 255));
  hb_stop_all();
  ASSERT_PIN(HB_PIN_IN1, LOW);
  ASSERT_PIN(HB_PIN_IN2, LOW);
  ASSERT_PIN(HB_PIN_ENA, LOW);
  ASSERT_PIN(HB_PIN_IN3, LOW);
  ASSERT_PIN(HB_PIN_IN4, LOW);
  ASSERT_PIN(HB_PIN_ENB, LOW);
  ASSERT_EQ(hb_direction(HB_MOTOR_A), 0);
  ASSERT_EQ(hb_direction(HB_MOTOR_B), 0);
  ASSERT_EQ(hb_speed(HB_MOTOR_A), 0);
  ASSERT_EQ(hb_speed(HB_MOTOR_B), 0);
}

TEST(invalid_motor_is_rejected) {
  hb_init();
  ASSERT_FALSE(hb_forward(2, 255));
  ASSERT_FALSE(hb_backward(3, 255));
  ASSERT_FALSE(hb_stop(4));
  ASSERT_EQ(hb_direction(2), 0);
  ASSERT_EQ(hb_speed(2), 0);
  ASSERT_PIN(HB_PIN_IN1, LOW);
  ASSERT_PIN(HB_PIN_IN2, LOW);
  ASSERT_PIN(HB_PIN_IN3, LOW);
  ASSERT_PIN(HB_PIN_IN4, LOW);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
