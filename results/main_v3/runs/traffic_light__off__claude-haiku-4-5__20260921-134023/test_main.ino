#include "logic.h"
#include "test_runner.h"

TEST(init_sets_red) {
  tl_init();
  ASSERT_EQ(tl_state(), TL_RED);
  ASSERT_PIN(TL_PIN_RED, HIGH);
  ASSERT_PIN(TL_PIN_YELLOW, LOW);
  ASSERT_PIN(TL_PIN_GREEN, LOW);
}

TEST(set_state_red) {
  tl_init();
  tl_set_state(TL_RED);
  ASSERT_EQ(tl_state(), TL_RED);
  ASSERT_PIN(TL_PIN_RED, HIGH);
  ASSERT_PIN(TL_PIN_YELLOW, LOW);
  ASSERT_PIN(TL_PIN_GREEN, LOW);
}

TEST(set_state_red_yellow) {
  tl_init();
  tl_set_state(TL_RED_YELLOW);
  ASSERT_EQ(tl_state(), TL_RED_YELLOW);
  ASSERT_PIN(TL_PIN_RED, HIGH);
  ASSERT_PIN(TL_PIN_YELLOW, HIGH);
  ASSERT_PIN(TL_PIN_GREEN, LOW);
}

TEST(set_state_green) {
  tl_init();
  tl_set_state(TL_GREEN);
  ASSERT_EQ(tl_state(), TL_GREEN);
  ASSERT_PIN(TL_PIN_RED, LOW);
  ASSERT_PIN(TL_PIN_YELLOW, LOW);
  ASSERT_PIN(TL_PIN_GREEN, HIGH);
}

TEST(set_state_green_blink) {
  tl_init();
  tl_set_state(TL_GREEN_BLINK);
  ASSERT_EQ(tl_state(), TL_GREEN_BLINK);
  ASSERT_PIN(TL_PIN_RED, LOW);
  ASSERT_PIN(TL_PIN_YELLOW, LOW);
  ASSERT_PIN(TL_PIN_GREEN, LOW);
}

TEST(set_state_yellow) {
  tl_init();
  tl_set_state(TL_YELLOW);
  ASSERT_EQ(tl_state(), TL_YELLOW);
  ASSERT_PIN(TL_PIN_RED, LOW);
  ASSERT_PIN(TL_PIN_YELLOW, HIGH);
  ASSERT_PIN(TL_PIN_GREEN, LOW);
}

TEST(next_state_sequence) {
  ASSERT_EQ(tl_next_state(TL_RED), TL_RED_YELLOW);
  ASSERT_EQ(tl_next_state(TL_RED_YELLOW), TL_GREEN);
  ASSERT_EQ(tl_next_state(TL_GREEN), TL_GREEN_BLINK);
  ASSERT_EQ(tl_next_state(TL_GREEN_BLINK), TL_YELLOW);
  ASSERT_EQ(tl_next_state(TL_YELLOW), TL_RED);
}

TEST(advance_cycles_states) {
  tl_init();
  ASSERT_EQ(tl_state(), TL_RED);
  
  tl_advance();
  ASSERT_EQ(tl_state(), TL_RED_YELLOW);
  
  tl_advance();
  ASSERT_EQ(tl_state(), TL_GREEN);
  
  tl_advance();
  ASSERT_EQ(tl_state(), TL_GREEN_BLINK);
  
  tl_advance();
  ASSERT_EQ(tl_state(), TL_YELLOW);
  
  tl_advance();
  ASSERT_EQ(tl_state(), TL_RED);
}

TEST(state_durations) {
  ASSERT_EQ(tl_state_duration_ms(TL_RED), 5000);
  ASSERT_EQ(tl_state_duration_ms(TL_RED_YELLOW), 1500);
  ASSERT_EQ(tl_state_duration_ms(TL_GREEN), 5000);
  ASSERT_EQ(tl_state_duration_ms(TL_GREEN_BLINK), 2000);
  ASSERT_EQ(tl_state_duration_ms(TL_YELLOW), 2000);
}

TEST(blink_green) {
  tl_init();
  tl_set_state(TL_GREEN_BLINK);
  tl_blink_green(2, 50);
  ASSERT_PIN(TL_PIN_GREEN, LOW);
}

void setup() { 
  Serial.begin(9600); 
  RUN_ALL_TESTS(); 
}

void loop() {}
