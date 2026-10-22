#include "logic.h"
#include "test_runner.h"

static void reset_pomodoro_count()
{
  pomo2_setup();
  pomo2_step(100, false, false);
  pomo2_step(200, false, true);
  pomo2_step(3201, false, true);
  pomo2_step(3300, false, false);
}

TEST(button_logic_tracks_edges_and_durations)
{
  MO::Button_Logic button;
  button.Update(100, false);
  ASSERT_FALSE(button.On());
  ASSERT_FALSE(button.Pressed());
  ASSERT_FALSE(button.Released());
  ASSERT_EQ(button.Duration(), 0);

  button.Update(150, true);
  ASSERT_TRUE(button.On());
  ASSERT_TRUE(button.Pressed());
  ASSERT_FALSE(button.Released());
  ASSERT_EQ(button.Duration(), 0);
  ASSERT_EQ(button.Last_Duration(), 50);

  button.Update(200, true);
  ASSERT_TRUE(button.On());
  ASSERT_FALSE(button.Pressed());
  ASSERT_EQ(button.Duration(), 50);

  button.Update(230, false);
  ASSERT_FALSE(button.On());
  ASSERT_TRUE(button.Released());
  ASSERT_FALSE(button.Pressed());
  ASSERT_EQ(button.Last_Duration(), 80);
  ASSERT_EQ(button.Duration(), 0);

  button.Update(260, false);
  ASSERT_FALSE(button.Released());
  ASSERT_EQ(button.Duration(), 30);

  button.Reset();
  ASSERT_FALSE(button.On());
  ASSERT_EQ(button.Duration(), 0);
  ASSERT_EQ(button.Last_Duration(), 0);
}

TEST(timer_and_period_values)
{
  MO::Timer timer(1000);
  timer.Update(100);
  ASSERT_EQ(timer.Duration(), 0);
  ASSERT_EQ(timer.Get_Time_Left(), 1000);
  ASSERT_FALSE(timer.Elapsed());

  timer.Update(350);
  ASSERT_EQ(timer.Duration(), 250);
  ASSERT_EQ(timer.Get_Time_Left(), 750);

  timer.Update(1100);
  ASSERT_TRUE(timer.Elapsed());
  ASSERT_EQ(timer.Get_Time_Left(), 0);

  timer.Reset(500);
  ASSERT_EQ(timer.Duration(), 0);
  ASSERT_EQ(timer.Get_Max(), 500);
  ASSERT_FALSE(timer.Elapsed());

  MO::Period period(25, 5);
  ASSERT_EQ(period.Get_Time_In_Min(), 25);
  ASSERT_EQ(period.Get_Time_In_Ms(), 1500000UL);
  ASSERT_EQ(period.Get_Beeps(), 5);
}

TEST(beeper_generates_requested_cycles)
{
  MO::Beeper beeper(2, 1);
  beeper.Setup();
  beeper.Reset(2);

  ASSERT_FALSE(beeper.Done());
  ASSERT_PIN(2, LOW);

  beeper.Update();
  ASSERT_PIN(2, HIGH);
  ASSERT_FALSE(beeper.Done());

  beeper.Update();
  ASSERT_PIN(2, LOW);
  ASSERT_FALSE(beeper.Done());

  beeper.Update();
  ASSERT_PIN(2, HIGH);
  ASSERT_FALSE(beeper.Done());

  beeper.Update();
  ASSERT_PIN(2, LOW);
  ASSERT_TRUE(beeper.Done());

  beeper.Update();
  ASSERT_PIN(2, LOW);
}

TEST(setup_initializes_ready_work_period)
{
  pomo2_setup();

  ASSERT_PIN(3, HIGH);
  ASSERT_PIN(4, HIGH);
  ASSERT_EQ(the_pomodoro.Get_State(), POM_READY);
  ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 0);
  ASSERT_TRUE(the_pomodoro.Is_Pomodoro());
  ASSERT_EQ(the_pomodoro.Get_Current_Period_Minutes(), 25);
  ASSERT_EQ(the_pomodoro.Get_Time_Max_Ms(), 1500000UL);
  ASSERT_EQ(the_pomodoro.Get_Time_Left_Ms(), 1500000UL);
}

