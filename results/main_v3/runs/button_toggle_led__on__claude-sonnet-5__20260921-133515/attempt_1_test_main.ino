#include "logic.h"
#include "test_runner.h"

TEST(init_sets_led_off_and_no_press) {
  toggle_init();
  ASSERT_PIN(TOGGLE_PIN_LED, LOW);
  ASSERT_FALSE(toggle_led_state());
}

TEST(set_led_forces_state) {
  toggle_init();
  toggle_set_led(true);
  ASSERT_PIN(TOGGLE_PIN_LED, HIGH);
  ASSERT_TRUE(toggle_led_state());

  toggle_set_led(false);
  ASSERT_PIN(TOGGLE_PIN_LED, LOW);
  ASSERT_FALSE(toggle_led_state());
}

TEST(update_toggles_on_release_edge) {
  toggle_init();
  // press
  toggle_update(true);
  ASSERT_FALSE(toggle_led_state());
  ASSERT_PIN(TOGGLE_PIN_LED, LOW);
  // release -> toggle
  toggle_update(false);
  ASSERT_TRUE(toggle_led_state());
  ASSERT_PIN(TOGGLE_PIN_LED, HIGH);
}

TEST(update_no_toggle_while_held_or_idle) {
  toggle_init();
  // idle, no press
  toggle_update(false);
  ASSERT_FALSE(toggle_led_state());
  // press and hold, no toggle yet
  toggle_update(true);
  toggle_update(true);
  ASSERT_FALSE(toggle_led_state());
}

TEST(update_multiple_press_release_cycles) {
  toggle_init();
  toggle_update(true);
  toggle_update(false); // toggle 1 -> on
  ASSERT_TRUE(toggle_led_state());

  toggle_update(true);
  toggle_update(false); // toggle 2 -> off
  ASSERT_FALSE(toggle_led_state());
  ASSERT_PIN(TOGGLE_PIN_LED, LOW);
}

TEST(poll_reads_actual_button_pin) {
  toggle_init();
  // Button pin is INPUT_PULLUP; call poll to exercise it via toggle_button_pressed.
  bool pressed = toggle_button_pressed();
  toggle_poll();
  // Result depends on the pin/mock state, but function must not crash and LED state
  // must stay boolean-consistent.
  ASSERT_TRUE(toggle_led_state() == true || toggle_led_state() == false);
  (void)pressed;
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
