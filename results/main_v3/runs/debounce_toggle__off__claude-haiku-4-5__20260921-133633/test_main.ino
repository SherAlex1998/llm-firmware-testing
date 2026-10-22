#include "logic.h"
#include "test_runner.h"

TEST(init_sets_led_high) {
  db_init();
  ASSERT_EQ(db_led_state(), HIGH);
  ASSERT_PIN(13, HIGH);
}

TEST(init_sets_button_state_low) {
  db_init();
  ASSERT_EQ(db_button_state(), LOW);
}

TEST(debounce_delay_default) {
  db_init();
  ASSERT_EQ(db_debounce_delay(), DB_DEFAULT_DEBOUNCE_MS);
}

TEST(set_and_get_debounce_delay) {
  db_init();
  db_set_debounce_delay(100);
  ASSERT_EQ(db_debounce_delay(), 100);
  db_set_debounce_delay(75);
  ASSERT_EQ(db_debounce_delay(), 75);
}

TEST(toggle_on_high_transition) {
  db_init();
  int initial_led = db_led_state();
  
  // Simulate button press (LOW->HIGH transition after debounce)
  db_update(HIGH, 0);
  db_update(HIGH, 60);  // past debounce window (default 50ms)
  
  int toggled_led = db_led_state();
  ASSERT_NE(initial_led, toggled_led);
}

TEST(no_toggle_on_high_hold) {
  db_init();
  
  // First press and debounce
  db_update(HIGH, 0);
  db_update(HIGH, 60);
  int led_after_press = db_led_state();
  
  // Continue holding button (no new transition)
  db_update(HIGH, 150);
  int led_after_hold = db_led_state();
  
  ASSERT_EQ(led_after_press, led_after_hold);
}

TEST(last_change_ms_tracking) {
  db_init();
  
  db_update(LOW, 100);
  unsigned long change1 = db_last_change_ms();
  
  db_update(HIGH, 200);
  unsigned long change2 = db_last_change_ms();
  
  ASSERT_EQ(change1, 100);
  ASSERT_EQ(change2, 200);
}

TEST(debounce_ignores_noise) {
  db_init();
  int initial_led = db_led_state();
  
  // Quick noise pulse (shorter than debounce window)
  db_update(HIGH, 0);
  db_update(HIGH, 25);
  db_update(LOW, 30);   // back to LOW before debounce settles
  db_update(LOW, 100);  // well after debounce window
  
  // LED should not have toggled due to noise
  int final_led = db_led_state();
  ASSERT_EQ(initial_led, final_led);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
