#include "logic.h"
#include "test_runner.h"

TEST(init_sets_default_fade_state) {
  xf_init();
  ASSERT_EQ(xf_value(), 0);
  ASSERT_EQ(xf_phase(), 0);
  ASSERT_EQ(xf_red_level(), XF_MAX_LEVEL);
  ASSERT_EQ(xf_green_level(), 0);
  ASSERT_PIN(XF_PIN_GREEN, LOW);
}

TEST(apply_sets_phase_zero_levels) {
  xf_init();
  xf_apply(60);
  ASSERT_EQ(xf_value(), 60);
  ASSERT_EQ(xf_phase(), 0);
  ASSERT_EQ(xf_red_level(), 90);
  ASSERT_EQ(xf_green_level(), 60);
}

TEST(apply_clamps_to_valid_range) {
  xf_init();
  xf_apply(-25);
  ASSERT_EQ(xf_value(), 0);
  ASSERT_EQ(xf_red_level(), XF_MAX_LEVEL);
  ASSERT_EQ(xf_green_level(), 0);
  ASSERT_PIN(XF_PIN_GREEN, LOW);

  xf_apply(XF_MAX_LEVEL + 25);
  ASSERT_EQ(xf_value(), XF_MAX_LEVEL);
  ASSERT_EQ(xf_red_level(), 0);
  ASSERT_EQ(xf_green_level(), XF_MAX_LEVEL);
  ASSERT_PIN(XF_PIN_RED, LOW);
}

TEST(step_advances_value_without_phase_change) {
  xf_init();
  xf_step();
  ASSERT_EQ(xf_value(), 1);
  ASSERT_EQ(xf_phase(), 0);
  ASSERT_EQ(xf_red_level(), XF_MAX_LEVEL - 1);
  ASSERT_EQ(xf_green_level(), 1);
}

TEST(step_wraps_and_switches_to_phase_one) {
  xf_init();
  xf_apply(XF_MAX_LEVEL);
  xf_step();
  ASSERT_EQ(xf_value(), 0);
  ASSERT_EQ(xf_phase(), 1);
  ASSERT_EQ(xf_red_level(), 0);
  ASSERT_EQ(xf_green_level(), XF_MAX_LEVEL);
  ASSERT_PIN(XF_PIN_RED, LOW);
}

TEST(phase_one_swaps_channel_levels) {
  xf_init();
  xf_apply(XF_MAX_LEVEL);
  xf_step();
  xf_apply(40);
  ASSERT_EQ(xf_value(), 40);
  ASSERT_EQ(xf_phase(), 1);
  ASSERT_EQ(xf_red_level(), 40);
  ASSERT_EQ(xf_green_level(), XF_MAX_LEVEL - 40);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
