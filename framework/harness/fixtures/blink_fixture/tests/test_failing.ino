// Deliberately failing test: expected verdict "fail" (1/2 passed).
#include "logic.h"
#include "test_runner.h"

TEST(passes) {
  led_init();
  led_on();
  ASSERT_PIN(LED_PIN, HIGH);
}

TEST(fails_on_purpose) {
  led_init();
  led_on();
  ASSERT_PIN(LED_PIN, LOW);
  ASSERT_TRUE(false);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
