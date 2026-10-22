#include "logic.h"
#include "test_runner.h"

// Helper: from IDLE, long-press action starts PREPARE, then full 10s countdown
// transitions into RUN_WORK (minute 24, second 59). Used by several tests.
static void enter_run_work() {
  pomo_handle_action(CLICK_LONG_ACTION);   // IDLE -> PREPARE (second = 10)
  for (int i = 0; i < 22; i++) pomo_tick(); // 11 decrements -> seconds_event
  pomo_update();                            // PREPARE -> RUN_WORK via Start()
}

TEST(init_sets_idle_state) {
  pomo_set_delay_scale(0);
  pomo_init();
  ASSERT_EQ(pomo_status(), STATUS_IDLE);
  ASSERT_EQ(pomo_minute(), DEFAULT_TIME);
  ASSERT_EQ(pomo_completed(), 0);
}

TEST(init_display_shows_hello) {
  pomo_set_delay_scale(0);
  pomo_init();
  ASSERT_TRUE(strcmp(pomo_display_text(), "HELL0") == 0);
  ASSERT_FALSE(pomo_display_dot());
  const int8_t* d = pomo_display_digits();
  ASSERT_EQ(d[0], 0x7f);
  ASSERT_EQ(d[1], 0x7f);
  ASSERT_EQ(d[2], 0x7f);
  ASSERT_EQ(d[3], 0x7f);
}

TEST(init_user_data_from_rtc) {
  pomo_set_delay_scale(0);
  pomo_init();
  ASSERT_EQ(pomo_user_data(0), 11);
  ASSERT_EQ(pomo_user_data(1), 0);
  ASSERT_EQ(pomo_user_data(2), 0);
  ASSERT_EQ(pomo_user_data(3), 0);
  ASSERT_EQ(pomo_user_data(4), 0);
}

TEST(read_click_none_when_released) {
  pomo_set_delay_scale(0);
  pomo_init();
  ASSERT_EQ(pomo_read_click(), CLICK_NONE);
}

TEST(increase_time_wraps_after_eight_clicks) {
  pomo_set_delay_scale(0);
  pomo_init();
  for (int i = 0; i < 7; i++) {
    pomo_handle_action(CLICK_SHORT_ACTION);
  }
  ASSERT_EQ(pomo_minute(), 60);
  pomo_handle_action(CLICK_SHORT_ACTION); // 8th click: 65 > MAX_TIME -> reset
  ASSERT_EQ(pomo_minute(), DEFAULT_TIME);
}

TEST(long_click_enters_prepare) {
  pomo_set_delay_scale(0);
  pomo_init();
  pomo_handle_action(CLICK_LONG_ACTION);
  ASSERT_EQ(pomo_status(), STATUS_PREPARE);
  ASSERT_EQ(pomo_second(), 10);
}

TEST(prepare_status_updates_prev_status) {
  pomo_set_delay_scale(0);
  pomo_init();
  pomo_handle_action(CLICK_LONG_ACTION);
  ASSERT_EQ(pomo_status(), STATUS_PREPARE);
  ASSERT_EQ(pomo_prev_status(), STATUS_IDLE);
}

TEST(tick_decrements_second) {
  pomo_set_delay_scale(0);
  pomo_init();
  ASSERT_EQ(pomo_second(), 59);
  pomo_tick();
  pomo_tick();
  ASSERT_EQ(pomo_second(), 58);
  ASSERT_FALSE(pomo_seconds_event());
}

TEST(seconds_event_after_full_minute) {
  pomo_set_delay_scale(0);
  pomo_init();
  for (int i = 0; i < 120; i++) pomo_tick(); // 60 decrements: 59 -> -1
  ASSERT_TRUE(pomo_seconds_event());
  ASSERT_EQ(pomo_second(), 59);
}

