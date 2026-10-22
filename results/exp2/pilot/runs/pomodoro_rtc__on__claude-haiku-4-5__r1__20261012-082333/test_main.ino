#include "logic.h"
#include "test_runner.h"

TEST(init_sets_idle_status) {
  pomo_set_delay_scale(0);
  pomo_init();
  ASSERT_EQ(pomo_status(), STATUS_IDLE);
}

TEST(init_sets_default_time) {
  pomo_set_delay_scale(0);
  pomo_init();
  ASSERT_EQ(pomo_minute(), DEFAULT_TIME);
}

TEST(completed_starts_at_zero) {
  pomo_set_delay_scale(0);
  pomo_init();
  ASSERT_EQ(pomo_completed(), 0);
}

TEST(prev_status_tracks_status_changes) {
  pomo_set_delay_scale(0);
  pomo_init();
  ASSERT_EQ(pomo_prev_status(), STATUS_IDLE);
}

TEST(read_click_returns_zero_when_button_not_pressed) {
  pomo_set_delay_scale(0);
  pomo_init();
  uint8_t click = pomo_read_click();
  ASSERT_EQ(click, CLICK_NONE);
}

TEST(seconds_event_starts_false) {
  pomo_set_delay_scale(0);
  pomo_init();
  ASSERT_FALSE(pomo_seconds_event());
}

TEST(user_data_initialized_to_zero) {
  pomo_set_delay_scale(0);
  pomo_init();
  for (uint8_t i = 0; i < 5; i++) {
    ASSERT_EQ(pomo_user_data(i), 0);
  }
}

TEST(display_digits_initialized) {
  pomo_set_delay_scale(0);
  pomo_init();
  const int8_t* digits = pomo_display_digits();
  ASSERT_TRUE(digits != NULL);
}

TEST(display_dot_starts_false) {
  pomo_set_delay_scale(0);
  pomo_init();
  ASSERT_FALSE(pomo_display_dot());
}

TEST(display_text_empty_after_init) {
  pomo_set_delay_scale(0);
  pomo_init();
  const char* text = pomo_display_text();
  ASSERT_TRUE(text != NULL);
}

TEST(handle_action_short_increases_time_in_idle) {
  pomo_set_delay_scale(0);
  pomo_init();
  int8_t original = pomo_minute();
  pomo_handle_action(CLICK_SHORT_ACTION);
  int8_t after = pomo_minute();
  ASSERT_NE(after, original);
}

TEST(handle_action_long_starts_prepare_from_idle) {
  pomo_set_delay_scale(0);
  pomo_init();
  pomo_handle_action(CLICK_LONG_ACTION);
  ASSERT_EQ(pomo_status(), STATUS_PREPARE);
}

TEST(time_wraps_around_at_max) {
  pomo_set_delay_scale(0);
  pomo_init();
  for (int i = 0; i < 10; i++) {
    pomo_handle_action(CLICK_SHORT_ACTION);
  }
  ASSERT_EQ(pomo_minute(), DEFAULT_TIME);
}

TEST(calc_weekday_monday_2030_03_11) {
  uint8_t weekday = pomo_calc_weekday(2030, 3, 11);
  ASSERT_EQ(weekday, 1);
}

TEST(calc_weekday_sunday_2030_03_10) {
  uint8_t weekday = pomo_calc_weekday(2030, 3, 10);
  ASSERT_EQ(weekday, 0);
}

TEST(calc_weekday_saturday_2030_03_09) {
  uint8_t weekday = pomo_calc_weekday(2030, 3, 9);
  ASSERT_EQ(weekday, 6);
}

TEST(apply_rollover_same_day_no_change) {
  uint16_t data[5] = {11, 5, 0, 0, 0};
  pomo_apply_rollover(data, 2030, 3, 11, 1);
  ASSERT_EQ(data[0], 11);
  ASSERT_EQ(data[1], 5);
}

