#include "logic.h"
#include "test_runner.h"

// ---------- helpers ----------

// Force stored pomodoro count to 0 and get a clean, freshly constructed
// Pomodoro state machine (state READY, timer_type T_POM25).
static void reset_to_clean_state()
{
    pomo2_setup();
    // long-press B2 (>3000ms) while READY clears the pomodoro counter
    pomo2_step(10, false, true);     // baseline press (first update after Reset)
    pomo2_step(4010, false, true);   // still held, duration > 3000 -> clears count
    pomo2_step(4110, false, false);  // release (last duration large, no start)
    pomo2_setup();                   // re-construct; eeprom now reads 0
}

// ---------- Button_Logic direct tests ----------

TEST(button_logic_first_update_is_baseline)
{
    MO::Button_Logic bl;
    bl.Update(1000, true);
    ASSERT_FALSE(bl.Pressed());
    ASSERT_FALSE(bl.Released());
    ASSERT_TRUE(bl.On());
    ASSERT_EQ(bl.Duration(), 0);
}

TEST(button_logic_press_release_durations)
{
    MO::Button_Logic bl;
    bl.Update(1000, true);           // baseline: On
    bl.Update(1500, true);           // still on, duration 500
    ASSERT_EQ(bl.Duration(), 500);
    ASSERT_FALSE(bl.Pressed());
    ASSERT_FALSE(bl.Released());

    bl.Update(1600, false);          // release
    ASSERT_TRUE(bl.Released());
    ASSERT_FALSE(bl.On());
    ASSERT_EQ(bl.Last_Duration(), 600);
    ASSERT_EQ(bl.Duration(), 0);

    bl.Update(1700, true);           // press again
    ASSERT_TRUE(bl.Pressed());
    ASSERT_TRUE(bl.On());
}

TEST(button_logic_reset_clears_state)
{
    MO::Button_Logic bl;
    bl.Update(100, true);
    bl.Update(300, false);
    bl.Reset();
    ASSERT_FALSE(bl.On());
    ASSERT_EQ(bl.Duration(), 0);
    ASSERT_EQ(bl.Last_Duration(), 0);
}

// ---------- Pomodoro / UI tests ----------

TEST(initial_state_after_setup)
{
    reset_to_clean_state();

    ASSERT_EQ(the_pomodoro.Get_State(), POM_READY);
    ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 0);
    ASSERT_EQ(the_pomodoro.Get_Current_Period_Minutes(), 25);
    ASSERT_TRUE(the_pomodoro.Is_Pomodoro());
    ASSERT_EQ(the_pomodoro.Get_Pomodoros(), 0);

    // beeper pin should be idle (LOW) right after setup
    ASSERT_PIN(2, LOW);
}

TEST(ready_state_shows_lcd_with_pom_count)
{
    reset_to_clean_state();

    pomo2_step(10, false, false); // run once -> Show_Ready writes line 0
    const char *line0 = pomo2_lcd_line(0);
    ASSERT_EQ(line0[0], 'P');
    ASSERT_EQ(line0[1], '2');
    ASSERT_EQ(line0[2], '5');
}

TEST(b1_cycles_through_period_types)
{
    reset_to_clean_state();

    pomo2_step(10, false, false);  // baseline (no press)
    pomo2_step(20, true, false);   // press b1
    pomo2_step(30, false, false);  // release -> cycles to T_POM5 (1)
    ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 1);
    ASSERT_EQ(the_pomodoro.Get_Current_Period_Minutes(), 5);

    pomo2_step(40, true, false);
    pomo2_step(50, false, false);  // release -> cycles to T_POM15 (2)
    ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 2);
    ASSERT_EQ(the_pomodoro.Get_Current_Period_Minutes(), 15);

    pomo2_step(60, true, false);
    pomo2_step(70, false, false);  // release -> wraps to T_POM25 (0)
    ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 0);
    ASSERT_EQ(the_pomodoro.Get_Current_Period_Minutes(), 25);
}

TEST(b2_short_click_starts_running)
{
    reset_to_clean_state();

    pomo2_step(10, false, false);  // baseline
    pomo2_step(20, false, true);   // press b2
    pomo2_step(120, false, false); // release, short duration (<250) -> RUNNING
    ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);
}