TEST(ready_b1_cycles_all_periods)
{
  reset_pomodoro_count();

  pomo2_step(3400, false, false);
  pomo2_step(3500, true, false);
  pomo2_step(3600, false, false);
  ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 1);
  ASSERT_FALSE(the_pomodoro.Is_Pomodoro());
  ASSERT_EQ(the_pomodoro.Get_Current_Period_Minutes(), 5);
  ASSERT_EQ(the_pomodoro.Get_Time_Max_Ms(), 300000UL);

  pomo2_step(3700, true, false);
  pomo2_step(3800, false, false);
  ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 2);
  ASSERT_EQ(the_pomodoro.Get_Current_Period_Minutes(), 15);
  ASSERT_EQ(the_pomodoro.Get_Time_Max_Ms(), 900000UL);

  pomo2_step(3900, true, false);
  pomo2_step(4000, false, false);
  ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 0);
  ASSERT_TRUE(the_pomodoro.Is_Pomodoro());
  ASSERT_EQ(the_pomodoro.Get_Current_Period_Minutes(), 25);
  ASSERT_EQ(the_pomodoro.Get_Time_Max_Ms(), 1500000UL);
}

TEST(short_b2_click_starts_and_pause_resume_preserves_time)
{
  reset_pomodoro_count();

  pomo2_step(3400, false, true);
  pomo2_step(3500, false, false);
  ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);
  ASSERT_PIN(2, LOW);

  pomo2_step(3600, false, false);
  pomo2_step(4600, false, false);
  ASSERT_EQ(the_pomodoro.Get_Time_Left_Ms(), 1499000UL);

  pomo2_step(4700, false, true);
  pomo2_step(4800, false, false);
  ASSERT_EQ(the_pomodoro.Get_State(), POM_PAUSED);
  uint32_t paused_left = the_pomodoro.Get_Time_Left_Ms();

  pomo2_step(4900, false, false);
  ASSERT_EQ(the_pomodoro.Get_Time_Left_Ms(), paused_left);

  pomo2_step(5000, false, true);
  pomo2_step(5100, false, false);
  ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);

  pomo2_step(5200, false, false);
  pomo2_step(6200, false, false);
  ASSERT_EQ(the_pomodoro.Get_Time_Left_Ms(), paused_left - 1000UL);
}

TEST(long_ready_b2_hold_clears_count_and_work_completion_advances)
{
  reset_pomodoro_count();
  ASSERT_EQ(the_pomodoro.Get_Pomodoros(), 0);
  ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 0);

  pomo2_step(3400, false, true);
  pomo2_step(3500, false, false);
  ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);

  pomo2_step(3600, false, false);
  pomo2_step(1503600UL, false, false);
  ASSERT_EQ(the_pomodoro.Get_State(), POM_FINISHED);

  pomo2_step(1503601UL, false, false);
  pomo2_step(1503602UL, false, false);
  pomo2_step(1503603UL, false, false);
  pomo2_step(1503604UL, false, false);
  pomo2_step(1503605UL, false, false);
  pomo2_step(1503606UL, false, false);
  pomo2_step(1503607UL, false, false);
  pomo2_step(1503608UL, false, false);
  pomo2_step(1503609UL, false, false);
  pomo2_step(1503610UL, false, false);
  pomo2_step(1503611UL, false, false);

  ASSERT_EQ(the_pomodoro.Get_State(), POM_READY);
  ASSERT_EQ(the_pomodoro.Get_Pomodoros(), 1);
  ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 1);
  ASSERT_EQ(the_pomodoro.Get_Current_Period_Minutes(), 5);
  ASSERT_EQ(the_pomodoro.Get_Time_Max_Ms(), 300000UL);
  ASSERT_PIN(2, LOW);
}

TEST(lcd_reports_ready_screen)
{
  reset_pomodoro_count();
  pomo2_step(3400, false, false);

  const char *line0 = pomo2_lcd_line(0);
  const char *line1 = pomo2_lcd_line(1);

  ASSERT_EQ(line0[0], 'P');
  ASSERT_EQ(line0[1], '2');
  ASSERT_EQ(line0[2], '5');
  ASSERT_EQ(line0[4], 'R');
  ASSERT_EQ(line0[5], 'E');
  ASSERT_EQ(line0[6], 'A');
  ASSERT_EQ(line0[7], 'D');
  ASSERT_EQ(line0[8], 'Y');
  ASSERT_EQ((uint8_t)line0[13], 1);
  ASSERT_EQ(line0[14], '0');
  ASSERT_EQ(line0[15], '0');
  ASSERT_EQ(line0[16], 0);
  ASSERT_EQ(line1[0], ' ');
  ASSERT_EQ(line1[15], ' ');
  ASSERT_EQ(line1[16], 0);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
