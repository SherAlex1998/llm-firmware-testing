#include "logic.h"
#include "test_runner.h"

TEST(init_enters_red_state) {
  tl_init();
  ASSERT_EQ(tl_state(), TL_RED);
  ASSERT_PIN(TL_PIN_RED, HIGH);
  ASSERT_PIN(TL_PIN_YELLOW, LOW);
  ASSERT_PIN(TL_PIN_GREEN, LOW);
}

TEST(set_state_drives_each_led_combination) {
  tl_init();

  tl_set_state(TL_RED_YELLOW);
  ASSERT_EQ(tl_state(), TL_RED_YELLOW);
  ASSERT_PIN(TL_PIN_RED, HIGH);
  ASSERT_PIN(TL_PIN_YELLOW, HIGH);
  ASSERT_PIN(TL_PIN_GREEN, LOW);

  tl_set_state(TL_GREEN);
  ASSERT_EQ(tl_state(), TL_GREEN);
  ASSERT_PIN(TL_PIN_RED, LOW);
  ASSERT_PIN(TL_PIN_YELLOW, LOW);
  ASSERT_PIN(TL_PIN_GREEN, HIGH);

  tl_set_state(TL_GREEN_BLINK);
  ASSERT_EQ(tl_state(), TL_GREEN_BLINK);
  ASSERT_PIN(TL_PIN_RED, LOW);
  ASSERT_PIN(TL_PIN_YELLOW, LOW);
  ASSERT_PIN(TL_PIN_GREEN, LOW);

  tl_set_state(TL_YELLOW);
  ASSERT_EQ(tl_state(), TL_YELLOW);
  ASSERT_PIN(TL_PIN_RED, LOW);
  ASSERT_PIN(TL_PIN_YELLOW, HIGH);
  ASSERT_PIN(TL_PIN_GREEN, LOW);
}

TEST(invalid_set_state_is_ignored) {
  tl_init();
  tl_set_state(TL_GREEN);
  tl_set_state(TL_STATE_COUNT);

  ASSERT_EQ(tl_state(), TL_GREEN);
  ASSERT_PIN(TL_PIN_RED, LOW);
  ASSERT_PIN(TL_PIN_YELLOW, LOW);
  ASSERT_PIN(TL_PIN_GREEN, HIGH);
}

TEST(next_state_follows_cycle_and_handles_invalid) {
  tl_init();
  ASSERT_EQ(tl_next_state(TL_RED), TL_RED_YELLOW);
  ASSERT_EQ(tl_next_state(TL_RED_YELLOW), TL_GREEN);
  ASSERT_EQ(tl_next_state(TL_GREEN), TL_GREEN_BLINK);
  ASSERT_EQ(tl_next_state(TL_GREEN_BLINK), TL_YELLOW);
  ASSERT_EQ(tl_next_state(TL_YELLOW), TL_RED);
  ASSERT_EQ(tl_next_state(TL_STATE_COUNT), TL_RED);
}

TEST(advance_updates_state_and_leds) {
  tl_init();

  tl_advance();
  ASSERT_EQ(tl_state(), TL_RED_YELLOW);
  ASSERT_PIN(TL_PIN_RED, HIGH);
  ASSERT_PIN(TL_PIN_YELLOW, HIGH);
  ASSERT_PIN(TL_PIN_GREEN, LOW);

  tl_advance();
  ASSERT_EQ(tl_state(), TL_GREEN);
  ASSERT_PIN(TL_PIN_RED, LOW);
  ASSERT_PIN(TL_PIN_YELLOW, LOW);
  ASSERT_PIN(TL_PIN_GREEN, HIGH);

  tl_set_state(TL_YELLOW);
  tl_advance();
  ASSERT_EQ(tl_state(), TL_RED);
  ASSERT_PIN(TL_PIN_RED, HIGH);
  ASSERT_PIN(TL_PIN_YELLOW, LOW);
  ASSERT_PIN(TL_PIN_GREEN, LOW);
}

TEST(state_durations_match_specification) {
  tl_init();
  ASSERT_EQ(tl_state_duration_ms(TL_RED), 5000);
  ASSERT_EQ(tl_state_duration_ms(TL_RED_YELLOW), 1500);
  ASSERT_EQ(tl_state_duration_ms(TL_GREEN), 5000);
  ASSERT_EQ(tl_state_duration_ms(TL_GREEN_BLINK), 2000);
  ASSERT_EQ(tl_state_duration_ms(TL_YELLOW), 2000);
  ASSERT_EQ(tl_state_duration_ms(TL_STATE_COUNT), 0);
}

TEST(blink_green_ends_with_green_off) {
  tl_init();
  tl_set_state(TL_GREEN);
  tl_blink_green(2, 1);

  ASSERT_EQ(tl_state(), TL_GREEN);
  ASSERT_PIN(TL_PIN_RED, LOW);
  ASSERT_PIN(TL_PIN_YELLOW, LOW);
  ASSERT_PIN(TL_PIN_GREEN, LOW);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
