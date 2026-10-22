#include "logic.h"
#include "test_runner.h"

TEST(init_sets_defaults_and_releases_coils) {
  stepper_init();

  ASSERT_EQ(stepper_phase(), 0);
  ASSERT_EQ(stepper_position(), 0);
  ASSERT_EQ(stepper_delay(), STEPPER_DEFAULT_DELAY_MS);
  ASSERT_PIN(STEPPER_PIN_IN1, LOW);
  ASSERT_PIN(STEPPER_PIN_IN2, LOW);
  ASSERT_PIN(STEPPER_PIN_IN3, LOW);
  ASSERT_PIN(STEPPER_PIN_IN4, LOW);
}

TEST(coil_patterns_match_sequence_and_reject_invalid) {
  stepper_init();

  ASSERT_EQ(stepper_coil_pattern(0), B1000);
  ASSERT_EQ(stepper_coil_pattern(1), B1100);
  ASSERT_EQ(stepper_coil_pattern(2), B0100);
  ASSERT_EQ(stepper_coil_pattern(3), B0110);
  ASSERT_EQ(stepper_coil_pattern(4), B0010);
  ASSERT_EQ(stepper_coil_pattern(5), B0011);
  ASSERT_EQ(stepper_coil_pattern(6), B0001);
  ASSERT_EQ(stepper_coil_pattern(7), B1001);
  ASSERT_EQ(stepper_coil_pattern(8), 0);
  ASSERT_EQ(stepper_coil_pattern(255), 0);
}

TEST(apply_phase_writes_expected_pins_and_invalid_preserves_state) {
  stepper_init();

  ASSERT_TRUE(stepper_apply_phase(3));
  ASSERT_EQ(stepper_phase(), 3);
  ASSERT_PIN(STEPPER_PIN_IN1, LOW);
  ASSERT_PIN(STEPPER_PIN_IN2, HIGH);
  ASSERT_PIN(STEPPER_PIN_IN3, HIGH);
  ASSERT_PIN(STEPPER_PIN_IN4, LOW);

  ASSERT_TRUE(stepper_apply_phase(6));
  ASSERT_EQ(stepper_phase(), 6);
  ASSERT_PIN(STEPPER_PIN_IN1, HIGH);
  ASSERT_PIN(STEPPER_PIN_IN2, LOW);
  ASSERT_PIN(STEPPER_PIN_IN3, LOW);
  ASSERT_PIN(STEPPER_PIN_IN4, LOW);

  ASSERT_FALSE(stepper_apply_phase(8));
  ASSERT_EQ(stepper_phase(), 6);
  ASSERT_PIN(STEPPER_PIN_IN1, HIGH);
  ASSERT_PIN(STEPPER_PIN_IN2, LOW);
  ASSERT_PIN(STEPPER_PIN_IN3, LOW);
  ASSERT_PIN(STEPPER_PIN_IN4, LOW);
}

TEST(half_steps_track_phase_position_and_wrap) {
  stepper_init();
  stepper_set_delay(0);

  stepper_step_half(3);
  ASSERT_EQ(stepper_phase(), 3);
  ASSERT_EQ(stepper_position(), 3);
  ASSERT_PIN(STEPPER_PIN_IN1, LOW);
  ASSERT_PIN(STEPPER_PIN_IN2, HIGH);
  ASSERT_PIN(STEPPER_PIN_IN3, HIGH);
  ASSERT_PIN(STEPPER_PIN_IN4, LOW);

  stepper_step_half(-5);
  ASSERT_EQ(stepper_phase(), 6);
  ASSERT_EQ(stepper_position(), -2);
  ASSERT_PIN(STEPPER_PIN_IN1, HIGH);
  ASSERT_PIN(STEPPER_PIN_IN2, LOW);
  ASSERT_PIN(STEPPER_PIN_IN3, LOW);
  ASSERT_PIN(STEPPER_PIN_IN4, LOW);
}

TEST(full_steps_use_two_coil_phases_and_update_position) {
  stepper_init();
  stepper_set_delay(0);

  stepper_step_full(2);
  ASSERT_EQ(stepper_phase(), 3);
  ASSERT_EQ(stepper_position(), 4);
  ASSERT_PIN(STEPPER_PIN_IN1, LOW);
  ASSERT_PIN(STEPPER_PIN_IN2, HIGH);
  ASSERT_PIN(STEPPER_PIN_IN3, HIGH);
  ASSERT_PIN(STEPPER_PIN_IN4, LOW);

  stepper_step_full(-3);
  ASSERT_EQ(stepper_phase(), 5);
  ASSERT_EQ(stepper_position(), -2);
  ASSERT_PIN(STEPPER_PIN_IN1, HIGH);
  ASSERT_PIN(STEPPER_PIN_IN2, HIGH);
  ASSERT_PIN(STEPPER_PIN_IN3, LOW);
  ASSERT_PIN(STEPPER_PIN_IN4, LOW);
}

TEST(release_turns_off_coils_without_changing_motion_state) {
  stepper_init();
  stepper_set_delay(0);

  stepper_step_half(1);
  ASSERT_EQ(stepper_phase(), 1);
  ASSERT_EQ(stepper_position(), 1);

  stepper_release();
  ASSERT_EQ(stepper_phase(), 1);
  ASSERT_EQ(stepper_position(), 1);
  ASSERT_PIN(STEPPER_PIN_IN1, LOW);
  ASSERT_PIN(STEPPER_PIN_IN2, LOW);
  ASSERT_PIN(STEPPER_PIN_IN3, LOW);
  ASSERT_PIN(STEPPER_PIN_IN4, LOW);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
