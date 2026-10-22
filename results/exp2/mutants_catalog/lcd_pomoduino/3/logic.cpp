#include "logic.h"
#include <new.h>

// Global objects of the original sketch.ino
MO::Button the_b1(MO::Const::PIN_B1);
MO::Button the_b2(MO::Const::PIN_B2);
MO::Eeprom the_eeprom;
MO::UI     the_ui;

// for MO::Pomodoro it does not matter what UI it uses
MO::Pomodoro the_pomodoro(&the_ui, &the_b1, &the_b2, &the_eeprom);

void pomo2_setup()
{
    // power-up state (testability: pomo2_setup() can be called again)
    the_b1.Reset();
    the_b2.Reset();
    // re-construct in place: Pomodoro keeps pointers to its own Period members, so it must not be copied
    the_pomodoro.~Pomodoro();
    new (&the_pomodoro) MO::Pomodoro(&the_ui, &the_b1, &the_b2, &the_eeprom);

    the_b1.Setup();
    the_b2.Setup();
    the_pomodoro.Setup();
}

void pomo2_loop_at(uint32_t a_time)
{
    the_b1.Update(a_time);
    the_b2.Update(a_time);
    the_pomodoro.Run(a_time);
}

void pomo2_step(uint32_t a_time, bool b1_down, bool b2_down)
{
    ;
    the_b2.Button_Logic::Update(a_time, b2_down);
    the_pomodoro.Run(a_time);
}

const char *pomo2_lcd_line(uint8_t a_y)
{
    return the_ui.Line(a_y);
}
