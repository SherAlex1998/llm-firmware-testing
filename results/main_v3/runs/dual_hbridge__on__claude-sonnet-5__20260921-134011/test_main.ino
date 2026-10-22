#include "logic.h"
#include "test_runner.h"

TEST(init_stops_all) {
  hb_init();
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(11, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_EQ(hb_direction(HB_MOTOR_A), 0);
  ASSERT_EQ(hb_direction(HB_MOTOR_B), 0);
  ASSERT_EQ(hb_speed(HB_MOTOR_A), 0);
  ASSERT_EQ(hb_speed(HB_MOTOR_B), 0);
}

TEST(forward_motor_a) {
  hb_init();
  ASSERT_TRUE(hb_forward(HB_MOTOR_A, 200));
  ASSERT_PIN(8, HIGH);
  ASSERT_PIN(9, LOW);
  ASSERT_EQ(hb_direction(HB_MOTOR_A), 1);
  ASSERT_EQ(hb_speed(HB_MOTOR_A), 200);
  // motor B untouched
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(11, LOW);
  ASSERT_EQ(hb_direction(HB_MOTOR_B), 0);
}

TEST(backward_motor_b) {
  hb_init();
  ASSERT_TRUE(hb_backward(HB_MOTOR_B, 150));
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(11, HIGH);
  ASSERT_EQ(hb_direction(HB_MOTOR_B), -1);
  ASSERT_EQ(hb_speed(HB_MOTOR_B), 150);
  // motor A untouched
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(9, LOW);
  ASSERT_EQ(hb_direction(HB_MOTOR_A), 0);
}

TEST(stop_single_motor) {
  hb_init();
  hb_forward(HB_MOTOR_A, 255);
  ASSERT_TRUE(hb_stop(HB_MOTOR_A));
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(9, LOW);
  ASSERT_EQ(hb_direction(HB_MOTOR_A), 0);
  ASSERT_EQ(hb_speed(HB_MOTOR_A), 0);
}

TEST(stop_all_motors) {
  hb_init();
  hb_forward(HB_MOTOR_A, 100);
  hb_backward(HB_MOTOR_B, 100);
  hb_stop_all();
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(11, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_EQ(hb_direction(HB_MOTOR_A), 0);
  ASSERT_EQ(hb_direction(HB_MOTOR_B), 0);
  ASSERT_EQ(hb_speed(HB_MOTOR_A), 0);
  ASSERT_EQ(hb_speed(HB_MOTOR_B), 0);
}

TEST(invalid_motor_rejected) {
  hb_init();
  ASSERT_FALSE(hb_forward(2, 100));
  ASSERT_FALSE(hb_backward(2, 100));
  ASSERT_FALSE(hb_stop(2));
  ASSERT_EQ(hb_direction(2), 0);
  ASSERT_EQ(hb_speed(2), 0);
}

TEST(max_speed_readback) {
  hb_init();
  ASSERT_TRUE(hb_forward(HB_MOTOR_A, 255));
  ASSERT_PIN(6, HIGH);
  ASSERT_EQ(hb_speed(HB_MOTOR_A), 255);
  ASSERT_TRUE(hb_backward(HB_MOTOR_B, 0));
  ASSERT_PIN(5, LOW);
  ASSERT_EQ(hb_speed(HB_MOTOR_B), 0);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
