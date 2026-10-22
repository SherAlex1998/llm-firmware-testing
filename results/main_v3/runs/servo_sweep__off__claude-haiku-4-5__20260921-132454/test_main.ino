#include "logic.h"
#include "test_runner.h"

TEST(clamp_angle_low) {
  ASSERT_EQ(sweep_clamp_angle(-10), 0);
  ASSERT_EQ(sweep_clamp_angle(0), 0);
}

TEST(clamp_angle_mid) {
  ASSERT_EQ(sweep_clamp_angle(90), 90);
  ASSERT_EQ(sweep_clamp_angle(45), 45);
}

TEST(clamp_angle_high) {
  ASSERT_EQ(sweep_clamp_angle(180), 180);
  ASSERT_EQ(sweep_clamp_angle(200), 180);
}

TEST(init_and_write) {
  sweep_init();
  ASSERT_EQ(sweep_angle(), SWEEP_START);
  ASSERT_EQ(sweep_write_count(), 1);
}

TEST(write_increments_count) {
  sweep_init();
  sweep_write(90);
  ASSERT_EQ(sweep_write_count(), 2);
  sweep_write(120);
  ASSERT_EQ(sweep_write_count(), 3);
}

TEST(write_clamps) {
  sweep_init();
  sweep_write(200);
  ASSERT_EQ(sweep_angle(), 180);
  sweep_write(-50);
  ASSERT_EQ(sweep_angle(), 0);
}

TEST(steps_per_cycle_valid) {
  unsigned int steps = sweep_steps_per_cycle(0, 10, 5);
  ASSERT_EQ(steps, 6);
  steps = sweep_steps_per_cycle(100, 120, 10);
  ASSERT_EQ(steps, 6);
}

TEST(steps_per_cycle_invalid) {
  ASSERT_EQ(sweep_steps_per_cycle(100, 90, 10), 0);
  ASSERT_EQ(sweep_steps_per_cycle(50, 100, 0), 0);
  ASSERT_EQ(sweep_steps_per_cycle(50, 100, -5), 0);
}

TEST(sweep_run_one_cycle) {
  sweep_init();
  sweep_run(100, 110, 5, 1, 0);
  unsigned long count = sweep_write_count();
  ASSERT_EQ(sweep_angle(), 100);
  ASSERT_TRUE(count > 1);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
