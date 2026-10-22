#include "logic.h"
#include "test_runner.h"

TEST(init_sets_default_state_and_output_low) {
  breath_init();

  ASSERT_EQ(breath_brightness(), 0);
  ASSERT_EQ(breath_velocity(), 2);
  ASSERT_TRUE(breath_is_rising());
  ASSERT_PIN(11, LOW);
}

TEST(reset_restores_defaults_after_changes) {
  breath_init();
  breath_set_velocity(-17);
  breath_step();
  breath_reset();

  ASSERT_EQ(breath_brightness(), 0);
  ASSERT_EQ(breath_velocity(), 2);
  ASSERT_TRUE(breath_is_rising());
  ASSERT_PIN(11, LOW);
}

TEST(velocity_setting_accepts_nonzero_and_ignores_zero) {
  breath_init();

  breath_set_velocity(7);
  ASSERT_EQ(breath_velocity(), 7);
  ASSERT_TRUE(breath_is_rising());

  breath_set_velocity(0);
  ASSERT_EQ(breath_velocity(), 7);

  breath_set_velocity(-9);
  ASSERT_EQ(breath_velocity(), -9);
  ASSERT_FALSE(breath_is_rising());
}

TEST(step_writes_current_brightness_then_advances) {
  breath_init();

  breath_step();

  ASSERT_PIN(11, LOW);
  ASSERT_EQ(breath_brightness(), 2);
  ASSERT_EQ(breath_velocity(), 2);
  ASSERT_TRUE(breath_is_rising());
}

TEST(step_bounces_at_maximum_and_minimum) {
  breath_init();
  breath_set_velocity(255);

  breath_step();
  ASSERT_EQ(breath_brightness(), 255);
  ASSERT_EQ(breath_velocity(), -255);
  ASSERT_FALSE(breath_is_rising());

  breath_step();
  ASSERT_PIN(11, HIGH);
  ASSERT_EQ(breath_brightness(), 0);
  ASSERT_EQ(breath_velocity(), 255);
  ASSERT_TRUE(breath_is_rising());

  breath_step();
  ASSERT_PIN(11, LOW);
  ASSERT_EQ(breath_brightness(), 255);
  ASSERT_EQ(breath_velocity(), -255);
}

TEST(step_clamps_overshoot_at_maximum) {
  breath_init();
  breath_set_velocity(200);

  breath_step();
  ASSERT_EQ(breath_brightness(), 200);
  ASSERT_EQ(breath_velocity(), 200);

  breath_step();
  ASSERT_EQ(breath_brightness(), 255);
  ASSERT_EQ(breath_velocity(), -200);
  ASSERT_FALSE(breath_is_rising());

  breath_step();
  ASSERT_PIN(11, HIGH);
  ASSERT_EQ(breath_brightness(), 55);
  ASSERT_EQ(breath_velocity(), -200);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
