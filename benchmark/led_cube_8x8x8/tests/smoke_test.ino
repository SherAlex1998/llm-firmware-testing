// Hand-written infrastructure check (not shown to the LLM): SPI on avr8js, voxel helpers, effect rotation.
#include "logic.h"
#include "test_runner.h"

TEST(voxel_set_get_clear) {
  cube_set_delay_scale(0);
  cube_init();
  setVoxel(1, 2, 3);
  ASSERT_TRUE(getVoxel(1, 2, 3));
  ASSERT_EQ(cube[5][4], 0x02);
  clearVoxel(1, 2, 3);
  ASSERT_FALSE(getVoxel(1, 2, 3));
}

TEST(button_switches_effect) {
  cube_set_delay_scale(0);
  cube_init();
  cube_loop_with(100, false);
  ASSERT_EQ(currentEffect, RAIN);
  cube_loop_with(200, true);
  ASSERT_EQ(currentEffect, PLANE_BOING);
}

TEST(auto_rotation_after_effect_duration) {
  cube_set_delay_scale(0);
  cube_init();
  cube_loop_with(5001, false);
  ASSERT_EQ(currentEffect, PLANE_BOING);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
