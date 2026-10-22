#include "logic.h"
#include "test_runner.h"

TEST(init_sets_pin_and_brightness) {
  breath_init();
  ASSERT_EQ(breath_brightness(), 0);
  ASSERT_EQ(breath_velocity(), 2);
  ASSERT_PIN(11, LOW);
}

TEST(reset_clears_state) {
  breath_init();
  breath_set_velocity(5);
  breath_brightness();
  breath_reset();
  ASSERT_EQ(breath_brightness(), 0);
  ASSERT_EQ(breath_velocity(), 2);
  ASSERT_PIN(11, LOW);
}

TEST(step_advances_brightness_and_writes_pin) {
  breath_init();
  breath_step();
  ASSERT_EQ(breath_brightness(), 2);
  ASSERT_PIN(11, HIGH);
  breath_step();
  ASSERT_EQ(breath_brightness(), 4);
}

TEST(step_bounces_at_max) {
  breath_init();
  breath_set_velocity(100);
  breath_step();
  ASSERT_EQ(breath_brightness(), 100);
  ASSERT_TRUE(breath_is_rising());
  breath_step();
  ASSERT_EQ(breath_brightness(), 255);
  ASSERT_FALSE(breath_is_rising());
}

TEST(step_bounces_at_min) {
  breath_init();
  breath_set_velocity(-50);
  for (int i = 0; i < 10; i++) {
    breath_step();
  }
  ASSERT_EQ(breath_brightness(), 0);
  ASSERT_TRUE(breath_is_rising());
}

TEST(set_velocity_changes_step_size) {
  breath_init();
  breath_set_velocity(10);
  ASSERT_EQ(breath_velocity(), 10);
  breath_step();
  ASSERT_EQ(breath_brightness(), 10);
  breath_set_velocity(-5);
  ASSERT_EQ(breath_velocity(), -5);
}

TEST(set_velocity_ignores_zero) {
  breath_init();
  breath_set_velocity(7);
  breath_set_velocity(0);
  ASSERT_EQ(breath_velocity(), 7);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
