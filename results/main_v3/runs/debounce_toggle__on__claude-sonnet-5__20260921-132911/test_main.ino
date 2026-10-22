#include "logic.h"
#include "test_runner.h"

TEST(init_sets_led_high_and_button_low) {
  db_init();
  ASSERT_EQ(db_led_state(), HIGH);
  ASSERT_PIN(13, HIGH);
  ASSERT_EQ(db_button_state(), LOW);
  ASSERT_EQ(db_debounce_delay(), (unsigned long)DB_DEFAULT_DEBOUNCE_MS);
}

TEST(short_press_below_delay_does_not_toggle) {
  db_init();
  int led_before = db_led_state();
  db_update(HIGH, 10);   // reading changes at t=10
  db_update(HIGH, 30);   // only 20ms later, less than 50ms debounce delay
  ASSERT_EQ(db_led_state(), led_before);
  ASSERT_EQ(db_button_state(), LOW);
}

TEST(press_held_past_delay_toggles_led) {
  db_init();
  int led_before = db_led_state();
  db_update(HIGH, 0);    // reading changes at t=0
  db_update(HIGH, 60);   // 60ms later, past the 50ms debounce delay
  ASSERT_EQ(db_button_state(), HIGH);
  ASSERT_EQ(db_led_state(), !led_before);
  ASSERT_PIN(13, db_led_state());
}

TEST(release_does_not_toggle_again) {
  db_init();
  int led_before = db_led_state();
  db_update(HIGH, 0);
  db_update(HIGH, 60);   // press registered, LED toggled once
  int led_after_press = db_led_state();
  ASSERT_EQ(led_after_press, !led_before);

  db_update(LOW, 100);   // release, reading changes
  db_update(LOW, 160);   // held past debounce delay
  ASSERT_EQ(db_button_state(), LOW);
  ASSERT_EQ(db_led_state(), led_after_press);  // no toggle on release
}

TEST(set_debounce_delay_changes_reported_value) {
  db_init();
  db_set_debounce_delay(100);
  ASSERT_EQ(db_debounce_delay(), 100);

  int led_before = db_led_state();
  db_update(HIGH, 0);
  db_update(HIGH, 60);   // less than new 100ms delay, should not toggle
  ASSERT_EQ(db_led_state(), led_before);
  ASSERT_EQ(db_button_state(), LOW);

  db_update(HIGH, 110);  // now past 100ms since t=0
  ASSERT_EQ(db_led_state(), !led_before);
  ASSERT_EQ(db_button_state(), HIGH);
}

TEST(last_change_ms_tracks_raw_reading_change) {
  db_init();
  ASSERT_EQ(db_last_change_ms(), 0);
  db_update(LOW, 5);     // no change from initial LOW reading
  ASSERT_EQ(db_last_change_ms(), 0);
  db_update(HIGH, 20);   // reading changes here
  ASSERT_EQ(db_last_change_ms(), 20);
  db_update(HIGH, 25);   // no change, timer should not reset
  ASSERT_EQ(db_last_change_ms(), 20);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
