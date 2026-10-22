// Hand-written infrastructure check (not shown to the LLM): fresh EEPROM, buttons released, count-down logic.
#include "logic.h"
#include "test_runner.h"

TEST(fresh_eeprom_alarm_time_and_idle) {
  ktimer_set_delay_scale(0);
  ktimer_init();
  ASSERT_EQ(state, IDLE);
  ASSERT_EQ(alarm_minutes, -1);          // erased EEPROM (0xFFFF), as in the original
  ASSERT_FALSE(button_press(BUTTON1));
}

TEST(button2_sets_minutes_and_button1_starts) {
  ktimer_set_delay_scale(0);
  ktimer_init();
  ktimer_loop_with(false, true);
  ktimer_loop_with(false, true);
  ASSERT_EQ(alarm_minutes, 2);
  ASSERT_TRUE(strncmp(ktimer_lcd_line(0), "02:00", 5) == 0);
  ktimer_loop_with(true, false);
  ASSERT_EQ(state, RUNNING);
  ktimer_loop_with(false, false);
  ASSERT_EQ(minutes, 1);
  ASSERT_EQ(seconds, 59);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
