#include "logic.h"
#include "test_runner.h"

TEST(init_state) {
  db_init();
  ASSERT_EQ(db_led_state(), HIGH);
  ASSERT_PIN(13, HIGH);
  ASSERT_EQ(db_button_state(), LOW);
  ASSERT_EQ(db_debounce_delay(), DB_DEFAULT_DEBOUNCE_MS);
}

TEST(press_toggles_led_after_debounce) {
  db_init();
  unsigned long t = 1000;
  db_update(HIGH, t);           // raw change, timer restarts
  ASSERT_EQ(db_led_state(), HIGH);
  ASSERT_EQ(db_button_state(), LOW);

  db_update(HIGH, t + 30);      // still within debounce window
  ASSERT_EQ(db_led_state(), HIGH);

  db_update(HIGH, t + 60);      // past debounce delay
  ASSERT_EQ(db_led_state(), LOW);
  ASSERT_PIN(13, LOW);
  ASSERT_EQ(db_button_state(), HIGH);
}

TEST(bounce_does_not_toggle) {
  db_init();
  unsigned long t = 2000;
  db_update(HIGH, t);
  db_update(LOW, t + 10);       // bounce within debounce window resets timer
  db_update(HIGH, t + 20);      // another bounce
  db_update(HIGH, t + 40);      // still before delay elapses from t+20
  ASSERT_EQ(db_led_state(), HIGH);
  ASSERT_EQ(db_button_state(), LOW);

  db_update(HIGH, t + 71);      // now past 50ms since t+20
  ASSERT_EQ(db_led_state(), LOW);
}

TEST(release_does_not_toggle_led) {
  db_init();
  unsigned long t = 3000;
  db_update(HIGH, t);
  db_update(HIGH, t + 60);      // press debounced, LED toggles to LOW
  ASSERT_EQ(db_led_state(), LOW);

  db_update(LOW, t + 70);       // release, timer restarts
  db_update(LOW, t + 130);      // past debounce
  ASSERT_EQ(db_button_state(), LOW);
  ASSERT_EQ(db_led_state(), LOW); // release should not toggle
}

TEST(custom_debounce_delay) {
  db_init();
  db_set_debounce_delay(100);
  ASSERT_EQ(db_debounce_delay(), 100);

  unsigned long t = 5000;
  db_update(HIGH, t);
  db_update(HIGH, t + 60);      // less than 100ms, no toggle yet
  ASSERT_EQ(db_led_state(), HIGH);

  db_update(HIGH, t + 101);     // past 100ms
  ASSERT_EQ(db_led_state(), LOW);
}

TEST(last_change_ms_tracks_raw_change) {
  db_init();
  unsigned long t = 4000;
  db_update(LOW, t);
  ASSERT_EQ(db_last_change_ms(), 0); // same as last_reading, no change yet

  db_update(HIGH, t + 5);
  ASSERT_EQ(db_last_change_ms(), t + 5);

  db_update(HIGH, t + 10);
  ASSERT_EQ(db_last_change_ms(), t + 5); // no raw change, timer unchanged
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
