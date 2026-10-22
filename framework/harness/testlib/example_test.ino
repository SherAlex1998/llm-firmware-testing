// Example test sketch for the harness (documentation only; the compiled self-test lives in
// framework/harness/fixtures/blink_fixture/tests/test_main.ino).
//
// A test sketch is a normal Arduino sketch that:
//   1. includes the module under test ("logic.h") and the framework ("test_runner.h"),
//   2. defines 3..8 tests with TEST(name) { ... },
//   3. calls RUN_ALL_TESTS() from setup() after Serial.begin(9600),
//   4. leaves loop() empty.
#include "logic.h"
#include "test_runner.h"

TEST(led_starts_off) {
  led_init();
  ASSERT_PIN(LED_PIN, LOW);
}

TEST(led_on_drives_pin_high) {
  led_init();
  led_on();
  ASSERT_PIN(LED_PIN, HIGH);
  ASSERT_EQ(led_state(), HIGH);
}

TEST(blink_ends_with_led_off) {
  led_init();
  led_blink(2, 10);          // short delays only: the whole run must finish within 15 s
  ASSERT_EQ(led_on_count(), 2);
  ASSERT_PIN(LED_PIN, LOW);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
