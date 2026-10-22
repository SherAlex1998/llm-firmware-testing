// Hand-written infrastructure check (not shown to the LLM): SPI display on avr8js, short and long presses.
#include "logic.h"
#include "test_runner.h"

TEST(idle_pass_shows_zero_zero) {
  score_init();
  score_loop_with(LOW, LOW, 1000);
  ASSERT_TRUE(strncmp(message, "0       0", 9) == 0);
  bool lit = false;
  for (uint8_t c = 0; c < 32; c++) if (score_column(c)) lit = true;
  ASSERT_TRUE(lit);
}

TEST(short_press_adds_point) {
  score_init();
  score_loop_with(LOW, HIGH, 1000);
  score_loop_with(LOW, LOW, 1100);
  ASSERT_EQ(currentScoreRight, 1);
}

TEST(long_press_resets) {
  score_init();
  currentScoreLeft = 7;
  score_loop_with(HIGH, LOW, 1000);
  score_loop_with(HIGH, LOW, 1600);
  ASSERT_EQ(currentScoreLeft, 0);
  score_loop_with(LOW, LOW, 1700);
  ASSERT_EQ(currentScoreLeft, 0);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
