#include "logic.h"
#include "test_runner.h"

TEST(init_state) {
  seq_init();
  ASSERT_EQ(seq_count(), 0);
  ASSERT_EQ(seq_current_led(), -1);
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(4, LOW);
}

TEST(apply_valid_step) {
  seq_init();
  ASSERT_TRUE(seq_apply(2));
  ASSERT_PIN(5, HIGH);
  ASSERT_EQ(seq_current_led(), 2);
  ASSERT_TRUE(seq_apply(3));
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(4, HIGH);
  ASSERT_EQ(seq_current_led(), 3);
}

TEST(apply_invalid_step) {
  seq_init();
  ASSERT_FALSE(seq_apply(4));
  ASSERT_FALSE(seq_apply(255));
}

TEST(advance_cycles_through_leds) {
  seq_init();
  seq_advance();
  ASSERT_PIN(7, HIGH);
  ASSERT_EQ(seq_count(), 1);
  seq_advance();
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(6, HIGH);
  ASSERT_EQ(seq_count(), 2);
  seq_advance();
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(5, HIGH);
  ASSERT_EQ(seq_count(), 3);
  seq_advance();
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(4, HIGH);
  ASSERT_EQ(seq_count(), 0);
  seq_advance();
  ASSERT_PIN(4, LOW);
  ASSERT_PIN(7, HIGH);
  ASSERT_EQ(seq_count(), 1);
}

TEST(all_off_clears_leds) {
  seq_init();
  seq_advance();
  ASSERT_PIN(7, HIGH);
  seq_all_off();
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(4, LOW);
  ASSERT_EQ(seq_current_led(), -1);
}

TEST(button_pressed_reflects_pin) {
  seq_init();
  pinMode(SEQ_PIN_BUTTON, OUTPUT);
  digitalWrite(SEQ_PIN_BUTTON, HIGH);
  ASSERT_TRUE(seq_button_pressed());
  digitalWrite(SEQ_PIN_BUTTON, LOW);
  ASSERT_FALSE(seq_button_pressed());
  pinMode(SEQ_PIN_BUTTON, INPUT);
}

TEST(poll_advances_only_when_pressed) {
  seq_init();
  pinMode(SEQ_PIN_BUTTON, OUTPUT);
  digitalWrite(SEQ_PIN_BUTTON, LOW);
  seq_poll();
  ASSERT_EQ(seq_count(), 0);
  ASSERT_PIN(7, LOW);
  digitalWrite(SEQ_PIN_BUTTON, HIGH);
  seq_poll();
  ASSERT_EQ(seq_count(), 1);
  ASSERT_PIN(7, HIGH);
  pinMode(SEQ_PIN_BUTTON, INPUT);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
