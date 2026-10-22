#include "logic.h"
#include "test_runner.h"

TEST(init_configures_pins) {
  toggle_init();
  ASSERT_PIN(13, LOW);
  ASSERT_FALSE(toggle_led_state());
}

TEST(button_pressed_reads_pin) {
  toggle_init();
  // Pin 2 is INPUT_PULLUP, so it reads HIGH when not pressed
  bool result = toggle_button_pressed();
  ASSERT_FALSE(result);
}

TEST(set_led_controls_output) {
  toggle_init();
  toggle_set_led(true);
  ASSERT_PIN(13, HIGH);
  ASSERT_TRUE(toggle_led_state());
  toggle_set_led(false);
  ASSERT_PIN(13, LOW);
  ASSERT_FALSE(toggle_led_state());
}

TEST(toggle_on_release_edge) {
  toggle_init();
  ASSERT_FALSE(toggle_led_state());
  
  // Simulate press (button goes LOW)
  toggle_update(true);
  ASSERT_FALSE(toggle_led_state());
  
  // Simulate release (button goes HIGH)
  toggle_update(false);
  ASSERT_TRUE(toggle_led_state());
  ASSERT_PIN(13, HIGH);
  
  // Press again
  toggle_update(true);
  ASSERT_TRUE(toggle_led_state());
  
  // Release again -> toggle back to OFF
  toggle_update(false);
  ASSERT_FALSE(toggle_led_state());
  ASSERT_PIN(13, LOW);
}

TEST(multiple_toggle_cycles) {
  toggle_init();
  
  // Cycle 1: press and release -> ON
  toggle_update(true);
  toggle_update(false);
  ASSERT_TRUE(toggle_led_state());
  
  // Cycle 2: press and release -> OFF
  toggle_update(true);
  toggle_update(false);
  ASSERT_FALSE(toggle_led_state());
  
  // Cycle 3: press and release -> ON
  toggle_update(true);
  toggle_update(false);
  ASSERT_TRUE(toggle_led_state());
}

TEST(held_button_no_toggle) {
  toggle_init();
  ASSERT_FALSE(toggle_led_state());
  
  // Press and hold (multiple updates while pressed)
  toggle_update(true);
  toggle_update(true);
  toggle_update(true);
  ASSERT_FALSE(toggle_led_state());
  
  // Release
  toggle_update(false);
  ASSERT_TRUE(toggle_led_state());
}

TEST(poll_integration) {
  toggle_init();
  
  // First poll while button not pressed - no change
  toggle_poll();
  ASSERT_FALSE(toggle_led_state());
  
  // Simulate press, release pattern by calling poll
  // (In real scenario, button pin would be manipulated externally;
  //  here we test that poll calls toggle_button_pressed correctly)
  toggle_poll();
  ASSERT_FALSE(toggle_led_state());
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
