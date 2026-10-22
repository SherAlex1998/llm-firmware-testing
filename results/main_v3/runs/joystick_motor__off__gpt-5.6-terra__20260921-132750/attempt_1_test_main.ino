#include "logic.h"
#include "test_runner.h"

TEST(init_sets_clockwise_and_stops) {
  jm_init();
  ASSERT_TRUE(jm_direction());
  ASSERT_EQ(jm_speed(), 0);
  ASSERT_PIN(JM_PIN_DIR_A, HIGH);
  ASSERT_PIN(JM_PIN_DIR_B, LOW);
  ASSERT_PIN(JM_PIN_SPEED, LOW);
}

TEST(direction_changes_output_pins) {
  jm_init();

  jm_set_direction(false);
  ASSERT_FALSE(jm_direction());
  ASSERT_PIN(JM_PIN_DIR_A, LOW);
  ASSERT_PIN(JM_PIN_DIR_B, HIGH);

  jm_set_direction(true);
  ASSERT_TRUE(jm_direction());
  ASSERT_PIN(JM_PIN_DIR_A, HIGH);
  ASSERT_PIN(JM_PIN_DIR_B, LOW);
}

TEST(speed_is_clamped_and_stop_sets_zero) {
  jm_init();

  jm_set_speed(-20);
  ASSERT_EQ(jm_speed(), 0);
  ASSERT_PIN(JM_PIN_SPEED, LOW);

  jm_set_speed(300);
  ASSERT_EQ(jm_speed(), 255);
  ASSERT_PIN(JM_PIN_SPEED, HIGH);

  jm_set_speed(123);
  ASSERT_EQ(jm_speed(), 123);

  jm_stop();
  ASSERT_EQ(jm_speed(), 0);
  ASSERT_PIN(JM_PIN_SPEED, LOW);
}

TEST(axis_conversion_maps_speed_and_direction) {
  jm_init();

  ASSERT_EQ(jm_speed_from_axis(512), 0);
  ASSERT_EQ(jm_speed_from_axis(532), 10);
  ASSERT_EQ(jm_speed_from_axis(492), 10);
  ASSERT_EQ(jm_speed_from_axis(0), 256);
  ASSERT_EQ(jm_speed_from_axis(1023), 255);

  ASSERT_FALSE(jm_clockwise_from_axis(512));
  ASSERT_FALSE(jm_clockwise_from_axis(511));
  ASSERT_TRUE(jm_clockwise_from_axis(513));
}

TEST(apply_axis_stops_in_dead_zone) {
  jm_init();
  jm_set_direction(false);

  jm_apply_axis(530);
  ASSERT_EQ(jm_speed(), 0);
  ASSERT_FALSE(jm_direction());
  ASSERT_PIN(JM_PIN_SPEED, LOW);
  ASSERT_PIN(JM_PIN_DIR_A, LOW);
  ASSERT_PIN(JM_PIN_DIR_B, HIGH);
}

TEST(apply_axis_sets_direction_and_speed_outside_dead_zone) {
  jm_init();

  jm_apply_axis(700);
  ASSERT_EQ(jm_speed(), 94);
  ASSERT_TRUE(jm_direction());
  ASSERT_PIN(JM_PIN_DIR_A, HIGH);
  ASSERT_PIN(JM_PIN_DIR_B, LOW);

  jm_apply_axis(300);
  ASSERT_EQ(jm_speed(), 106);
  ASSERT_FALSE(jm_direction());
  ASSERT_PIN(JM_PIN_DIR_A, LOW);
  ASSERT_PIN(JM_PIN_DIR_B, HIGH);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
