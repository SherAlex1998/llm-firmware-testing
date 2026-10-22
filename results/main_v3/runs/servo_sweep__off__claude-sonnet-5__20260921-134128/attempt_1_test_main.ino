#include "logic.h"
#include "test_runner.h"

TEST(init_moves_to_start) {
  sweep_init();
  ASSERT_EQ(sweep_angle(), SWEEP_START);
  ASSERT_EQ(sweep_write_count(), 1);
}

TEST(clamp_angle_bounds) {
  ASSERT_EQ(sweep_clamp_angle(-10), 0);
  ASSERT_EQ(sweep_clamp_angle(200), 180);
  ASSERT_EQ(sweep_clamp_angle(90), 90);
}

TEST(write_clamps_and_counts) {
  sweep_init();
  unsigned long before = sweep_write_count();
  sweep_write(300);
  ASSERT_EQ(sweep_angle(), 180);
  ASSERT_EQ(sweep_write_count(), before + 1);
}

TEST(steps_per_cycle_valid) {
  ASSERT_EQ(sweep_steps_per_cycle(0, 10, 5), 6);
  ASSERT_EQ(sweep_steps_per_cycle(0, 9, 5), 6);
}

TEST(steps_per_cycle_invalid) {
  ASSERT_EQ(sweep_steps_per_cycle(10, 0, 5), 0);
  ASSERT_EQ(sweep_steps_per_cycle(0, 10, 0), 0);
  ASSERT_EQ(sweep_steps_per_cycle(0, 10, -1), 0);
}

TEST(run_executes_full_cycle) {
  sweep_init();
  unsigned long before = sweep_write_count();
  sweep_run(20, 30, 10, 1, 0);
  unsigned long expected = sweep_steps_per_cycle(20, 30, 10);
  ASSERT_EQ(sweep_write_count() - before, expected);
  ASSERT_EQ(sweep_angle(), 20);
}

TEST(run_invalid_args_no_writes) {
  sweep_init();
  unsigned long before = sweep_write_count();
  sweep_run(30, 20, 5, 1, 0);
  ASSERT_EQ(sweep_write_count(), before);
}

TEST(pin_reflects_last_write) {
  sweep_init();
  sweep_write(180);
  ASSERT_PIN(9, HIGH);
  sweep_write(0);
  ASSERT_PIN(9, LOW);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