TEST(update_clears_seconds_event_in_idle) {
  pomo_set_delay_scale(0);
  pomo_init();
  for (int i = 0; i < 120; i++) pomo_tick();
  ASSERT_TRUE(pomo_seconds_event());
  pomo_update();
  ASSERT_FALSE(pomo_seconds_event());
  ASSERT_EQ(pomo_status(), STATUS_IDLE);
}

TEST(prepare_to_run_work_after_countdown) {
  pomo_set_delay_scale(0);
  pomo_init();
  enter_run_work();
  ASSERT_EQ(pomo_status(), STATUS_RUN_WORK);
  ASSERT_EQ(pomo_minute(), DEFAULT_TIME - 1);
  ASSERT_EQ(pomo_second(), 59);
}

TEST(pause_toggles_status) {
  pomo_set_delay_scale(0);
  pomo_init();
  enter_run_work();
  pomo_handle_action(CLICK_SHORT_ACTION); // RUN_WORK -> RUN_PAUSE
  ASSERT_EQ(pomo_status(), STATUS_RUN_PAUSE);
  pomo_handle_action(CLICK_SHORT_ACTION); // RUN_PAUSE -> RUN_WORK
  ASSERT_EQ(pomo_status(), STATUS_RUN_WORK);
}

TEST(cancel_from_run_work_returns_to_idle) {
  pomo_set_delay_scale(0);
  pomo_init();
  enter_run_work();
  pomo_handle_action(CLICK_LONG_ACTION); // RUN_WORK -> Cancel -> IDLE
  ASSERT_EQ(pomo_status(), STATUS_IDLE);
  ASSERT_EQ(pomo_minute(), DEFAULT_TIME);
}

TEST(display_updates_during_run_work) {
  pomo_set_delay_scale(0);
  pomo_init();
  enter_run_work();
  bool dot_before = pomo_display_dot();
  pomo_tick();
  pomo_update();
  const int8_t* d = pomo_display_digits();
  ASSERT_EQ(d[0], 2);
  ASSERT_EQ(d[1], 4);
  ASSERT_EQ(d[2], 5);
  ASSERT_EQ(d[3], 9);
  ASSERT_NE((int)pomo_display_dot(), (int)dot_before);
}

TEST(apply_rollover_same_month_next_day) {
  uint16_t data[5] = {11, 100, 200, 300, 400};
  pomo_apply_rollover(data, 2030, 3, 12, 2); // Tue after Mon, same week
  ASSERT_EQ(data[0], 12);
  ASSERT_EQ(data[1], 0);
  ASSERT_EQ(data[2], 300);
  ASSERT_EQ(data[3], 300);
  ASSERT_EQ(data[4], 400);
}

TEST(apply_rollover_crossing_week) {
  uint16_t data[5] = {16, 100, 200, 300, 400}; // Saturday
  pomo_apply_rollover(data, 2030, 3, 17, 0);   // Sunday
  ASSERT_EQ(data[0], 17);
  ASSERT_EQ(data[1], 0);
  ASSERT_EQ(data[2], 0);
  ASSERT_EQ(data[3], 600);
  ASSERT_EQ(data[4], 400);
}

TEST(apply_rollover_crossing_month) {
  uint16_t data[5] = {28, 50, 60, 70, 80};
  pomo_apply_rollover(data, 2030, 4, 1, 0);
  ASSERT_EQ(data[0], 1);
  ASSERT_EQ(data[1], 0);
  ASSERT_EQ(data[2], 0);
  ASSERT_EQ(data[3], 0);
  ASSERT_EQ(data[4], 260);
}

TEST(calc_weekday_known_dates) {
  ASSERT_EQ(pomo_calc_weekday(2030, 3, 11), 1); // Monday
  ASSERT_EQ(pomo_calc_weekday(2030, 3, 17), 0); // Sunday
  ASSERT_EQ(pomo_calc_weekday(2030, 3, 12), 2); // Tuesday
  ASSERT_EQ(pomo_calc_weekday(2030, 4, 1), 1);  // Monday
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
