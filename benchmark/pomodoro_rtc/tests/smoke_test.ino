// Hand-written infrastructure check (not shown to the LLM): DS1307 with the fixed date, state machine reachable.
#include "logic.h"
#include "test_runner.h"

TEST(rtc_date_reaches_statistics) {
  pomo_set_delay_scale(0);
  pomo_init();
  ASSERT_EQ(pomo_user_data(0), 11);          // RTC starts at 2030-03-11
  ASSERT_EQ(pomo_status(), STATUS_IDLE);
}

TEST(short_click_in_idle_adds_five_minutes) {
  pomo_set_delay_scale(0);
  pomo_init();
  pomo_handle_action(CLICK_SHORT_ACTION);
  ASSERT_EQ(pomo_minute(), 30);
}

TEST(long_click_prepares_then_starts_work) {
  pomo_set_delay_scale(0);
  pomo_init();
  pomo_handle_action(CLICK_LONG_ACTION);
  ASSERT_EQ(pomo_status(), STATUS_PREPARE);
  for (uint8_t i = 0; i < 22; i++) pomo_tick();   // 11 s: the 10 s countdown runs out
  ASSERT_TRUE(pomo_seconds_event());
  pomo_update();
  ASSERT_EQ(pomo_status(), STATUS_RUN_WORK);
  ASSERT_EQ(pomo_minute(), 24);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
