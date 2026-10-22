#include "logic.h"
#include "test_runner.h"

TEST(init_sets_default_states_and_led_high) {
  db_init();
  ASSERT_EQ(db_led_state(), HIGH);
  ASSERT_EQ(db_button_state(), LOW);
  ASSERT_EQ(db_debounce_delay(), DB_DEFAULT_DEBOUNCE_MS);
  ASSERT_EQ(db_last_change_ms(), 0);
  ASSERT_PIN(DB_PIN_LED, HIGH);
}

TEST(press_toggles_led_only_after_debounce_delay) {
  db_init();

  db_update(HIGH, 10);
  ASSERT_EQ(db_last_change_ms(), 10);
  ASSERT_EQ(db_button_state(), LOW);
  ASSERT_EQ(db_led_state(), HIGH);
  ASSERT_PIN(DB_PIN_LED, HIGH);

  db_update(HIGH, 60);
  ASSERT_EQ(db_button_state(), LOW);
  ASSERT_EQ(db_led_state(), HIGH);

  db_update(HIGH, 61);
  ASSERT_EQ(db_button_state(), HIGH);
  ASSERT_EQ(db_led_state(), LOW);
  ASSERT_PIN(DB_PIN_LED, LOW);
}

TEST(bounce_restarts_debounce_timer) {
  db_init();

  db_update(HIGH, 10);
  db_update(LOW, 30);
  ASSERT_EQ(db_last_change_ms(), 30);

  db_update(HIGH, 45);
  ASSERT_EQ(db_last_change_ms(), 45);
  db_update(HIGH, 95);
  ASSERT_EQ(db_button_state(), LOW);
  ASSERT_EQ(db_led_state(), HIGH);

  db_update(HIGH, 96);
  ASSERT_EQ(db_button_state(), HIGH);
  ASSERT_EQ(db_led_state(), LOW);
  ASSERT_PIN(DB_PIN_LED, LOW);
}

TEST(release_does_not_toggle_led_and_next_press_toggles_again) {
  db_init();

  db_update(HIGH, 1);
  db_update(HIGH, 52);
  ASSERT_EQ(db_button_state(), HIGH);
  ASSERT_EQ(db_led_state(), LOW);

  db_update(LOW, 60);
  db_update(LOW, 111);
  ASSERT_EQ(db_button_state(), LOW);
  ASSERT_EQ(db_led_state(), LOW);
  ASSERT_PIN(DB_PIN_LED, LOW);

  db_update(HIGH, 120);
  db_update(HIGH, 171);
  ASSERT_EQ(db_button_state(), HIGH);
  ASSERT_EQ(db_led_state(), HIGH);
  ASSERT_PIN(DB_PIN_LED, HIGH);
}

TEST(custom_debounce_delay_is_used) {
  db_init();
  db_set_debounce_delay(10);
  ASSERT_EQ(db_debounce_delay(), 10);

  db_update(HIGH, 100);
  db_update(HIGH, 110);
  ASSERT_EQ(db_button_state(), LOW);
  ASSERT_EQ(db_led_state(), HIGH);

  db_update(HIGH, 111);
  ASSERT_EQ(db_button_state(), HIGH);
  ASSERT_EQ(db_led_state(), LOW);
  ASSERT_PIN(DB_PIN_LED, LOW);
}

TEST(timestamp_arithmetic_handles_millis_wraparound) {
  db_init();
  db_set_debounce_delay(20);

  db_update(HIGH, 0xFFFFFFF0UL);
  ASSERT_EQ(db_last_change_ms(), 0xFFFFFFF0UL);

  db_update(HIGH, 5);
  ASSERT_EQ(db_button_state(), HIGH);
  ASSERT_EQ(db_led_state(), LOW);
  ASSERT_PIN(DB_PIN_LED, LOW);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
