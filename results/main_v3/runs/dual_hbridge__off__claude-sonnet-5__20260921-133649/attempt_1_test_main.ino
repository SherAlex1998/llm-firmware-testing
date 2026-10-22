#include "logic.h"
#include "test_runner.h"

TEST(init_stops_all) {
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

TEST(motor_a_forward) {
  hb_init();
  ASSERT_TRUE(hb_forward(HB_MOTOR_A, 255));
  ASSERT_PIN(HB_PIN_IN1, HIGH);
  ASSERT_PIN(HB_PIN_IN2, LOW);
  ASSERT_PIN(HB_PIN_ENA, HIGH);
  ASSERT_EQ(hb_direction(HB_MOTOR_A), 1);
  ASSERT_EQ(hb_speed(HB_MOTOR_A), 255);
}

TEST(motor_a_backward) {
  hb_init();
  ASSERT_TRUE(hb_backward(HB_MOTOR_A, 200));
  ASSERT_PIN(HB_PIN_IN1, LOW);
  ASSERT_PIN(HB_PIN_IN2, HIGH);
  ASSERT_EQ(hb_direction(HB_MOTOR_A), -1);
  ASSERT_EQ(hb_speed(HB_MOTOR_A), 200);
}

TEST(motor_b_forward_and_stop) {
  hb_init();
  ASSERT_TRUE(hb_forward(HB_MOTOR_B, 128));
  ASSERT_PIN(HB_PIN_IN3, HIGH);
  ASSERT_PIN(HB_PIN_IN4, LOW);
  ASSERT_EQ(hb_direction(HB_MOTOR_B), 1);
  ASSERT_EQ(hb_speed(HB_MOTOR_B), 128);

  ASSERT_TRUE(hb_stop(HB_MOTOR_B));
  ASSERT_PIN(HB_PIN_IN3, LOW);
  ASSERT_PIN(HB_PIN_IN4, LOW);
  ASSERT_PIN(HB_PIN_ENB, LOW);
  ASSERT_EQ(hb_direction(HB_MOTOR_B), 0);
  ASSERT_EQ(hb_speed(HB_MOTOR_B), 0);
}

TEST(invalid_motor_returns_false) {
  hb_init();
  ASSERT_FALSE(hb_forward(2, 100));
  ASSERT_FALSE(hb_backward(2, 100));
  ASSERT_FALSE(hb_stop(2));
  ASSERT_EQ(hb_direction(2), 0);
  ASSERT_EQ(hb_speed(2), 0);
}

TEST(stop_all_stops_both_motors) {
  hb_init();
  hb_forward(HB_MOTOR_A, 150);
  hb_backward(HB_MOTOR_B, 100);
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

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
