// Hand-written infrastructure check (not shown to the LLM): button released on avr8js, state machine with explicit time.
#include "logic.h"
#include "test_runner.h"

TEST(button_released_gives_no_click) {
  neo_set_delay_scale(0);
  neo_init();
  ASSERT_EQ(getButtonClick(), None);
  ASSERT_EQ(currentState, Off);
}

TEST(long_click_starts_first_pomodoro_red) {
  neo_set_delay_scale(0);
  neo_init();
  neo_loop_at(1000, LongClick);
  ASSERT_EQ(currentState, Pomodoro);
  ASSERT_EQ(pomodoro, 1);
  ASSERT_EQ(neo_pixel(23), 0x100000UL);
}

TEST(pomodoro_ends_with_short_pause) {
  neo_set_delay_scale(0);
  neo_init();
  neo_loop_at(1000, LongClick);
  neo_loop_at(1000 + 25UL * 60000 + 1, None);
  ASSERT_EQ(currentState, ShortPause);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
