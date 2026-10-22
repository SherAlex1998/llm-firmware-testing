// Hand-written infrastructure check (not shown to the LLM): ISR twins with explicit time, LED intervals, LCD text.
#include "logic.h"
#include "test_runner.h"

TEST(fresh_start_is_green_and_just_now) {
  pet_init();
  pet_loop_at(1000);
  ASSERT_PIN(greenLEDPin, HIGH);
  ASSERT_TRUE(strncmp(pet_lcd_line(1) + 4, "Just now", 8) == 0);
}

TEST(seven_hours_after_feeding_is_yellow) {
  pet_init();
  resetTimeAt(5000);
  pet_loop_at(5000 + 7 * hour);
  ASSERT_PIN(yellowLEDPin, HIGH);
  ASSERT_PIN(greenLEDPin, LOW);
}

TEST(second_press_moves_feeding_back_an_hour) {
  pet_init();
  resetTimeAt(5000);
  resetTimeAt(5600);
  ASSERT_EQ((long)(5000 - lastFeedingTime), (long)hour);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
