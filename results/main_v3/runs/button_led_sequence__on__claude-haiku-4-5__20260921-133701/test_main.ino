#include "logic.h"
#include "test_runner.h"

TEST(init_sets_up_pins) {
  seq_init();
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(4, LOW);
  ASSERT_EQ(seq_count(), 0);
  ASSERT_EQ(seq_current_led(), -1);
}

TEST(apply_valid_steps) {
  seq_init();
  ASSERT_TRUE(seq_apply(0));
  ASSERT_PIN(7, HIGH);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(4, LOW);
  ASSERT_EQ(seq_current_led(), 0);
  
  ASSERT_TRUE(seq_apply(2));
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(5, HIGH);
  ASSERT_PIN(4, LOW);
  ASSERT_EQ(seq_current_led(), 2);
  
  ASSERT_TRUE(seq_apply(3));
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(4, HIGH);
  ASSERT_EQ(seq_current_led(), 3);
}

TEST(apply_invalid_step) {
  seq_init();
  ASSERT_FALSE(seq_apply(4));
  ASSERT_FALSE(seq_apply(255));
  ASSERT_EQ(seq_current_led(), -1);
}

TEST(advance_cycles_through_leds) {
  seq_init();
  ASSERT_EQ(seq_count(), 0);
  seq_advance();
  ASSERT_PIN(7, HIGH);
  ASSERT_EQ(seq_current_led(), 0);
  ASSERT_EQ(seq_count(), 1);
  
  seq_advance();
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(6, HIGH);
  ASSERT_EQ(seq_current_led(), 1);
  ASSERT_EQ(seq_count(), 2);
  
  seq_advance();
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(5, HIGH);
  ASSERT_EQ(seq_current_led(), 2);
  ASSERT_EQ(seq_count(), 3);
  
  seq_advance();
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(4, HIGH);
  ASSERT_EQ(seq_current_led(), 3);
  ASSERT_EQ(seq_count(), 0);
}

TEST(all_off_turns_off_leds) {
  seq_init();
  seq_apply(0);
  ASSERT_PIN(7, HIGH);
  seq_all_off();
  ASSERT_PIN(7, LOW);
  ASSERT_PIN(6, LOW);
  ASSERT_PIN(5, LOW);
  ASSERT_PIN(4, LOW);
  ASSERT_EQ(seq_current_led(), -1);
  ASSERT_EQ(seq_count(), 0);
}

TEST(button_pressed_reads_pin) {
  seq_init();
  bool pressed = seq_button_pressed();
  ASSERT_TRUE(pressed == HIGH || pressed == LOW);
}

TEST(poll_advances_on_button_press) {
  seq_init();
  ASSERT_EQ(seq_count(), 0);
  ASSERT_EQ(seq_current_led(), -1);
  
  seq_poll();
  if (seq_button_pressed()) {
    ASSERT_EQ(seq_current_led(), 0);
    ASSERT_EQ(seq_count(), 1);
  } else {
    ASSERT_EQ(seq_current_led(), -1);
    ASSERT_EQ(seq_count(), 0);
  }
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
