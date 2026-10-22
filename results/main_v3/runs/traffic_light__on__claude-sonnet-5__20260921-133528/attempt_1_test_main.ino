#include "logic.h"
#include "test_runner.h"

TEST(init_sets_red_state) {
  tl_init();
  ASSERT_EQ(tl_state(), TL_RED);
  ASSERT_PIN(TL_PIN_RED, HIGH);
  ASSERT_PIN(TL_PIN_YELLOW, LOW);
  ASSERT_PIN(TL_PIN_GREEN, LOW);
}

TEST(set_state_outputs) {
  tl_init();

  tl_set_state(TL_RED);
  ASSERT_PIN(TL_PIN_RED, HIGH);
  ASSERT_PIN(TL_PIN_YELLOW, LOW);
  ASSERT_PIN(TL_PIN_GREEN, LOW);

  tl_set_state(TL_RED_YELLOW);
  ASSERT_PIN(TL_PIN_RED, HIGH);
  ASSERT_PIN(TL_PIN_YELLOW, HIGH);
  ASSERT_PIN(TL_PIN_GREEN, LOW);

  tl_set_state(TL_GREEN);
  ASSERT_PIN(TL_PIN_RED, LOW);
  ASSERT_PIN(TL_PIN_YELLOW, LOW);
  ASSERT_PIN(TL_PIN_GREEN, HIGH);

  tl_set_state(TL_GREEN_BLINK);
  ASSERT_PIN(TL_PIN_RED, LOW);
  ASSERT_PIN(TL_PIN_YELLOW, LOW);
  ASSERT_PIN(TL_PIN_GREEN, LOW);

  tl_set_state(TL_YELLOW);
  ASSERT_PIN(TL_PIN_RED, LOW);
  ASSERT_PIN(TL_PIN_YELLOW, HIGH);
  ASSERT_PIN(TL_PIN_GREEN, LOW);
}

TEST(set_state_invalid_is_ignored) {
  tl_init();
  tl_set_state(TL_GREEN);
  uint8_t before = tl_state();
  tl_set_state(TL_STATE_COUNT); // invalid, out of range
  ASSERT_EQ(tl_state(), before);
  ASSERT_PIN(TL_PIN_RED, LOW);
  ASSERT_PIN(TL_PIN_YELLOW, LOW);
  ASSERT_PIN(TL_PIN_GREEN, HIGH);
}

TEST(next_state_cycle_and_wraparound) {
  ASSERT_EQ(tl_next_state(TL_RED), TL_RED_YELLOW);
  ASSERT_EQ(tl_next_state(TL_RED_YELLOW), TL_GREEN);
  ASSERT_EQ(tl_next_state(TL_GREEN), TL_GREEN_BLINK);
  ASSERT_EQ(tl_next_state(TL_GREEN_BLINK), TL_YELLOW);
  ASSERT_EQ(tl_next_state(TL_YELLOW), TL_RED); // wrap-around
  ASSERT_EQ(tl_next_state(TL_STATE_COUNT), TL_RED); // invalid input
}

TEST(advance_transitions_state) {
  tl_init();
  ASSERT_EQ(tl_state(), TL_RED);

  tl_advance();
  ASSERT_EQ(tl_state(), TL_RED_YELLOW);
  ASSERT_PIN(TL_PIN_RED, HIGH);
  ASSERT_PIN(TL_PIN_YELLOW, HIGH);
  ASSERT_PIN(TL_PIN_GREEN, LOW);

  tl_advance();
  ASSERT_EQ(tl_state(), TL_GREEN);
  ASSERT_PIN(TL_PIN_GREEN, HIGH);

  tl_advance();
  ASSERT_EQ(tl_state(), TL_GREEN_BLINK);

  tl_advance();
  ASSERT_EQ(tl_state(), TL_YELLOW);
  ASSERT_PIN(TL_PIN_YELLOW, HIGH);

  tl_advance();
  ASSERT_EQ(tl_state(), TL_RED); // full cycle wraps back
  ASSERT_PIN(TL_PIN_RED, HIGH);
}

TEST(state_duration_values) {
  ASSERT_EQ(tl_state_duration_ms(TL_RED), 5000);
  ASSERT_EQ(tl_state_duration_ms(TL_RED_YELLOW), 1500);
  ASSERT_EQ(tl_state_duration_ms(TL_GREEN), 5000);
  ASSERT_EQ(tl_state_duration_ms(TL_GREEN_BLINK), 2000);
  ASSERT_EQ(tl_state_duration_ms(TL_YELLOW), 2000);
  ASSERT_EQ(tl_state_duration_ms(TL_STATE_COUNT), 0); // invalid state
}

TEST(blink_green_ends_low) {
  tl_init();
  tl_set_state(TL_GREEN);
  ASSERT_PIN(TL_PIN_GREEN, HIGH);

  tl_blink_green(3, 10);
  ASSERT_PIN(TL_PIN_GREEN, LOW);
  // red/yellow untouched by blinking
  ASSERT_PIN(TL_PIN_RED, LOW);
  ASSERT_PIN(TL_PIN_YELLOW, LOW);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
