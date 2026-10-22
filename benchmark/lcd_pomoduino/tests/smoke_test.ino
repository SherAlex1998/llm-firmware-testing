// Hand-written infrastructure check (not shown to the LLM): fresh EEPROM, buttons released, state machine reachable.
#include "logic.h"
#include "test_runner.h"

TEST(boots_ready_with_fresh_eeprom_count) {
  pomo2_setup();
  ASSERT_EQ(the_pomodoro.Get_State(), POM_READY);
  ASSERT_EQ(the_pomodoro.Get_Pomodoros(), 255);   // erased EEPROM reads 0xFF, as in the original
  pomo2_loop_at(100);
  ASSERT_FALSE(the_b1.On());
}

TEST(short_b2_click_starts_work_period) {
  pomo2_setup();
  pomo2_step(1000, false, true);
  pomo2_step(1100, false, false);
  ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);
  pomo2_step(1200, false, false);
  ASSERT_TRUE(strncmp(pomo2_lcd_line(0), "P25", 3) == 0);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
