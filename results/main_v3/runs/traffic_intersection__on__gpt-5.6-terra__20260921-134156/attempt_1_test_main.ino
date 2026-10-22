#include "logic.h"
#include "test_runner.h"

TEST(init_sets_all_directions_yellow) {
  ti_init();

  ASSERT_EQ(ti_phase(), 0);
  ASSERT_EQ(ti_direction_color(0), TI_YELLOW);
  ASSERT_EQ(ti_direction_color(1), TI_YELLOW);
  ASSERT_EQ(ti_direction_color(2), TI_YELLOW);
  ASSERT_EQ(ti_direction_color(3), TI_YELLOW);

  ASSERT_PIN(13, LOW);
  ASSERT_PIN(12, LOW);
  ASSERT_PIN(11, HIGH);
  ASSERT_PIN(10, LOW);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(8, HIGH);
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(5, HIGH);
  ASSERT_PIN(4, LOW);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(2, HIGH);

  ASSERT_FALSE(ti_is_safe());
}

TEST(set_direction_controls_leds_and_safety) {
  ti_init();

  ASSERT_TRUE(ti_set_direction(0, TI_RED));
  ASSERT_TRUE(ti_set_direction(1, TI_RED));
  ASSERT_TRUE(ti_set_direction(2, TI_RED));
  ASSERT_TRUE(ti_set_direction(3, TI_RED));
  ASSERT_TRUE(ti_is_safe());

  ASSERT_TRUE(ti_set_direction(3, TI_GREEN));
  ASSERT_EQ(ti_direction_color(3), TI_GREEN);
  ASSERT_PIN(4, LOW);
  ASSERT_PIN(3, HIGH);
  ASSERT_PIN(2, LOW);
  ASSERT_TRUE(ti_is_safe());

  ASSERT_TRUE(ti_set_direction(2, TI_YELLOW));
  ASSERT_EQ(ti_direction_color(2), TI_YELLOW);
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(5, HIGH);
  ASSERT_FALSE(ti_is_safe());
}

TEST(set_direction_rejects_invalid_arguments) {
  ti_init();

  ASSERT_FALSE(ti_set_direction(4, TI_RED));
  ASSERT_EQ(ti_direction_color(0), TI_YELLOW);
  ASSERT_EQ(ti_direction_color(3), TI_YELLOW);
  ASSERT_EQ(ti_direction_color(4), TI_RED);
  ASSERT_PIN(4, LOW);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(2, HIGH);

  ASSERT_TRUE(ti_set_direction(2, TI_GREEN));
  ASSERT_FALSE(ti_set_direction(2, 3));
  ASSERT_EQ(ti_direction_color(2), TI_GREEN);
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(6, HIGH);
  ASSERT_PIN(5, LOW);
}

TEST(apply_first_phase_sets_a_green) {
  ti_init();

  ASSERT_TRUE(ti_apply_phase(0));
  ASSERT_EQ(ti_phase(), 0);
  ASSERT_EQ(ti_direction_color(0), TI_GREEN);
  ASSERT_EQ(ti_direction_color(1), TI_RED);
  ASSERT_EQ(ti_direction_color(2), TI_RED);
  ASSERT_EQ(ti_direction_color(3), TI_RED);

  ASSERT_PIN(13, LOW);
  ASSERT_PIN(12, HIGH);
  ASSERT_PIN(11, LOW);
  ASSERT_PIN(10, HIGH);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(7, HIGH);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(4, HIGH);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(2, LOW);

  ASSERT_TRUE(ti_is_safe());
}

TEST(apply_last_phase_and_rejects_invalid_phase) {
  ti_init();

  ASSERT_TRUE(ti_apply_phase(7));
  ASSERT_EQ(ti_phase(), 7);
  ASSERT_EQ(ti_direction_color(0), TI_RED);
  ASSERT_EQ(ti_direction_color(1), TI_RED);
  ASSERT_EQ(ti_direction_color(2), TI_RED);
  ASSERT_EQ(ti_direction_color(3), TI_YELLOW);

  ASSERT_PIN(13, HIGH);
  ASSERT_PIN(12, LOW);
  ASSERT_PIN(11, LOW);
  ASSERT_PIN(10, HIGH);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(7, HIGH);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(4, LOW);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(2, HIGH);

  ASSERT_TRUE(ti_is_safe());
  ASSERT_FALSE(ti_apply_phase(8));
  ASSERT_EQ(ti_phase(), 7);
  ASSERT_PIN(13, HIGH);
  ASSERT_PIN(12, LOW);
  ASSERT_PIN(11, LOW);
  ASSERT_PIN(10, HIGH);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(7, HIGH);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(4, LOW);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(2, HIGH);
}

TEST(next_phase_advances_and_wraps) {
  ti_init();

  ti_next_phase();
  ASSERT_EQ(ti_phase(), 1);
  ASSERT_EQ(ti_direction_color(0), TI_YELLOW);
  ASSERT_EQ(ti_direction_color(1), TI_RED);
  ASSERT_EQ(ti_direction_color(2), TI_RED);
  ASSERT_EQ(ti_direction_color(3), TI_RED);
  ASSERT_PIN(13, LOW);
  ASSERT_PIN(12, LOW);
  ASSERT_PIN(11, HIGH);
  ASSERT_PIN(10, HIGH);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(7, HIGH);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(4, HIGH);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(2, LOW);

  ASSERT_TRUE(ti_apply_phase(7));
  ti_next_phase();
  ASSERT_EQ(ti_phase(), 0);
  ASSERT_EQ(ti_direction_color(0), TI_GREEN);
  ASSERT_EQ(ti_direction_color(1), TI_RED);
  ASSERT_EQ(ti_direction_color(2), TI_RED);
  ASSERT_EQ(ti_direction_color(3), TI_RED);
  ASSERT_PIN(13, LOW);
  ASSERT_PIN(12, HIGH);
  ASSERT_PIN(11, LOW);
  ASSERT_PIN(10, HIGH);
  ASSERT_PIN(9, LOW);
  ASSERT_PIN(8, LOW);
  ASSERT_PIN(7, HIGH);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(4, HIGH);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(2, LOW);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
