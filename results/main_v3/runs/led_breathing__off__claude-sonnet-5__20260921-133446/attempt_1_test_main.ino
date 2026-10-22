#include "logic.h"
#include "test_runner.h"

TEST(initial_state_after_init) {
  breath_init();
  ASSERT_EQ(breath_brightness(), 0);
  ASSERT_EQ(breath_velocity(), BREATH_DEFAULT_VELOCITY);
  ASSERT_TRUE(breath_is_rising());
  ASSERT_PIN(BREATH_PIN, LOW);
}

TEST(step_advances_brightness) {
  breath_init();
  breath_step(); // brightness 0 -> 2
  ASSERT_EQ(breath_brightness(), BREATH_DEFAULT_VELOCITY);
}

TEST(set_velocity_ignores_zero) {
  breath_init();
  breath_set_velocity(7);
  ASSERT_EQ(breath_velocity(), 7);
  breath_set_velocity(0);
  ASSERT_EQ(breath_velocity(), 7);
}

TEST(bounce_at_max_flips_velocity) {
  breath_init();
  breath_set_velocity(300); // overshoot in one step
  breath_step(); // writes 0, brightness -> 300 -> clamped 255, velocity -> -300
  ASSERT_EQ(breath_brightness(), 255);
  ASSERT_TRUE(breath_velocity() < 0);
  ASSERT_FALSE(breath_is_rising());
  breath_step(); // writes current brightness (255) to pin
  ASSERT_PIN(BREATH_PIN, HIGH);
}

TEST(bounce_at_min_after_max) {
  breath_init();
  breath_set_velocity(300);
  breath_step(); // -> brightness 255, velocity -300
  breath_step(); // writes 255, brightness -> -45 -> clamped 0, velocity -> 100
  ASSERT_EQ(breath_brightness(), 0);
  ASSERT_TRUE(breath_velocity() > 0);
  ASSERT_TRUE(breath_is_rising());
  breath_step(); // writes current brightness (0) to pin
  ASSERT_PIN(BREATH_PIN, LOW);
}

TEST(reset_restores_defaults) {
  breath_init();
  breath_step();
  breath_step();
  breath_reset();
  ASSERT_EQ(breath_brightness(), 0);
  ASSERT_EQ(breath_velocity(), BREATH_DEFAULT_VELOCITY);
  ASSERT_PIN(BREATH_PIN, LOW);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
