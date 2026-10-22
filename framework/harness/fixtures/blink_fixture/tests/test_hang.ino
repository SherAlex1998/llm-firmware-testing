// Test that never finishes: expected verdict "timeout" (no @RUN_END).
#include "logic.h"
#include "test_runner.h"

TEST(first_passes) {
  led_init();
  ASSERT_PIN(LED_PIN, LOW);
}

TEST(hangs_forever) {
  while (true) {
    led_toggle();
    delay(50);
  }
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
