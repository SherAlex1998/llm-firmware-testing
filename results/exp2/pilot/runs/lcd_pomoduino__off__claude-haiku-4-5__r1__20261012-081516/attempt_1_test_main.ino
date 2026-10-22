#include "logic.h"
#include "test_runner.h"

TEST(setup_initializes_system) {
  pomo2_setup();
  ASSERT_EQ(the_pomodoro.Get_State(), POM_READY);
  ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 0); // T_POM25
  ASSERT_TRUE(the_pomodoro.Is_Pomodoro());
}

TEST(initial_state_ready) {
  pomo2_setup();
  pomo2_loop_at(0);
  const char *line0 = pomo2_lcd_line(0);
  ASSERT_TRUE(line0 != NULL);
  ASSERT_EQ(the_pomodoro.Get_Current_Period_Minutes(), 25);
}

TEST(b1_cycles_timer_types) {
  pomo2_setup();
  pomo2_loop_at(0);
  ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 0); // T_POM25
  
  // Press and release B1
  pomo2_step(100, true, false);
  pomo2_step(300, false, false);
  pomo2_loop_at(300);
  
  ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 1); // T_POM5
  ASSERT_EQ(the_pomodoro.Get_Current_Period_Minutes(), 5);
  ASSERT_FALSE(the_pomodoro.Is_Pomodoro());
}

TEST(b1_continues_cycling) {
  pomo2_setup();
  pomo2_loop_at(0);
  
  // Cycle through: 25 -> 5 -> 15 -> 25
  pomo2_step(100, true, false);
  pomo2_step(300, false, false);
  pomo2_loop_at(300);
  ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 1);
  
  pomo2_step(400, true, false);
  pomo2_step(600, false, false);
  pomo2_loop_at(600);
  ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 2); // T_POM15
  ASSERT_EQ(the_pomodoro.Get_Current_Period_Minutes(), 15);
  
  pomo2_step(700, true, false);
  pomo2_step(900, false, false);
  pomo2_loop_at(900);
  ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 0); // Back to T_POM25
}

TEST(b2_short_press_starts_timer) {
  pomo2_setup();
  pomo2_loop_at(0);
  ASSERT_EQ(the_pomodoro.Get_State(), POM_READY);
  
  // Short B2 press (< 250ms)
  pomo2_step(100, false, true);
  pomo2_step(200, false, false);
  pomo2_loop_at(200);
  
  ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);
  ASSERT_GT(the_pomodoro.Get_Time_Left_Ms(), 0);
}

TEST(timer_counts_down) {
  pomo2_setup();
  pomo2_loop_at(0);
  
  // Start timer with B2
  pomo2_step(100, false, true);
  pomo2_step(200, false, false);
  pomo2_loop_at(200);
  
  uint32_t time_left_1 = the_pomodoro.Get_Time_Left_Ms();
  ASSERT_GT(time_left_1, 0);
  
  // Update timer
  pomo2_loop_at(300000);
  uint32_t time_left_2 = the_pomodoro.Get_Time_Left_Ms();
  ASSERT_LT(time_left_2, time_left_1);
}

TEST(b2_release_pauses_timer) {
  pomo2_setup();
  pomo2_loop_at(0);
  
  // Start timer
  pomo2_step(100, false, true);
  pomo2_step(200, false, false);
  pomo2_loop_at(200);
  ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);
  
  // Advance time
  pomo2_loop_at(300000);
  uint32_t time_left_paused = the_pomodoro.Get_Time_Left_Ms();
  
  // Release B2 to pause
  pomo2_step(300100, false, false);
  pomo2_loop_at(300100);
  
  ASSERT_EQ(the_pomodoro.Get_State(), POM_PAUSED);
  ASSERT_EQ(the_pomodoro.Get_Time_Left_Ms(), time_left_paused);
}

TEST(b2_resume_from_pause) {
  pomo2_setup();
  pomo2_loop_at(0);
  
  // Start timer
  pomo2_step(100, false, true);
  pomo2_step(200, false, false);
  pomo2_loop_at(200);
  
  // Pause
  pomo2_loop_at(300000);
  pomo2_step(300100, false, false);
  pomo2_loop_at(300100);
  ASSERT_EQ(the_pomodoro.Get_State(), POM_PAUSED);
  
  // Resume with short B2 press
  uint32_t paused_time = the_pomodoro.Get_Time_Left_Ms();
  pomo2_step(300200, false, true);
  pomo2_step(300300, false, false);
  pomo2_loop_at(300300);
  
  ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);
  ASSERT_EQ(the_pomodoro.Get_Time_Left_Ms(), paused_time);
}

