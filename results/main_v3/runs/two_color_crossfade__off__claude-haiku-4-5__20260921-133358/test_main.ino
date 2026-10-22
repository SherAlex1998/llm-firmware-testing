#include "logic.h"
#include "test_runner.h"

TEST(init_sets_phase_and_value) {
  xf_init();
  ASSERT_EQ(xf_phase(), 0);
  ASSERT_EQ(xf_value(), 0);
  ASSERT_EQ(xf_red_level(), 150);
  ASSERT_EQ(xf_green_level(), 0);
}

TEST(apply_clamps_negative) {
  xf_init();
  xf_apply(-10);
  ASSERT_EQ(xf_value(), 0);
  ASSERT_EQ(xf_red_level(), 150);
  ASSERT_EQ(xf_green_level(), 0);
}

TEST(apply_clamps_above_max) {
  xf_init();
  xf_apply(200);
  ASSERT_EQ(xf_value(), 150);
  ASSERT_EQ(xf_red_level(), 0);
  ASSERT_EQ(xf_green_level(), 150);
}

TEST(phase_0_crossfade) {
  xf_init();
  ASSERT_EQ(xf_phase(), 0);
  xf_apply(50);
  ASSERT_EQ(xf_value(), 50);
  ASSERT_EQ(xf_red_level(), 100);
  ASSERT_EQ(xf_green_level(), 50);
  xf_apply(150);
  ASSERT_EQ(xf_red_level(), 0);
  ASSERT_EQ(xf_green_level(), 150);
}

TEST(phase_1_crossfade) {
  xf_init();
  for (int i = 0; i < 151; i++) {
    xf_step();
  }
  ASSERT_EQ(xf_phase(), 1);
  xf_apply(75);
  ASSERT_EQ(xf_phase(), 1);
  ASSERT_EQ(xf_value(), 75);
  ASSERT_EQ(xf_red_level(), 75);
  ASSERT_EQ(xf_green_level(), 75);
}

TEST(step_increments_and_wraps) {
  xf_init();
  xf_apply(0);
  ASSERT_EQ(xf_phase(), 0);
  for (int i = 0; i < 150; i++) {
    xf_step();
  }
  ASSERT_EQ(xf_value(), 150);
  ASSERT_EQ(xf_phase(), 0);
  xf_step();
  ASSERT_EQ(xf_value(), 0);
  ASSERT_EQ(xf_phase(), 1);
}

TEST(step_phase_alternates) {
  xf_init();
  ASSERT_EQ(xf_phase(), 0);
  for (int i = 0; i < 151; i++) {
    xf_step();
  }
  ASSERT_EQ(xf_phase(), 1);
  for (int i = 0; i < 151; i++) {
    xf_step();
  }
  ASSERT_EQ(xf_phase(), 0);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
