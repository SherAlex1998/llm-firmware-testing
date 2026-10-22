#include "logic.h"
#include "test_runner.h"

TEST(init_sets_output) {
  breath_init();
  ASSERT_PIN(11, LOW);
  ASSERT_EQ(breath_brightness(), 0);
  ASSERT_EQ(breath_velocity(), 2);
}

TEST(reset_returns_to_min) {
  breath_init();
  breath_step();
  breath_step();
  breath_reset();
  ASSERT_EQ(breath_brightness(), 0);
  ASSERT_EQ(breath_velocity(), 2);
  ASSERT_PIN(11, LOW);
}

TEST(step_increments_brightness) {
  breath_init();
  breath_step();
  ASSERT_EQ(breath_brightness(), 2);
  breath_step();
  ASSERT_EQ(breath_brightness(), 4);
  breath_step();
  ASSERT_EQ(breath_brightness(), 6);
}

TEST(step_bounces_at_max) {
  breath_init();
  breath_set_velocity(100);
  breath_step();
  breath_step();
  breath_step();
  ASSERT_EQ(breath_brightness(), 255);
  ASSERT_TRUE(breath_is_rising());
  breath_step();
  ASSERT_EQ(breath_brightness(), 255);
  ASSERT_FALSE(breath_is_rising());
}

TEST(step_bounces_at_min) {
  breath_init();
  breath_set_velocity(-50);
  breath_step();
  ASSERT_EQ(breath_brightness(), 0);
  ASSERT_FALSE(breath_is_rising());
  breath_step();
  ASSERT_EQ(breath_brightness(), 0);
  ASSERT_TRUE(breath_is_rising());
}

TEST(set_velocity_changes_step_size) {
  breath_init();
  ASSERT_EQ(breath_velocity(), 2);
  breath_set_velocity(5);
  ASSERT_EQ(breath_velocity(), 5);
  breath_step();
  ASSERT_EQ(breath_brightness(), 5);
  breath_step();
  ASSERT_EQ(breath_brightness(), 10);
}

TEST(set_velocity_zero_ignored) {
  breath_init();
  breath_set_velocity(3);
  breath_set_velocity(0);
  ASSERT_EQ(breath_velocity(), 3);
  breath_step();
  ASSERT_EQ(breath_brightness(), 3);
}

TEST(full_breathing_cycle) {
  breath_init();
  breath_set_velocity(50);
  ASSERT_TRUE(breath_is_rising());
  breath_step();
  breath_step();
  breath_step();
  ASSERT_EQ(breath_brightness(), 255);
  ASSERT_TRUE(breath_is_rising());
  breath_step();
  ASSERT_FALSE(breath_is_rising());
  breath_step();
  ASSERT_EQ(breath_brightness(), 155);
  ASSERT_FALSE(breath_is_rising());
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
