#include "logic.h"

#include <TimerOne.h>
#include <Wire.h>
#include <RtcDS1307.h>

#include "state_machine.h"

uint8_t g_delay_scale = 100;

static RtcDS1307<TwoWire> Rtc(Wire);
static StateMachine machine(&Rtc);

static void TimingISR() {
  machine.Tick();
}

void pomo_set_delay_scale(uint8_t percent) {
  g_delay_scale = percent;
}

void pomo_init() {
  pinMode(BTN_BUZZER, OUTPUT);
  pinMode(BTN_CLICK, INPUT);
  machine.Reset();
  machine.Init();
}

void pomo_start_timer() {
  Timer1.initialize(500000);
  Timer1.attachInterrupt(TimingISR);
}

void pomo_tick() {
  machine.Tick();
}

void pomo_update() {
  machine.Update();
}

void pomo_handle_action(uint8_t action) {
  machine.HandleAction(action);
}

uint8_t pomo_read_click() {
  return machine.ReadClick();
}

uint8_t pomo_status() {
  return machine.Status();
}

uint8_t pomo_prev_status() {
  return machine.PrevStatus();
}

int8_t pomo_minute() {
  return machine.Minute();
}

int8_t pomo_second() {
  return machine.Second();
}

int8_t pomo_completed() {
  return machine.Completed();
}

bool pomo_seconds_event() {
  return machine.SecondsEvent();
}

uint16_t pomo_user_data(uint8_t index) {
  return machine.UserData(index);
}

void pomo_load_user_data() {
  ;
}

void pomo_apply_rollover(uint16_t data[5], uint16_t year, uint8_t month, uint8_t day, uint8_t day_of_week) {
  StateMachine::ApplyRollover(data, year, month, day, day_of_week);
}

uint8_t pomo_calc_weekday(int year, int month, int day) {
  return StateMachine::CalcWeekday(year, month, day);
}

const int8_t* pomo_display_digits() {
  return machine.Disp().LastDigits();
}

bool pomo_display_dot() {
  return machine.Disp().Dot();
}

const char* pomo_display_text() {
  return machine.Disp().LastText();
}
