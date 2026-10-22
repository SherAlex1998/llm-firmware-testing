#include "logic.h"
#include "test_runner.h"

// Helper: fresh deterministic init with no blocking delays.
static void init_fresh() {
  pomo_set_delay_scale(0);
  pomo_init();
}

// Helper: from IDLE, send a long click (Prepare), tick through the 10 s
// countdown, then let pomo_update() fire the transition into RUN_WORK.
static void advance_to_running() {
  pomo_handle_action(CLICK_LONG_ACTION);
  for (int i = 0; i < 40 && !pomo_seconds_event(); i++) {
    pomo_tick();
  }
  pomo_update();
}

TEST(init_defaults) {
  init_fresh();
  ASSERT_EQ(pomo_status(), STATUS_IDLE);
  ASSERT_EQ(pomo_prev_status(), STATUS_IDLE);
  ASSERT_EQ(pomo_minute(), DEFAULT_TIME);
  ASSERT_EQ(pomo_second(), 59);
  ASSERT_EQ(pomo_completed(), 0);
  ASSERT_FALSE(pomo_seconds_event());
  // RTC starts at 2030-03-11 (Monday), RAM empty -> rollover from all-zero stats.
  ASSERT_EQ(pomo_user_data(0), 11);
  ASSERT_EQ(pomo_user_data(1), 0);
  ASSERT_EQ(pomo_user_data(2), 0);
  ASSERT_EQ(pomo_user_data(3), 0);
  ASSERT_EQ(pomo_user_data(4), 0);
}

TEST(calc_weekday_known_monday) {
  // 2030-03-11 is a Monday (per module spec); Sunday == 0.
  ASSERT_EQ(pomo_calc_weekday(2030, 3, 11), 1);
  ASSERT_EQ(pomo_calc_weekday(2030, 3, 10), 0);
}

TEST(apply_rollover_same_day_noop) {
  uint16_t data[5] = {15, 2, 3, 4, 5};
  pomo_apply_rollover(data, 2030, 3, 15, 1);
  ASSERT_EQ(data[0], 15);
  ASSERT_EQ(data[1], 2);
  ASSERT_EQ(data[2], 3);
  ASSERT_EQ(data[3], 4);
  ASSERT_EQ(data[4], 5);
}

TEST(apply_rollover_next_day_same_week) {
  uint16_t data[5] = {10, 7, 2, 3, 4};
  pomo_apply_rollover(data, 2030, 3, 11, 1); // today is Monday
  ASSERT_EQ(data[0], 11);
  ASSERT_EQ(data[1], 0);
  ASSERT_EQ(data[2], 9);
  ASSERT_EQ(data[3], 3);
  ASSERT_EQ(data[4], 4);
}

TEST(apply_rollover_next_day_new_week) {
  uint16_t data[5] = {1, 3, 5, 2, 1};
  pomo_apply_rollover(data, 2030, 3, 11, 1); // today is Monday
  ASSERT_EQ(data[0], 11);
  ASSERT_EQ(data[1], 0);
  ASSERT_EQ(data[2], 0);
  ASSERT_EQ(data[3], 10);
  ASSERT_EQ(data[4], 1);
}

TEST(apply_rollover_new_month) {
  uint16_t data[5] = {25, 4, 6, 8, 10};
  pomo_apply_rollover(data, 2030, 4, 5, 3);
  ASSERT_EQ(data[0], 5);
  ASSERT_EQ(data[1], 0);
  ASSERT_EQ(data[2], 0);
  ASSERT_EQ(data[3], 0);
  ASSERT_EQ(data[4], 28);
}

TEST(increase_time_wraps_at_max) {
  init_fresh();
  ASSERT_EQ(pomo_minute(), 25);
  for (int i = 0; i < 7; i++) pomo_handle_action(CLICK_SHORT_ACTION);
  ASSERT_EQ(pomo_minute(), 60);
  pomo_handle_action(CLICK_SHORT_ACTION);
  ASSERT_EQ(pomo_minute(), DEFAULT_TIME);
}

TEST(long_click_idle_prepares) {
  init_fresh();
  pomo_handle_action(CLICK_LONG_ACTION);
  ASSERT_EQ(pomo_status(), STATUS_PREPARE);
  ASSERT_EQ(pomo_prev_status(), STATUS_IDLE);
  ASSERT_EQ(pomo_second(), 10);
}

TEST(prepare_counts_down_to_running) {
  init_fresh();
  advance_to_running();
  ASSERT_EQ(pomo_status(), STATUS_RUN_WORK);
  ASSERT_EQ(pomo_prev_status(), STATUS_PREPARE);
  ASSERT_EQ(pomo_minute(), DEFAULT_TIME - 1);
  ASSERT_EQ(pomo_second(), 59);
}

TEST(pause_and_resume_work) {
  init_fresh();
  advance_to_running();
  ASSERT_EQ(pomo_status(), STATUS_RUN_WORK);

  pomo_handle_action(CLICK_SHORT_ACTION);
  ASSERT_EQ(pomo_status(), STATUS_RUN_PAUSE);
  ASSERT_EQ(pomo_prev_status(), STATUS_RUN_WORK);

  pomo_handle_action(CLICK_SHORT_ACTION);
  ASSERT_EQ(pomo_status(), STATUS_RUN_WORK);
  ASSERT_EQ(pomo_second(), 59);
}

TEST(long_click_work_cancels) {
  init_fresh();
  advance_to_running();
  ASSERT_EQ(pomo_status(), STATUS_RUN_WORK);

  pomo_handle_action(CLICK_LONG_ACTION);
  ASSERT_EQ(pomo_status(), STATUS_IDLE);
  ASSERT_EQ(pomo_minute(), DEFAULT_TIME);
  ASSERT_EQ(pomo_completed(), 0);
}

TEST(display_shadow_after_init) {
  init_fresh();
  ASSERT_FALSE(pomo_display_dot());
  ASSERT_EQ((int)strcmp(pomo_display_text(), "HELL0"), 0);
  const int8_t* d = pomo_display_digits();
  ASSERT_EQ(d[0], 0x7f);
  ASSERT_EQ(d[1], 0x7f);
  ASSERT_EQ(d[2], 0x7f);
  ASSERT_EQ(d[3], 0x7f);
}

TEST(read_click_none_when_button_released) {
  init_fresh();
  ASSERT_EQ(pomo_read_click(), CLICK_NONE);
}

TEST(user_data_index_wraps_mod_5) {
  init_fresh();
  ASSERT_EQ(pomo_user_data(0), pomo_user_data(5));
  ASSERT_EQ(pomo_user_data(1), pomo_user_data(6));
}

TEST(load_user_data_keeps_stats_stable) {
  init_fresh();
  uint16_t before0 = pomo_user_data(0);
  uint16_t before1 = pomo_user_data(1);
  pomo_load_user_data();
  ASSERT_EQ(pomo_user_data(0), before0);
  ASSERT_EQ(pomo_user_data(1), before1);
}

TEST(start_timer_does_not_crash_or_change_state_immediately) {
  init_fresh();
  pomo_start_timer();
  ASSERT_EQ(pomo_status(), STATUS_IDLE);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
