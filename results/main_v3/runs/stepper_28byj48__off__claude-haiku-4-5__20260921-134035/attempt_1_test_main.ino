#include "logic.h"
#include "test_runner.h"

TEST(init_state) {
  stepper_init();
  ASSERT_EQ(stepper_phase(), 0);
  ASSERT_EQ(stepper_position(), 0);
  ASSERT_EQ(stepper_delay(), STEPPER_DEFAULT_DELAY_MS);
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(11, LOW);
}

TEST(coil_pattern_valid) {
  stepper_init();
  ASSERT_EQ(stepper_coil_pattern(0), B1000);
  ASSERT_EQ(stepper_coil_pattern(1), B1100);
  ASSERT_EQ(stepper_coil_pattern(2), B0100);
  ASSERT_EQ(stepper_coil_pattern(3), B0110);
  ASSERT_EQ(stepper_coil_pattern(4), B0010);
  ASSERT_EQ(stepper_coil_pattern(5), B0011);
  ASSERT_EQ(stepper_coil_pattern(6), B0001);
  ASSERT_EQ(stepper_coil_pattern(7), B1001);
}

TEST(coil_pattern_invalid) {
  stepper_init();
  ASSERT_EQ(stepper_coil_pattern(8), 0);
  ASSERT_EQ(stepper_coil_pattern(255), 0);
}

TEST(apply_phase_valid) {
  stepper_init();
  ASSERT_TRUE(stepper_apply_phase(1));
  ASSERT_EQ(stepper_phase(), 1);
  ASSERT_PIN(8, HIGH);
  ASSERT_PIN(9, HIGH);
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(11, LOW);
}

TEST(apply_phase_invalid) {
  stepper_init();
  ASSERT_FALSE(stepper_apply_phase(8));
  ASSERT_FALSE(stepper_apply_phase(255));
  ASSERT_EQ(stepper_phase(), 0);
}

TEST(step_half_forward) {
  stepper_init();
  stepper_set_delay(0);
  stepper_step_half(3);
  ASSERT_EQ(stepper_phase(), 3);
  ASSERT_EQ(stepper_position(), 3);
}

TEST(step_half_backward) {
  stepper_init();
  stepper_set_delay(0);
  stepper_step_half(5);
  ASSERT_EQ(stepper_position(), 5);
  stepper_step_half(-3);
  ASSERT_EQ(stepper_phase(), 2);
  ASSERT_EQ(stepper_position(), 2);
}

TEST(step_full_forward) {
  stepper_init();
  stepper_set_delay(0);
  stepper_step_full(2);
  ASSERT_EQ(stepper_position(), 4);
  ASSERT_TRUE(stepper_phase() % 2 == 1);
}

TEST(step_full_backward) {
  stepper_init();
  stepper_set_delay(0);
  stepper_step_full(3);
  ASSERT_EQ(stepper_position(), 6);
  stepper_step_full(-2);
  ASSERT_EQ(stepper_position(), 2);
  ASSERT_TRUE(stepper_phase() % 2 == 1);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {
}