TEST(running_release_pauses)
{
    reset_to_clean_state();

    pomo2_step(10, false, false);
    pomo2_step(20, false, true);
    pomo2_step(120, false, false); // start running
    ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);

    // one update so the running timer actually starts counting
    pomo2_step(200, false, false);
    ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);

    // press and release b2 again -> any release while RUNNING pauses
    pomo2_step(300, false, true);
    pomo2_step(400, false, false);
    ASSERT_EQ(the_pomodoro.Get_State(), POM_PAUSED);
}

TEST(paused_short_click_resumes_running)
{
    reset_to_clean_state();

    pomo2_step(10, false, false);
    pomo2_step(20, false, true);
    pomo2_step(120, false, false); // RUNNING
    pomo2_step(200, false, false);
    pomo2_step(300, false, true);
    pomo2_step(400, false, false); // PAUSED
    ASSERT_EQ(the_pomodoro.Get_State(), POM_PAUSED);

    pomo2_step(500, false, true);
    pomo2_step(600, false, false); // short release -> resumes RUNNING
    ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);
}

TEST(paused_long_press_cancels_to_ready)
{
    reset_to_clean_state();

    pomo2_step(10, false, false);
    pomo2_step(20, false, true);
    pomo2_step(120, false, false); // RUNNING
    pomo2_step(200, false, false);
    pomo2_step(300, false, true);
    pomo2_step(400, false, false); // PAUSED
    ASSERT_EQ(the_pomodoro.Get_State(), POM_PAUSED);

    // hold b2 for > 3000ms while paused -> cancel back to READY
    pomo2_step(500, false, true);
    pomo2_step(4000, false, true);
    ASSERT_EQ(the_pomodoro.Get_State(), POM_READY);
    ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 0);
}

TEST(timer_elapses_enters_finished)
{
    reset_to_clean_state();

    pomo2_step(10, false, false);
    pomo2_step(20, false, true);
    pomo2_step(120, false, false); // RUNNING (short break would be quicker, use default 25min type)
    ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);

    pomo2_step(200, false, false); // starts the running timer counting (start_time=200)
    ASSERT_EQ(the_pomodoro.Get_State(), POM_RUNNING);

    uint32_t max_ms = the_pomodoro.Get_Time_Max_Ms();
    ASSERT_EQ(max_ms, (uint32_t)25 * 60 * 1000);

    // jump far enough ahead that the timer has elapsed
    pomo2_step(200 + max_ms, false, false);
    ASSERT_EQ(the_pomodoro.Get_State(), POM_FINISHED);
}

TEST(full_completion_increments_pomodoro_and_persists)
{
    reset_to_clean_state();

    pomo2_step(10, false, false);
    pomo2_step(20, false, true);
    pomo2_step(120, false, false); // RUNNING
    pomo2_step(200, false, false);

    uint32_t max_ms = the_pomodoro.Get_Time_Max_Ms();
    pomo2_step(200 + max_ms, false, false);
    ASSERT_EQ(the_pomodoro.Get_State(), POM_FINISHED);

    bool saw_high = false;
    uint32_t t = 200 + max_ms;
    for (int i = 0; i < 20 && the_pomodoro.Get_State() == POM_FINISHED; i++)
    {
        t += 50;
        pomo2_step(t, false, false);
        if (digitalRead(2) == HIGH) saw_high = true;
    }

    ASSERT_TRUE(saw_high);
    ASSERT_EQ(the_pomodoro.Get_State(), POM_READY);
    ASSERT_EQ(the_pomodoro.Get_Pomodoros(), 1);
    ASSERT_EQ(the_pomodoro.Get_Timer_Type(), 1); // T_POM5 after first pomodoro

    // persisted across re-construction (reads from EEPROM)
    pomo2_setup();
    ASSERT_EQ(the_pomodoro.Get_Pomodoros(), 1);

    // cleanup so other tests see a clean counter
    reset_to_clean_state();
}

// ---------- Eeprom direct test ----------

TEST(eeprom_read_write_roundtrip)
{
    the_eeprom.Write(10, 42);
    ASSERT_EQ(the_eeprom.Read(10), 42);
    the_eeprom.Write(10, 7);
    ASSERT_EQ(the_eeprom.Read(10), 7);
}

void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
void loop() {}
