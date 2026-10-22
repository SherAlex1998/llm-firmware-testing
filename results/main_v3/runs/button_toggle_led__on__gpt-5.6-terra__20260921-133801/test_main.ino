#include "logic.h"
#include "test_runner.h"

TEST(init_sets_led_off_and_button_idle) {
  toggle_init();

  ASSERT_FALSE(toggle_led_state());
  ASSERT_PIN(TOGGLE_PIN_LED, LOW);
  ASSERT_FALSE(toggle_button_pressed());
}

TEST(set_led_controls_state_and_pin) {
  toggle_init();

  toggle_set_led(true);
  ASSERT_TRUE(toggle_led_state());
  ASSERT_PIN(TOGGLE_PIN_LED, HIGH);

  toggle_set_led(false);
  ASSERT_FALSE(toggle_led_state());
  ASSERT_PIN(TOGGLE_PIN_LED, LOW);
}

TEST(update_toggles_only_on_release_after_press) {
  toggle_init();

  toggle_update(false);
  ASSERT_FALSE(toggle_led_state());
  ASSERT_PIN(TOGGLE_PIN_LED, LOW);

  toggle_update(true);
  ASSERT_FALSE(toggle_led_state());
  ASSERT_PIN(TOGGLE_PIN_LED, LOW);

  toggle_update(true);
  ASSERT_FALSE(toggle_led_state());

  toggle_update(false);
  ASSERT_TRUE(toggle_led_state());
  ASSERT_PIN(TOGGLE_PIN_LED, HIGH);
}

TEST(successive_press_release_cycles_toggle_led) {
  toggle_init();

  toggle_update(true);
  toggle_update(false);
  ASSERT_TRUE(toggle_led_state());
  ASSERT_PIN(TOGGLE_PIN_LED, HIGH);

  toggle_update(true);
  toggle_update(false);
  ASSERT_FALSE(toggle_led_state());
  ASSERT_PIN(TOGGLE_PIN_LED, LOW);
}

TEST(poll_idle_does_not_change_led) {
  toggle_init();

  toggle_poll();
  ASSERT_FALSE(toggle_led_state());
  ASSERT_PIN(TOGGLE_PIN_LED, LOW);

  toggle_set_led(true);
  toggle_poll();
  ASSERT_TRUE(toggle_led_state());
  ASSERT_PIN(TOGGLE_PIN_LED, HIGH);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
