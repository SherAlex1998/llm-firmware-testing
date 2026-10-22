#include "logic.h"
#include "test_runner.h"

TEST(init_sets_pins_and_state) {
  stepper_init();
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(11, LOW);
  ASSERT_EQ(stepper_phase(), 0);
  ASSERT_EQ(stepper_position(), 0);
  ASSERT_EQ(stepper_delay(), STEPPER_DEFAULT_DELAY_MS);
}

TEST(coil_pattern_valid_phases) {
  stepper_init();
  // Test all 8 valid phases
  ASSERT_EQ(stepper_coil_pattern(0), B1000);
  ASSERT_EQ(stepper_coil_pattern(1), B1100);
  ASSERT_EQ(stepper_coil_pattern(2), B0100);
  ASSERT_EQ(stepper_coil_pattern(3), B0110);
  ASSERT_EQ(stepper_coil_pattern(4), B0010);
  ASSERT_EQ(stepper_coil_pattern(5), B0011);
  ASSERT_EQ(stepper_coil_pattern(6), B0001);
  ASSERT_EQ(stepper_coil_pattern(7), B1001);
}

TEST(coil_pattern_invalid_phase) {
  stepper_init();
  // Invalid phases should return 0
  ASSERT_EQ(stepper_coil_pattern(8), 0);
  ASSERT_EQ(stepper_coil_pattern(255), 0);
}

TEST(apply_phase_valid) {
  stepper_init();
  // Apply phase 3 (pattern B0110)
  ASSERT_TRUE(stepper_apply_phase(3));
  ASSERT_EQ(stepper_phase(), 3);
  ASSERT_PIN(8, LOW);   // bit0 = 0
  ASSERT_PIN(9, HIGH);  // bit1 = 1
  ASSERT_PIN(10, HIGH); // bit2 = 1
  ASSERT_PIN(11, LOW);  // bit3 = 0
}

TEST(apply_phase_invalid) {
  stepper_init();
  // Invalid phase should return false and not change state
  ASSERT_FALSE(stepper_apply_phase(8));
  ASSERT_EQ(stepper_phase(), 0);
}

TEST(step_full_forward) {
  stepper_init();
  // Full step forward: moves through odd phases (1, 3, 5, 7, 1, ...)
  stepper_step_full(1);
  ASSERT_EQ(stepper_phase(), 1);
  ASSERT_EQ(stepper_position(), 2);  // 1 full step = 2 half-steps
  
  stepper_step_full(1);
  ASSERT_EQ(stepper_phase(), 3);
  ASSERT_EQ(stepper_position(), 4);
  
  stepper_step_full(2);
  ASSERT_EQ(stepper_phase(), 7);
  ASSERT_EQ(stepper_position(), 8);
  
  // Test wrap-around
  stepper_step_full(1);
  ASSERT_EQ(stepper_phase(), 1);
  ASSERT_EQ(stepper_position(), 10);
}

TEST(step_full_backward) {
  stepper_init();
  // Go forward first to a known state
  stepper_step_full(2);
  ASSERT_EQ(stepper_phase(), 3);
  ASSERT_EQ(stepper_position(), 4);
  
  // Step backward
  stepper_step_full(-1);
  ASSERT_EQ(stepper_phase(), 1);
  ASSERT_EQ(stepper_position(), 2);
  
  stepper_step_full(-2);
  ASSERT_EQ(stepper_phase(), 7);  // wrap-around
  ASSERT_EQ(stepper_position(), -2);
}

TEST(step_half_forward) {
  stepper_init();
  // Half step forward walks through all phases sequentially
  stepper_step_half(1);
  ASSERT_EQ(stepper_phase(), 1);
  ASSERT_EQ(stepper_position(), 1);
  
  stepper_step_half(1);
  ASSERT_EQ(stepper_phase(), 2);
  ASSERT_EQ(stepper_position(), 2);
  
  stepper_step_half(6);
  ASSERT_EQ(stepper_phase(), 0);  // wrap-around after phase 7
  ASSERT_EQ(stepper_position(), 8);
}

TEST(step_half_backward) {
  stepper_init();
  // Go forward to phase 3
  stepper_step_half(3);
  ASSERT_EQ(stepper_phase(), 3);
  ASSERT_EQ(stepper_position(), 3);
  
  // Step backward
  stepper_step_half(-2);
  ASSERT_EQ(stepper_phase(), 1);
  ASSERT_EQ(stepper_position(), 1);
  
  stepper_step_half(-2);
  ASSERT_EQ(stepper_phase(), 7);  // wrap-around
  ASSERT_EQ(stepper_position(), -1);
}

TEST(release_all_coils_off) {
  stepper_init();
  // Apply a phase to energize coils
  stepper_apply_phase(5);
  ASSERT_PIN(8, HIGH);
  ASSERT_PIN(9, HIGH);
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(11, HIGH);
  
  // Release should turn all off but preserve phase and position
  stepper_release();
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(11, LOW);
  ASSERT_EQ(stepper_phase(), 5);  // phase unchanged
  ASSERT_EQ(stepper_position(), 0);  // position unchanged
}

TEST(set_delay) {
  stepper_init();
  ASSERT_EQ(stepper_delay(), STEPPER_DEFAULT_DELAY_MS);
  
  stepper_set_delay(5);
  ASSERT_EQ(stepper_delay(), 5);
  
  stepper_set_delay(0);
  ASSERT_EQ(stepper_delay(), 0);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
