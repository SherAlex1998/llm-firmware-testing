#include "logic.h"
#include "test_runner.h"

TEST(init_sets_default_state) {
  jm_init();
  ASSERT_TRUE(jm_direction());
  ASSERT_EQ(jm_speed(), 0);
  ASSERT_PIN(3, HIGH);
  ASSERT_PIN(4, LOW);
  ASSERT_PIN(5, LOW);
}

TEST(set_direction_clockwise_and_ccw) {
  jm_init();
  jm_set_direction(false);
  ASSERT_FALSE(jm_direction());
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, HIGH);

  jm_set_direction(true);
  ASSERT_TRUE(jm_direction());
  ASSERT_PIN(3, HIGH);
  ASSERT_PIN(4, LOW);
}

TEST(set_speed_clamps_and_reads_back_extremes) {
  jm_init();
  jm_set_speed(-10);
  ASSERT_EQ(jm_speed(), 0);
  ASSERT_PIN(5, LOW);

  jm_set_speed(255);
  ASSERT_EQ(jm_speed(), 255);
  ASSERT_PIN(5, HIGH);

  jm_set_speed(300);
  ASSERT_EQ(jm_speed(), 255);
  ASSERT_PIN(5, HIGH);

  jm_set_speed(100);
  ASSERT_EQ(jm_speed(), 100);
}

TEST(stop_zeroes_speed) {
  jm_init();
  jm_set_speed(200);
  ASSERT_EQ(jm_speed(), 200);
  jm_stop();
  ASSERT_EQ(jm_speed(), 0);
  ASSERT_PIN(5, LOW);
}

TEST(pure_axis_helpers) {
  ASSERT_EQ(jm_speed_from_axis(512), 0);
  ASSERT_EQ(jm_speed_from_axis(0), 256);
  ASSERT_EQ(jm_speed_from_axis(1023), 255);

  ASSERT_FALSE(jm_clockwise_from_axis(512));
  ASSERT_FALSE(jm_clockwise_from_axis(0));
  ASSERT_TRUE(jm_clockwise_from_axis(1023));
}

TEST(apply_axis_dead_zone_stops_motor) {
  jm_init();
  jm_set_speed(150);
  jm_apply_axis(512);
  ASSERT_EQ(jm_speed(), 0);
  ASSERT_PIN(5, LOW);

  jm_apply_axis(505);
  ASSERT_EQ(jm_speed(), 0);
}

TEST(apply_axis_sets_direction_and_speed_clockwise) {
  jm_init();
  jm_apply_axis(1023);
  ASSERT_TRUE(jm_direction());
  ASSERT_EQ(jm_speed(), 255);
  ASSERT_PIN(3, HIGH);
  ASSERT_PIN(4, LOW);
}

TEST(apply_axis_sets_direction_and_speed_ccw) {
  jm_init();
  jm_apply_axis(0);
  ASSERT_FALSE(jm_direction());
  ASSERT_EQ(jm_speed(), 256 > 255 ? 255 : 256);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, HIGH);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
