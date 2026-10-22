#include "logic.h"
#include "test_runner.h"

TEST(init_sets_pins_and_direction) {
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

TEST(set_direction_counterclockwise) {
  jm_init();
  jm_set_direction(false);
  ASSERT_PIN(JM_PIN_DIR_A, LOW);
  ASSERT_PIN(JM_PIN_DIR_B, HIGH);
  ASSERT_FALSE(jm_direction());
}

TEST(set_speed_clamped_and_read_back) {
  jm_init();
  jm_set_speed(100);
  ASSERT_EQ(jm_speed(), 100);
  jm_set_speed(255);
  ASSERT_EQ(jm_speed(), 255);
  jm_set_speed(-10);
  ASSERT_EQ(jm_speed(), 0);
  jm_set_speed(300);
  ASSERT_EQ(jm_speed(), 255);
}

TEST(stop_sets_speed_zero) {
  jm_init();
  jm_set_speed(200);
  jm_stop();
  ASSERT_EQ(jm_speed(), 0);
  ASSERT_PIN(JM_PIN_SPEED, LOW);
}

TEST(speed_from_axis_computation) {
  jm_init();
  int speed_center = jm_speed_from_axis(512);
  ASSERT_EQ(speed_center, 0);
  int speed_right = jm_speed_from_axis(612);
  ASSERT_EQ(speed_right, 50);
  int speed_left = jm_speed_from_axis(412);
  ASSERT_EQ(speed_left, 50);
  int speed_max_right = jm_speed_from_axis(1023);
  ASSERT_EQ(speed_max_right, 255);
  int speed_max_left = jm_speed_from_axis(0);
  ASSERT_EQ(speed_max_left, 255);
}

TEST(clockwise_from_axis_computation) {
  jm_init();
  ASSERT_FALSE(jm_clockwise_from_axis(512));
  ASSERT_FALSE(jm_clockwise_from_axis(400));
  ASSERT_TRUE(jm_clockwise_from_axis(600));
  ASSERT_TRUE(jm_clockwise_from_axis(1023));
}

TEST(apply_axis_with_dead_zone) {
  jm_init();
  jm_apply_axis(512);
  ASSERT_EQ(jm_speed(), 0);
  jm_apply_axis(520);
  ASSERT_EQ(jm_speed(), 0);
  jm_apply_axis(532);
  ASSERT_EQ(jm_speed(), 10);
  ASSERT_TRUE(jm_direction());
}

TEST(apply_axis_direction_and_speed) {
  jm_init();
  jm_apply_axis(712);
  ASSERT_TRUE(jm_direction());
  ASSERT_PIN(JM_PIN_DIR_A, HIGH);
  ASSERT_PIN(JM_PIN_DIR_B, LOW);
  ASSERT_EQ(jm_speed(), 100);
  jm_apply_axis(312);
  ASSERT_FALSE(jm_direction());
  ASSERT_PIN(JM_PIN_DIR_A, LOW);
  ASSERT_PIN(JM_PIN_DIR_B, HIGH);
  ASSERT_EQ(jm_speed(), 100);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
