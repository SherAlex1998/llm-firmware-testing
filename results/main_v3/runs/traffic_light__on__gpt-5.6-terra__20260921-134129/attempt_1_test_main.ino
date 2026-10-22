#include "logic.h"
#include "test_runner.h"

TEST(init_enters_red_state) {
  tl_init();
  ASSERT_EQ(tl_state(), TL_RED);
  ASSERT_PIN(2, HIGH);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, LOW);
}

TEST(set_state_drives_each_led_combination) {
  tl_init();

  tl_set_state(TL_RED_YELLOW);
  ASSERT_EQ(tl_state(), TL_RED_YELLOW);
  ASSERT_PIN(2, HIGH);
  ASSERT_PIN(3, HIGH);
  ASSERT_PIN(4, LOW);

  tl_set_state(TL_GREEN);
  ASSERT_EQ(tl_state(), TL_GREEN);
  ASSERT_PIN(2, LOW);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, HIGH);

  tl_set_state(TL_GREEN_BLINK);
  ASSERT_EQ(tl_state(), TL_GREEN_BLINK);
  ASSERT_PIN(2, LOW);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, LOW);

  tl_set_state(TL_YELLOW);
  ASSERT_EQ(tl_state(), TL_YELLOW);
  ASSERT_PIN(2, LOW);
  ASSERT_PIN(3, HIGH);
  ASSERT_PIN(4, LOW);
}

TEST(advance_follows_full_cycle_and_wraps) {
  tl_init();

  tl_advance();
  ASSERT_EQ(tl_state(), TL_RED_YELLOW);
  ASSERT_PIN(2, HIGH);
  ASSERT_PIN(3, HIGH);
  ASSERT_PIN(4, LOW);

  tl_advance();
  ASSERT_EQ(tl_state(), TL_GREEN);
  ASSERT_PIN(2, LOW);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, HIGH);

  tl_advance();
  ASSERT_EQ(tl_state(), TL_GREEN_BLINK);
  ASSERT_PIN(2, LOW);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, LOW);

  tl_advance();
  ASSERT_EQ(tl_state(), TL_YELLOW);
  ASSERT_PIN(2, LOW);
  ASSERT_PIN(3, HIGH);
  ASSERT_PIN(4, LOW);

  tl_advance();
  ASSERT_EQ(tl_state(), TL_RED);
  ASSERT_PIN(2, HIGH);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, LOW);
}

TEST(invalid_set_state_is_ignored) {
  tl_init();
  tl_set_state(TL_GREEN);

  tl_set_state(TL_STATE_COUNT);
  ASSERT_EQ(tl_state(), TL_GREEN);
  ASSERT_PIN(2, LOW);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, HIGH);
}

TEST(next_state_and_durations_are_correct) {
  tl_init();

  ASSERT_EQ(tl_next_state(TL_RED), TL_RED_YELLOW);
  ASSERT_EQ(tl_next_state(TL_RED_YELLOW), TL_GREEN);
  ASSERT_EQ(tl_next_state(TL_GREEN), TL_GREEN_BLINK);
  ASSERT_EQ(tl_next_state(TL_GREEN_BLINK), TL_YELLOW);
  ASSERT_EQ(tl_next_state(TL_YELLOW), TL_RED);
  ASSERT_EQ(tl_next_state(TL_STATE_COUNT), TL_RED);

  ASSERT_EQ(tl_state_duration_ms(TL_RED), 5000);
  ASSERT_EQ(tl_state_duration_ms(TL_RED_YELLOW), 1500);
  ASSERT_EQ(tl_state_duration_ms(TL_GREEN), 5000);
  ASSERT_EQ(tl_state_duration_ms(TL_GREEN_BLINK), 2000);
  ASSERT_EQ(tl_state_duration_ms(TL_YELLOW), 2000);
  ASSERT_EQ(tl_state_duration_ms(TL_STATE_COUNT), 0);
}

TEST(blink_green_ends_low_and_keeps_state) {
  tl_init();
  tl_set_state(TL_GREEN);

  tl_blink_green(2, 5);

  ASSERT_EQ(tl_state(), TL_GREEN);
  ASSERT_PIN(2, LOW);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, LOW);
}

TEST(blink_green_zero_times_ends_low) {
  tl_init();
  tl_set_state(TL_GREEN);

  tl_blink_green(0, 5);

  ASSERT_EQ(tl_state(), TL_GREEN);
  ASSERT_PIN(2, LOW);
  ASSERT_PIN(3, LOW);
  ASSERT_PIN(4, LOW);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
