#include "logic.h"
#include "test_runner.h"

TEST(init_sets_idle_defaults) {
  pomo_set_delay_scale(0);
  pomo_init();

  ASSERT_EQ(pomo_status(), STATUS_IDLE);
  ASSERT_EQ(pomo_prev_status(), STATUS_IDLE);
  ASSERT_EQ(pomo_minute(), DEFAULT_TIME);
  ASSERT_EQ(pomo_second(), 59);
  ASSERT_EQ(pomo_completed(), 0);
  ASSERT_FALSE(pomo_seconds_event());
  ASSERT_FALSE(pomo_display_dot());
  ASSERT_TRUE(pomo_display_text()[0] == 'H');
  ASSERT_EQ(pomo_user_data(0), 11);
}

TEST(idle_short_action_increases_time_and_redraws) {
  pomo_set_delay_scale(0);
  pomo_init();

  pomo_handle_action(CLICK_SHORT_ACTION);
  pomo_update();

  const int8_t* digits = pomo_display_digits();
  ASSERT_EQ(pomo_status(), STATUS_IDLE);
  ASSERT_EQ(pomo_minute(), 30);
  ASSERT_EQ(digits[0], 3);
  ASSERT_EQ(digits[1], 0);
  ASSERT_EQ(digits[2], 0x40);
  ASSERT_EQ(digits[3], 0);
  ASSERT_FALSE(pomo_display_dot());
}

TEST(long_action_enters_prepare_mode) {
  pomo_set_delay_scale(0);
  pomo_init();

  pomo_handle_action(CLICK_LONG_ACTION);
  pomo_update();

  const int8_t* digits = pomo_display_digits();
  ASSERT_EQ(pomo_status(), STATUS_PREPARE);
  ASSERT_EQ(pomo_prev_status(), STATUS_IDLE);
  ASSERT_EQ(pomo_minute(), DEFAULT_TIME);
  ASSERT_EQ(pomo_second(), 10);
  ASSERT_EQ(digits[0], 0x40);
  ASSERT_EQ(digits[1], 0x40);
  ASSERT_EQ(digits[2], 1);
  ASSERT_EQ(digits[3], 0);
  ASSERT_TRUE(pomo_display_dot());
}

TEST(prepare_countdown_starts_work_timer) {
  pomo_set_delay_scale(0);
  pomo_init();

  pomo_handle_action(CLICK_LONG_ACTION);
  for (uint8_t i = 0; i < 22; i++) {
    pomo_tick();
  }

  ASSERT_TRUE(pomo_seconds_event());
  ASSERT_EQ(pomo_second(), 59);

  pomo_update();

  ASSERT_EQ(pomo_status(), STATUS_RUN_WORK);
  ASSERT_EQ(pomo_prev_status(), STATUS_PREPARE);
  ASSERT_EQ(pomo_minute(), 24);
  ASSERT_EQ(pomo_second(), 59);
  ASSERT_FALSE(pomo_seconds_event());
  ASSERT_FALSE(pomo_display_dot());
}

TEST(work_pause_and_resume_restore_seconds) {
  pomo_set_delay_scale(0);
  pomo_init();

  pomo_handle_action(CLICK_LONG_ACTION);
  for (uint8_t i = 0; i < 22; i++) {
    pomo_tick();
  }
  pomo_update();

  pomo_tick();
  pomo_tick();
  ASSERT_EQ(pomo_second(), 58);

  pomo_handle_action(CLICK_SHORT_ACTION);
  ASSERT_EQ(pomo_status(), STATUS_RUN_PAUSE);
  ASSERT_EQ(pomo_prev_status(), STATUS_RUN_WORK);
  ASSERT_EQ(pomo_second(), 59);

  pomo_handle_action(CLICK_SHORT_ACTION);
  ASSERT_EQ(pomo_status(), STATUS_RUN_WORK);
  ASSERT_EQ(pomo_prev_status(), STATUS_RUN_PAUSE);
  ASSERT_EQ(pomo_second(), 58);
}

TEST(cancel_prepare_returns_to_idle) {
  pomo_set_delay_scale(0);
  pomo_init();

  pomo_handle_action(CLICK_LONG_ACTION);
  ASSERT_EQ(pomo_status(), STATUS_PREPARE);

  pomo_handle_action(CLICK_SHORT_ACTION);

  ASSERT_EQ(pomo_status(), STATUS_IDLE);
  ASSERT_EQ(pomo_prev_status(), STATUS_PREPARE);
  ASSERT_EQ(pomo_minute(), DEFAULT_TIME);
  ASSERT_FALSE(pomo_display_dot());
}

