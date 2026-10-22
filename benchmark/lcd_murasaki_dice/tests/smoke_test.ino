// Hand-written infrastructure check (not shown to the LLM): pot at 0 selects d100, roll on falling edge, base 5.
#include "logic.h"
#include "test_runner.h"

TEST(unconnected_pot_selects_d100) {
  jdr_set_delay_scale(0);
  jdr_init();
  selectDice();
  ASSERT_EQ(selectedDiceType, 100);
  ASSERT_EQ(selectedDiceIndex, 14);
}

TEST(falling_edge_rolls_within_range) {
  jdr_set_delay_scale(0);
  jdr_init();
  selectDiceWith(1000);              // d2
  ASSERT_EQ(selectedDiceType, 2);
  rollDiceWith(HIGH, 100);
  ASSERT_EQ(lastRoll, 0);
  rollDiceWith(LOW, 200);
  ASSERT_TRUE(lastRoll >= 1 && lastRoll <= 2);
}

TEST(clock_shows_base5_seconds) {
  jdr_set_delay_scale(0);
  jdr_init();
  updateMurasakiClockAt(7000);       // 7 = 12 in base 5
  ASSERT_EQ(jdr_lcd_line(0)[14], '1');
  ASSERT_EQ(jdr_lcd_line(0)[15], '2');
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
