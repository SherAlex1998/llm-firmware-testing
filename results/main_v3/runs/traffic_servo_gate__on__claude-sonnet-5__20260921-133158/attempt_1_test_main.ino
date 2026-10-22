#include "logic.h"
#include "test_runner.h"

TEST(init_sets_green_state) {
  tsg_init();
  ASSERT_EQ(tsg_state(), TSG_GREEN);
  ASSERT_PIN(11, HIGH);
  ASSERT_PIN(12, LOW);
  ASSERT_PIN(13, LOW);
  ASSERT_EQ(tsg_gate_angle(), TSG_GATE_OPEN_ANGLE);
  ASSERT_TRUE(tsg_gate_open());
}

TEST(set_state_yellow_closes_gate) {
  tsg_init();
  ASSERT_TRUE(tsg_set_state(TSG_YELLOW));
  ASSERT_EQ(tsg_state(), TSG_YELLOW);
  ASSERT_PIN(11, LOW);
  ASSERT_PIN(12, HIGH);
  ASSERT_PIN(13, LOW);
  ASSERT_EQ(tsg_gate_angle(), TSG_GATE_CLOSED_ANGLE);
  ASSERT_FALSE(tsg_gate_open());
}

TEST(set_state_red_closes_gate) {
  tsg_init();
  ASSERT_TRUE(tsg_set_state(TSG_RED));
  ASSERT_EQ(tsg_state(), TSG_RED);
  ASSERT_PIN(11, LOW);
  ASSERT_PIN(12, LOW);
  ASSERT_PIN(13, HIGH);
  ASSERT_EQ(tsg_gate_angle(), TSG_GATE_CLOSED_ANGLE);
  ASSERT_FALSE(tsg_gate_open());
}

TEST(set_state_off_leaves_leds_off_and_gate_unchanged) {
  tsg_init();
  tsg_set_state(TSG_GREEN);
  int angle_before = tsg_gate_angle();
  ASSERT_TRUE(tsg_set_state(TSG_OFF));
  ASSERT_EQ(tsg_state(), TSG_OFF);
  ASSERT_PIN(11, LOW);
  ASSERT_PIN(12, LOW);
  ASSERT_PIN(13, LOW);
  ASSERT_EQ(tsg_gate_angle(), angle_before);
}

TEST(set_state_invalid_returns_false) {
  tsg_init();
  ASSERT_FALSE(tsg_set_state(TSG_STATE_COUNT));
  ASSERT_EQ(tsg_state(), TSG_GREEN);
}

TEST(next_state_cycles_through_all_states) {
  ASSERT_EQ(tsg_next_state(TSG_GREEN), TSG_YELLOW);
  ASSERT_EQ(tsg_next_state(TSG_YELLOW), TSG_RED);
  ASSERT_EQ(tsg_next_state(TSG_RED), TSG_OFF);
  ASSERT_EQ(tsg_next_state(TSG_OFF), TSG_GREEN);
  ASSERT_EQ(tsg_next_state(TSG_STATE_COUNT), TSG_GREEN);
}

TEST(next_advances_current_state) {
  tsg_init();
  ASSERT_EQ(tsg_state(), TSG_GREEN);
  tsg_next();
  ASSERT_EQ(tsg_state(), TSG_YELLOW);
  tsg_next();
  ASSERT_EQ(tsg_state(), TSG_RED);
  tsg_next();
  ASSERT_EQ(tsg_state(), TSG_OFF);
  tsg_next();
  ASSERT_EQ(tsg_state(), TSG_GREEN);
}

TEST(state_duration_valid_and_invalid) {
  ASSERT_EQ(tsg_state_duration_ms(TSG_GREEN), 5000);
  ASSERT_EQ(tsg_state_duration_ms(TSG_OFF), 5000);
  ASSERT_EQ(tsg_state_duration_ms(TSG_STATE_COUNT), 0);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
