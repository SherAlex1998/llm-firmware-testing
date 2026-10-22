#include "logic.h"
#include "test_runner.h"

TEST(init_defaults) {
  jm_init();
  ASSERT_TRUE(jm_direction());
  ASSERT_EQ(jm_speed(), 0);
  ASSERT_PIN(JM_PIN_DIR_A, HIGH);
  ASSERT_PIN(JM_PIN_DIR_B, LOW);
  ASSERT_PIN(JM_PIN_SPEED, LOW);
}

TEST(set_direction_counterclockwise) {
  jm_init();
  jm_set_direction(false);
  ASSERT_FALSE(jm_direction());
  ASSERT_PIN(JM_PIN_DIR_A, LOW);
  ASSERT_PIN(JM_PIN_DIR_B, HIGH);
}

TEST(set_speed_clamped) {
  jm_init();
  jm_set_speed(-10);
  ASSERT_EQ(jm_speed(), 0);
  jm_set_speed(300);
  ASSERT_EQ(jm_speed(), 255);
  ASSERT_PIN(JM_PIN_SPEED, HIGH);
  jm_set_speed(128);
  ASSERT_EQ(jm_speed(), 128);
}

TEST(stop_zeroes_speed) {
  jm_init();
  jm_set_speed(200);
  ASSERT_EQ(jm_speed(), 200);
  jm_stop();
  ASSERT_EQ(jm_speed(), 0);
  ASSERT_PIN(JM_PIN_SPEED, LOW);
}

TEST(pure_speed_and_direction_from_axis) {
  ASSERT_EQ(jm_speed_from_axis(512), 0);
  ASSERT_EQ(jm_speed_from_axis(0), 256);
  ASSERT_EQ(jm_speed_from_axis(1023), 255);
  ASSERT_TRUE(jm_clockwise_from_axis(600));
  ASSERT_FALSE(jm_clockwise_from_axis(400));
  ASSERT_FALSE(jm_clockwise_from_axis(512));
}

TEST(apply_axis_dead_zone_stops) {
  jm_init();
  jm_set_speed(150);
  jm_apply_axis(515); // |515-512|/2 = 1 < dead zone
  ASSERT_EQ(jm_speed(), 0);
  ASSERT_PIN(JM_PIN_SPEED, LOW);
}

TEST(apply_axis_positive_sets_clockwise) {
  jm_init();
  jm_apply_axis(700); // speed = |700-512|/2 = 94, clockwise
  ASSERT_TRUE(jm_direction());
  ASSERT_EQ(jm_speed(), 94);
  ASSERT_PIN(JM_PIN_DIR_A, HIGH);
  ASSERT_PIN(JM_PIN_DIR_B, LOW);
}

TEST(apply_axis_negative_sets_counterclockwise) {
  jm_init();
  jm_apply_axis(200); // speed = |200-512|/2 = 156, counterclockwise
  ASSERT_FALSE(jm_direction());
  ASSERT_EQ(jm_speed(), 156);
  ASSERT_PIN(JM_PIN_DIR_A, LOW);
  ASSERT_PIN(JM_PIN_DIR_B, HIGH);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