TEST(tick_requires_two_half_seconds) {
  pomo_set_delay_scale(0);
  pomo_init();

  pomo_tick();
  ASSERT_EQ(pomo_second(), 59);
  ASSERT_FALSE(pomo_seconds_event());

  pomo_tick();
  ASSERT_EQ(pomo_second(), 58);
  ASSERT_FALSE(pomo_seconds_event());
}

TEST(released_button_produces_no_click) {
  pomo_set_delay_scale(0);
  pomo_init();

  ASSERT_EQ(pomo_read_click(), CLICK_NONE);
  ASSERT_EQ(pomo_read_click(), CLICK_NONE);
}

TEST(calc_weekday_matches_known_dates) {
  pomo_set_delay_scale(0);
  pomo_init();

  ASSERT_EQ(pomo_calc_weekday(2030, 3, 11), 1);
  ASSERT_EQ(pomo_calc_weekday(2030, 3, 17), 0);
  ASSERT_EQ(pomo_calc_weekday(2000, 1, 1), 6);
  ASSERT_EQ(pomo_calc_weekday(2024, 2, 29), 4);
}

TEST(rollover_same_day_leaves_statistics_unchanged) {
  pomo_set_delay_scale(0);
  pomo_init();

  uint16_t data[5] = {11, 2, 3, 4, 5};
  pomo_apply_rollover(data, 2030, 3, 11, 1);

  ASSERT_EQ(data[0], 11);
  ASSERT_EQ(data[1], 2);
  ASSERT_EQ(data[2], 3);
  ASSERT_EQ(data[3], 4);
  ASSERT_EQ(data[4], 5);
}

TEST(rollover_next_day_moves_today_to_week) {
  pomo_set_delay_scale(0);
  pomo_init();

  uint16_t data[5] = {11, 2, 3, 4, 5};
  pomo_apply_rollover(data, 2030, 3, 12, 2);

  ASSERT_EQ(data[0], 12);
  ASSERT_EQ(data[1], 0);
  ASSERT_EQ(data[2], 5);
  ASSERT_EQ(data[3], 4);
  ASSERT_EQ(data[4], 5);
}

TEST(rollover_week_boundary_moves_week_to_month) {
  pomo_set_delay_scale(0);
  pomo_init();

  uint16_t data[5] = {16, 2, 3, 4, 5};
  pomo_apply_rollover(data, 2030, 3, 17, 0);

  ASSERT_EQ(data[0], 17);
  ASSERT_EQ(data[1], 0);
  ASSERT_EQ(data[2], 0);
  ASSERT_EQ(data[3], 9);
  ASSERT_EQ(data[4], 5);
}

TEST(rollover_month_boundary_moves_month_to_total) {
  pomo_set_delay_scale(0);
  pomo_init();

  uint16_t data[5] = {31, 2, 3, 4, 5};
  pomo_apply_rollover(data, 2030, 4, 1, 1);

  ASSERT_EQ(data[0], 1);
  ASSERT_EQ(data[1], 0);
  ASSERT_EQ(data[2], 0);
  ASSERT_EQ(data[3], 0);
  ASSERT_EQ(data[4], 14);
}

TEST(completing_work_enters_rest_and_records_statistic) {
  pomo_set_delay_scale(0);
  pomo_init();

  pomo_handle_action(CLICK_LONG_ACTION);
  for (uint8_t i = 0; i < 22; i++) {
    pomo_tick();
  }
  pomo_update();

  for (uint8_t minute = 0; minute < 25; minute++) {
    for (uint8_t second = 0; second < 60; second++) {
      pomo_tick();
      pomo_tick();
    }
    ASSERT_TRUE(pomo_seconds_event());
    pomo_update();
  }

  ASSERT_EQ(pomo_status(), STATUS_RUN_REST);
  ASSERT_EQ(pomo_prev_status(), STATUS_RUN_WORK);
  ASSERT_EQ(pomo_completed(), 1);
  ASSERT_EQ(pomo_user_data(1), 1);
  ASSERT_EQ(pomo_minute(), 4);
  ASSERT_EQ(pomo_second(), 59);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
