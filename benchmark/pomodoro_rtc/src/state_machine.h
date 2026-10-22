#ifndef StateMachine_h
#define StateMachine_h
#include "logic.h"
#include "soundbuzz.h"
#include "click.h"
#include "display4.h"
#include "rtsl.h"

class StateMachine {
public:
  StateMachine(RtcDS1307<TwoWire> *rtsl);
  void Init();
  void Reset();                       // fields back to their power-up values (testability)
  void Update();

  void Tick();

  void HandleAction(int8_t action);   // body of HandleClick for a given action
  int8_t ReadClick() { return button.GetClick(); }
  void LoadUserData();
  static void ApplyRollover(unsigned int *data, int year, int month, int day, int dayOfWeek);
  static byte CalcWeekday(int, int, int);

  // testability getters
  byte Status() const { return cur_status; }
  byte PrevStatus() const { return prev_status; }
  int8_t Minute() const { return pomodoro_minute; }
  int8_t Second() const { return second; }
  int8_t Completed() const { return pomodoro_completed; }
  bool SecondsEvent() const { return seconds_event; }
  unsigned int UserData(uint8_t i) const { return userData[i % 5]; }
  const Display4 &Disp() const { return disp; }

private:
  void Idle();
  void IncreaseTime();
  void Prepare();
  void Start();
  void Rest();
  void Pause();
  void Cancel();

  void HandleClick();
  void MinusSecond();
  void UpdateDisplay();
  void UpdatePomodoro(bool success = true);
  void UpdateStatus(int8_t newStatus);
  byte GetStatus();

  bool DisplayClick();
  void ClearDisplay();

  void PrintStats();

  SoundBuzz sound;
  Click button;
  Display4 disp;
  RTSL rtc = NULL;

  int8_t timeDisp[4];
  unsigned int userData[5];  // 0 - current_day_number, stats_for_today, stats_this_week (of a given month), stats_this_month, stats_total

  byte cur_status = STATUS_IDLE;
  byte prev_status = STATUS_IDLE;

  bool redraw = false;
  bool seconds_event = false;

  int8_t second = 59;
  byte idle_time = 0;
  int8_t pause_second = 0;
  int8_t pause_minute = 0;
  byte halfsecond = 0;

  int8_t lastClickDisplay = 0;

  int8_t pomodoro_minute = DEFAULT_TIME;
  int8_t pomodoro_minute_last = DEFAULT_TIME;
  int8_t pomodoro_completed = 0;  // track number of completed pomodors, after 3 pomodoros give rest 25 min
};

#endif