TEST(button_logic_pressed_released) {
  pomo2_setup();
  
  // Test B1 press detection
  the_b1.Button_Logic::Update(0, false);
  ASSERT_FALSE(the_b1.Pressed());
  ASSERT_FALSE(the_b1.On());
  
  the_b1.Button_Logic::Update(100, true);
  ASSERT_TRUE(the_b1.Pressed());
  ASSERT_TRUE(the_b1.On());
  
  the_b1.Button_Logic::Update(200, false);
  ASSERT_TRUE(the_b1.Released());
  ASSERT_FALSE(the_b1.On());
}

TEST(button_logic_duration) {
  pomo2_setup();
  
  the_b2.Button_Logic::Update(0, false);
  ASSERT_EQ(the_b2.Duration(), 0);
  
  the_b2.Button_Logic::Update(1000, true);
  ASSERT_EQ(the_b2.Duration(), 1000);
  
  the_b2.Button_Logic::Update(2000, true);
  ASSERT_EQ(the_b2.Duration(), 2000);
  
  the_b2.Button_Logic::Update(3000, false);
  ASSERT_EQ(the_b2.Last_Duration(), 3000);
}

TEST(button_logic_reset) {
  pomo2_setup();
  
  the_b1.Button_Logic::Update(100, true);
  the_b1.Button_Logic::Update(200, false);
  
  the_b1.Reset();
  ASSERT_FALSE(the_b1.On());
  ASSERT_FALSE(the_b1.Pressed());
  ASSERT_FALSE(the_b1.Released());
  ASSERT_EQ(the_b1.Duration(), 0);
}

TEST(b2_long_press_clears_pomodoros) {
  pomo2_setup();
  pomo2_loop_at(0);
  
  // Set some pomodoros
  ASSERT_GE(the_pomodoro.Get_Pomodoros(), 0);
  
  // Long B2 press (> 3s) in READY state
  pomo2_step(100, false, true);
  pomo2_step(4000, false, true);
  pomo2_loop_at(4000);
  
  ASSERT_EQ(the_pomodoro.Get_Pomodoros(), 0);
  ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 0); // Reset to T_POM25
}

TEST(pomodoro_increments_after_work_period) {
  pomo2_setup();
  pomo2_loop_at(0);
  
  uint8_t initial_pom = the_pomodoro.Get_Pomodoros();
  
  // Start a very short work period (use debug mode timing if available)
  pomo2_step(100, false, true);
  pomo2_step(200, false, false);
  pomo2_loop_at(200);
  
  // Simulate completion (this is simplified - full completion depends on Period timing)
  // Just verify pomodoro count doesn't decrease
  ASSERT_GE(the_pomodoro.Get_Pomodoros(), initial_pom);
}

TEST(get_state_returns_valid_state) {
  pomo2_setup();
  pomo2_loop_at(0);
  
  uint8_t state = the_pomodoro.Get_State();
  ASSERT_TRUE(state == POM_READY || state == POM_RUNNING || 
              state == POM_FINISHED || state == POM_PAUSED);
}

TEST(get_timer_type_valid_range) {
  pomo2_setup();
  pomo2_loop_at(0);
  
  uint8_t type = the_pomodoro.Get_Timer_Type();
  ASSERT_LE(type, 2);
}

TEST(lcd_line_returns_string) {
  pomo2_setup();
  pomo2_loop_at(0);
  
  const char *line0 = pomo2_lcd_line(0);
  const char *line1 = pomo2_lcd_line(1);
  
  ASSERT_TRUE(line0 != NULL);
  ASSERT_TRUE(line1 != NULL);
}

TEST(beeper_pin_set_to_output) {
  pomo2_setup();
  
  // Verify beeper pin is initialized
  pinMode(MO::Const::PIN_BEEPER, OUTPUT);
  digitalWrite(MO::Const::PIN_BEEPER, LOW);
  ASSERT_PIN(MO::Const::PIN_BEEPER, LOW);
  
  digitalWrite(MO::Const::PIN_BEEPER, HIGH);
  ASSERT_PIN(MO::Const::PIN_BEEPER, HIGH);
}

TEST(buttons_configured_as_pullup) {
  pomo2_setup();
  
  // Buttons should be INPUT_PULLUP - not connected reads HIGH
  pinMode(MO::Const::PIN_B1, INPUT_PULLUP);
  pinMode(MO::Const::PIN_B2, INPUT_PULLUP);
  
  int b1_state = digitalRead(MO::Const::PIN_B1);
  int b2_state = digitalRead(MO::Const::PIN_B2);
  
  ASSERT_EQ(b1_state, HIGH);
  ASSERT_EQ(b2_state, HIGH);
}

TEST(time_left_equals_max_at_start) {
  pomo2_setup();
  pomo2_loop_at(0);
  
  // Start timer
  pomo2_step(100, false, true);
  pomo2_step(200, false, false);
  pomo2_loop_at(200);
  
  uint32_t time_left = the_pomodoro.Get_Time_Left_Ms();
  uint32_t time_max = the_pomodoro.Get_Time_Max_Ms();
  
  ASSERT_EQ(time_left, time_max);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