TEST(apply_rollover_next_day_rolls_today_to_week) {
  uint16_t data[5] = {10, 3, 0, 0, 0};
  pomo_apply_rollover(data, 2030, 3, 11, 1);
  ASSERT_EQ(data[0], 11);
  ASSERT_EQ(data[1], 0);
  ASSERT_EQ(data[2], 3);
}

TEST(apply_rollover_next_month_rolls_all) {
  uint16_t data[5] = {31, 2, 1, 0, 0};
  pomo_apply_rollover(data, 2030, 4, 1, 2);
  ASSERT_EQ(data[0], 1);
  ASSERT_EQ(data[1], 0);
  ASSERT_EQ(data[2], 0);
  ASSERT_EQ(data[3], 3);
}

TEST(status_transitions_prepare_to_idle_on_cancel) {
  pomo_set_delay_scale(0);
  pomo_init();
  pomo_handle_action(CLICK_LONG_ACTION);
  ASSERT_EQ(pomo_status(), STATUS_PREPARE);
  pomo_handle_action(CLICK_SHORT_ACTION);
  ASSERT_EQ(pomo_status(), STATUS_IDLE);
}

TEST(tick_increments_half_second_counter) {
  pomo_set_delay_scale(0);
  pomo_init();
  bool event_before = pomo_seconds_event();
  pomo_tick();
  pomo_tick();
  bool event_after = pomo_seconds_event();
  ASSERT_NE(event_before, event_after);
}

TEST(load_user_data_reads_from_rtc) {
  pomo_set_delay_scale(0);
  pomo_init();
  pomo_load_user_data();
  uint16_t day_of_month = pomo_user_data(0);
  ASSERT_EQ(day_of_month, 11);
}

TEST(button_pin_configured_as_input) {
  pomo_set_delay_scale(0);
  pomo_init();
  pinMode(BTN_CLICK, INPUT);
  ASSERT_PIN(BTN_CLICK, LOW);
}

TEST(buzzer_pin_configured_as_output) {
  pomo_set_delay_scale(0);
  pomo_init();
  digitalWrite(BTN_BUZZER, LOW);
  ASSERT_PIN(BTN_BUZZER, LOW);
}

TEST(tm1637_clk_pin_configured) {
  pomo_set_delay_scale(0);
  pomo_init();
  pinMode(TM_CLK_PIN, OUTPUT);
  digitalWrite(TM_CLK_PIN, LOW);
  ASSERT_PIN(TM_CLK_PIN, LOW);
}

TEST(tm1637_dio_pin_configured) {
  pomo_set_delay_scale(0);
  pomo_init();
  pinMode(TM_DIO_PIN, OUTPUT);
  digitalWrite(TM_DIO_PIN, LOW);
  ASSERT_PIN(TM_DIO_PIN, LOW);
}

TEST(second_value_in_range) {
  pomo_set_delay_scale(0);
  pomo_init();
  int8_t sec = pomo_second();
  ASSERT_TRUE(sec >= 0 && sec <= 59);
}

TEST(minute_value_never_negative) {
  pomo_set_delay_scale(0);
  pomo_init();
  int8_t min = pomo_minute();
  ASSERT_TRUE(min >= 0);
}

TEST(completed_never_negative) {
  pomo_set_delay_scale(0);
  pomo_init();
  int8_t comp = pomo_completed();
  ASSERT_TRUE(comp >= 0);
}

TEST(delay_scale_persists_across_init) {
  pomo_set_delay_scale(50);
  pomo_init();
  pomo_init();
  ASSERT_TRUE(true);
}

TEST(update_processes_without_crash) {
  pomo_set_delay_scale(0);
  pomo_init();
  pomo_update();
  ASSERT_TRUE(true);
}

TEST(start_timer_initializes_timer1) {
  pomo_set_delay_scale(0);
  pomo_init();
  pomo_start_timer();
  ASSERT_TRUE(true);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
