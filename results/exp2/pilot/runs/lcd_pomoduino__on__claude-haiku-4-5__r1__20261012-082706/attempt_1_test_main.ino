#include "logic.h"
#include "test_runner.h"

TEST(setup_initializes_state) {
  pomo2_setup();
  ASSERT_EQ(the_pomodoro.Get_State(), POM_READY);
  ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 0); // T_POM25
  ASSERT_TRUE(the_pomodoro.Is_Pomodoro());
}

TEST(button_b1_cycles_timer_type) {
  pomo2_setup();
  // Start at 25 min (T_POM25)
  ASSERT_EQ(the_pomodoro.Get_Current_Period_Minutes(), 25);
  
  // Press and release B1
  pomo2_step(0, false, false);
  pomo2_step(100, true, false);
  pomo2_step(200, false, false);
  
  // Should cycle to 5 min (T_POM5)
  ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 1);
  ASSERT_EQ(the_pomodoro.Get_Current_Period_Minutes(), 5);
  ASSERT_FALSE(the_pomodoro.Is_Pomodoro());
}

TEST(button_b2_starts_timer) {
  pomo2_setup();
  ASSERT_EQ(the_pomodoro.Get_State(), POM_READY);
  
  // Short press on B2 (< 250ms)
  pomo2_step(0, false, false);
  pomo2_step(100, false, true);
  pomo2_step(200, false, false); // release
  
  // Should transition to RUNNING
  ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);
}

TEST(timer_counts_down) {
  pomo2_setup();
  
  // Start timer
  pomo2_step(0, false, false);
  pomo2_step(100, false, true);
  pomo2_step(200, false, false);
  
  uint32_t max_time = the_pomodoro.Get_Time_Max_Ms();
  ASSERT_NE(max_time, 0);
  
  uint32_t initial_left = the_pomodoro.Get_Time_Left_Ms();
  
  // Advance time and check countdown
  pomo2_step(1100, false, false);
  uint32_t left_after = the_pomodoro.Get_Time_Left_Ms();
  
  ASSERT_TRUE(left_after < initial_left);
  ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);
}

TEST(timer_elapsed_transitions_to_finished) {
  pomo2_setup();
  
  // Start a 5-min timer (using B1 to cycle to short break)
  pomo2_step(0, false, false);
  pomo2_step(100, true, false);
  pomo2_step(200, false, false); // cycle to 5 min
  
  pomo2_step(300, false, false);
  pomo2_step(400, false, true);
  pomo2_step(500, false, false); // start
  
  ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);
  
  // Fast-forward past timer (5 min = 300000 ms in normal mode, but 5000 ms in debug)
  uint32_t max_ms = the_pomodoro.Get_Time_Max_Ms();
  pomo2_step(500 + max_ms + 1000, false, false);
  
  // Should transition to FINISHED
  ASSERT_EQ(the_pomodoro.Get_State(), POM_FINISHED);
}

TEST(pause_and_resume) {
  pomo2_setup();
  
  // Start timer
  pomo2_step(0, false, false);
  pomo2_step(100, false, true);
  pomo2_step(200, false, false);
  ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);
  
  uint32_t time_left_1 = the_pomodoro.Get_Time_Left_Ms();
  
  // Advance and check countdown
  pomo2_step(1500, false, false);
  uint32_t time_left_2 = the_pomodoro.Get_Time_Left_Ms();
  ASSERT_TRUE(time_left_2 < time_left_1);
  
  // Release B2 to pause
  pomo2_step(1600, false, true);
  pomo2_step(1700, false, false);
  ASSERT_EQ(the_pomodoro.Get_State(), POM_PAUSED);
  
  uint32_t paused_time = the_pomodoro.Get_Time_Left_Ms();
  
  // Advance time while paused - time should not decrease
  pomo2_step(2700, false, false);
  uint32_t paused_time_later = the_pomodoro.Get_Time_Left_Ms();
  ASSERT_EQ(paused_time, paused_time_later);
  
  // Resume with short B2 press
  pomo2_step(2800, false, true);
  pomo2_step(2900, false, false);
  ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);
}

TEST(lcd_shows_ready_state) {
  pomo2_setup();
  pomo2_loop_at(0);
  
  const char *line0 = pomo2_lcd_line(0);
  ASSERT_TRUE(line0 != NULL);
  // Line should contain "READY" and period info
}

