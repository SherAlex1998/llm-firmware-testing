// Hand-written test for counter_fixture: all tests pass on the original sources.
#include "logic.h"
#include "test_runner.h"

TEST(init_is_zero) {
  cnt_init();
  ASSERT_EQ(cnt_value(), 0);
  ASSERT_PIN(OVF_LED_PIN, LOW);
}

TEST(digits_split_value) {
  cnt_init();
  cnt_add(42);
  ASSERT_EQ(cnt_digit(0), 2);
  ASSERT_EQ(cnt_digit(1), 4);
}

TEST(overflow_wraps_and_lights_led) {
  cnt_init();
  cnt_add(99);
  cnt_add(3);
  ASSERT_EQ(cnt_value(), 2);
  ASSERT_PIN(OVF_LED_PIN, HIGH);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
