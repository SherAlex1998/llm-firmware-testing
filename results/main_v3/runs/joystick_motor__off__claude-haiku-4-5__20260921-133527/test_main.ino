#include "logic.h"
#include "test_runner.h"

TEST(init_sets_pins_and_defaults) {
  jm_init();
  ASSERT_PIN(JM_PIN_DIR_A, HIGH);
  ASSERT_PIN(JM_PIN_DIR_B, LOW);
  ASSERT_EQ(jm_speed(), 0);
  ASSERT_TRUE(jm_direction());
}

TEST(set_direction_clockwise) {
  jm_init();
  jm_set_direction(true);
  ASSERT_PIN(JM_PIN_DIR_A, HIGH);
  ASSERT_PIN(JM_PIN_DIR_B, LOW);
  ASSERT_TRUE(jm_direction());
}

TEST(set_direction_counter_clockwise) {
  jm_init();
  jm_set_direction(false);
  ASSERT_PIN(JM_PIN_DIR_A, LOW);
  ASSERT_PIN(JM_PIN_DIR_B, HIGH);
  ASSERT_FALSE(jm_direction());
}

TEST(speed_clamping) {
  jm_init();
  jm_set_speed(-10);
  ASSERT_EQ(jm_speed(), 0);
  jm_set_speed(300);
  ASSERT_EQ(jm_speed(), 255);
  jm_set_speed(128);
  ASSERT_EQ(jm_speed(), 128);
}

TEST(speed_from_axis) {
  jm_init();
  int speed_center = jm_speed_from_axis(512);
  ASSERT_EQ(speed_center, 0);
  int speed_right = jm_speed_from_axis(612);
  ASSERT_EQ(speed_right, 50);
  int speed_left = jm_speed_from_axis(412);
  ASSERT_EQ(speed_left, 50);
  int speed_max = jm_speed_from_axis(0);
  ASSERT_EQ(speed_max, 256);
}

TEST(clockwise_from_axis) {
  jm_init();
  ASSERT_TRUE(jm_clockwise_from_axis(513));
  ASSERT_FALSE(jm_clockwise_from_axis(511));
  ASSERT_FALSE(jm_clockwise_from_axis(512));
}

TEST(apply_axis_with_deadzone) {
  jm_init();
  jm_apply_axis(512);
  ASSERT_EQ(jm_speed(), 0);
  jm_apply_axis(515);
  ASSERT_EQ(jm_speed(), 0);
  jm_apply_axis(532);
  ASSERT_EQ(jm_speed(), 10);
  ASSERT_TRUE(jm_direction());
}

TEST(apply_axis_direction_change) {
  jm_init();
  jm_apply_axis(600);
  ASSERT_TRUE(jm_direction());
  ASSERT_EQ(jm_speed(), 44);
  jm_apply_axis(400);
  ASSERT_FALSE(jm_direction());
  ASSERT_EQ(jm_speed(), 44);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
