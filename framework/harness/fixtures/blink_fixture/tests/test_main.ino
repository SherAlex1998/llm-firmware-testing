// Hand-written test for blink_fixture: all tests pass on the original logic.cpp.
#include "logic.h"
#include "test_runner.h"

TEST(init_turns_led_off) {
  led_init();
  ASSERT_PIN(LED_PIN, LOW);
  ASSERT_EQ(led_state(), LOW);
}

TEST(on_sets_pin_high) {
  led_init();
  led_on();
  ASSERT_PIN(LED_PIN, HIGH);
  ASSERT_EQ(led_state(), HIGH);
}

TEST(off_sets_pin_low) {
  led_init();
  led_on();
  led_off();
  ASSERT_PIN(LED_PIN, LOW);
}

TEST(toggle_flips_twice) {
  led_init();
  led_toggle();
  ASSERT_PIN(LED_PIN, HIGH);
  led_toggle();
  ASSERT_PIN(LED_PIN, LOW);
}

TEST(blink_counts_and_ends_off) {
  led_init();
  led_blink(3, 5);
  ASSERT_EQ(led_on_count(), 3);
  ASSERT_PIN(LED_PIN, LOW);
}

TEST(on_count_accumulates) {
  led_init();
  led_on();
  led_on();
  led_off();
  ASSERT_EQ(led_on_count(), 2);
  ASSERT_NE(led_on_count(), 0);
  ASSERT_NEAR(led_on_count(), 3, 1);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
