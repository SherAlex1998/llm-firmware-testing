#include "logic.h"
#include "test_runner.h"

TEST(init_sets_all_yellow) {
  ti_init();
  for (uint8_t d = 0; d < TI_NUM_DIRECTIONS; d++) {
    ASSERT_EQ(ti_direction_color(d), TI_YELLOW);
    ASSERT_PIN(TI_PIN_YELLOW[d], HIGH);
    ASSERT_PIN(TI_PIN_RED[d], LOW);
    ASSERT_PIN(TI_PIN_GREEN[d], LOW);
  }
  ASSERT_EQ(ti_phase(), 0);
}

TEST(set_direction_valid_and_invalid) {
  ti_init();
  ASSERT_TRUE(ti_set_direction(0, TI_GREEN));
  ASSERT_EQ(ti_direction_color(0), TI_GREEN);
  ASSERT_PIN(TI_PIN_GREEN[0], HIGH);
  ASSERT_PIN(TI_PIN_RED[0], LOW);
  ASSERT_PIN(TI_PIN_YELLOW[0], LOW);

  // invalid direction
  ASSERT_FALSE(ti_set_direction(4, TI_RED));
  // invalid color
  ASSERT_FALSE(ti_set_direction(1, 3));
  // out of range dir returns TI_RED
  ASSERT_EQ(ti_direction_color(5), TI_RED);
}

TEST(apply_phase_zero_green_a) {
  ti_init();
  ASSERT_TRUE(ti_apply_phase(0));
  ASSERT_EQ(ti_phase(), 0);
  ASSERT_EQ(ti_direction_color(0), TI_GREEN);
  ASSERT_PIN(TI_PIN_GREEN[0], HIGH);
  for (uint8_t d = 1; d < TI_NUM_DIRECTIONS; d++) {
    ASSERT_EQ(ti_direction_color(d), TI_RED);
    ASSERT_PIN(TI_PIN_RED[d], HIGH);
  }
}

TEST(apply_phase_last_yellow_d) {
  ti_init();
  ASSERT_TRUE(ti_apply_phase(7));
  ASSERT_EQ(ti_phase(), 7);
  ASSERT_EQ(ti_direction_color(3), TI_YELLOW);
  ASSERT_PIN(TI_PIN_YELLOW[3], HIGH);
  for (uint8_t d = 0; d < 3; d++) {
    ASSERT_EQ(ti_direction_color(d), TI_RED);
    ASSERT_PIN(TI_PIN_RED[d], HIGH);
  }

  // invalid phase out of range
  ASSERT_FALSE(ti_apply_phase(8));
  ASSERT_EQ(ti_phase(), 7); // unchanged
}

TEST(next_phase_wraps_around) {
  ti_init();
  ti_apply_phase(7);
  ti_next_phase();
  ASSERT_EQ(ti_phase(), 0);
  ASSERT_EQ(ti_direction_color(0), TI_GREEN);
  ASSERT_PIN(TI_PIN_GREEN[0], HIGH);
}

TEST(is_safe_true_for_single_active) {
  ti_init();
  ti_apply_phase(2); // dir 1 green, others red
  ASSERT_TRUE(ti_is_safe());

  ti_apply_phase(5); // dir 2 yellow, others red
  ASSERT_TRUE(ti_is_safe());
}

TEST(is_safe_false_for_conflicting_directions) {
  ti_init();
  ti_apply_phase(0); // dir 0 green
  ti_set_direction(1, TI_GREEN); // manually make dir1 non-red too
  ASSERT_FALSE(ti_is_safe());
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