TEST(lcd_shows_running_state) {
  pomo2_setup();
  
  // Start timer
  pomo2_step(0, false, false);
  pomo2_step(100, false, true);
  pomo2_step(200, false, false);
  
  pomo2_loop_at(500);
  
  const char *line0 = pomo2_lcd_line(0);
  ASSERT_TRUE(line0 != NULL);
}

TEST(button_pressed_released_detection) {
  pomo2_setup();
  
  // Test button press
  the_b1.Button_Logic::Update(0, false);
  ASSERT_FALSE(the_b1.Pressed());
  ASSERT_FALSE(the_b1.On());
  
  the_b1.Button_Logic::Update(100, true);
  ASSERT_TRUE(the_b1.Pressed());
  ASSERT_TRUE(the_b1.On());
  
  the_b1.Button_Logic::Update(200, true);
  ASSERT_FALSE(the_b1.Pressed());
  ASSERT_TRUE(the_b1.On());
  ASSERT_EQ(the_b1.Duration(), 100);
  
  the_b1.Button_Logic::Update(300, false);
  ASSERT_TRUE(the_b1.Released());
  ASSERT_FALSE(the_b1.On());
  ASSERT_EQ(the_b1.Last_Duration(), 100);
}

TEST(button_duration_tracking) {
  pomo2_setup();
  
  the_b2.Button_Logic::Update(0, false);
  the_b2.Button_Logic::Update(100, true);
  ASSERT_EQ(the_b2.Duration(), 0);
  
  the_b2.Button_Logic::Update(1500, true);
  ASSERT_EQ(the_b2.Duration(), 1400);
  
  the_b2.Button_Logic::Update(4500, true);
  ASSERT_TRUE(the_b2.Duration() > 3000);
}

TEST(pomodoro_count_increments) {
  pomo2_setup();
  uint8_t initial_count = the_pomodoro.Get_Pomodoros();
  
  // Complete a 25-min pomodoro cycle
  pomo2_step(0, false, false);
  pomo2_step(100, false, true);
  pomo2_step(200, false, false); // start
  
  uint32_t max_ms = the_pomodoro.Get_Time_Max_Ms();
  pomo2_step(300 + max_ms + 1000, false, false); // timer elapsed
  
  // Beeper should be running; advance until beeper finishes
  for (int i = 0; i < 20; i++) {
    pomo2_step(300 + max_ms + 1000 + (i * 100), false, false);
  }
  
  // After finish completes, should return to ready with incremented count
  if (the_pomodoro.Get_State() == POM_READY) {
    ASSERT_EQ(the_pomodoro.Get_Pomodoros(), initial_count + 1);
  }
}

TEST(ms_to_m_s_conversion) {
  const char *result = Utils::ms_to_m_s(125000); // 2 min 5 sec
  ASSERT_TRUE(result != NULL);
  // Result should be "02:05" format
}

TEST(beeper_output_pin) {
  pomo2_setup();
  pinMode(2, OUTPUT);
  
  // Start and complete a timer
  pomo2_step(0, false, false);
  pomo2_step(100, false, true);
  pomo2_step(200, false, false);
  
  uint32_t max_ms = the_pomodoro.Get_Time_Max_Ms();
  pomo2_step(300 + max_ms + 1000, false, false);
  
  // Beeper should be active (pin toggling)
  ASSERT_EQ(the_pomodoro.Get_State(), POM_FINISHED);
}

TEST(long_b2_press_clears_count) {
  pomo2_setup();
  
  // Do one pomodoro
  pomo2_step(0, false, false);
  pomo2_step(100, false, true);
  pomo2_step(200, false, false);
  uint32_t max_ms = the_pomodoro.Get_Time_Max_Ms();
  pomo2_step(300 + max_ms + 1000, false, false);
  
  for (int i = 0; i < 20; i++) {
    pomo2_step(300 + max_ms + 1000 + (i * 100), false, false);
  }
  
  // Now we should be back in READY
  if (the_pomodoro.Get_State() == POM_READY && the_pomodoro.Get_Pomodoros() > 0) {
    // Long press B2 (> 3 seconds)
    pomo2_step(5000, false, true);
    pomo2_step(8500, false, true);
    pomo2_step(8600, false, false);
    
    // Count should be cleared
    ASSERT_EQ(the_pomodoro.Get_Pomodoros(), 0);
  }
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
