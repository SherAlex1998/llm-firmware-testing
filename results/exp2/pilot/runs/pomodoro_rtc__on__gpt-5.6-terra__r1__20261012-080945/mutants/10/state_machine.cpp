#include "state_machine.h"
#include "pace.h"

StateMachine::StateMachine(RtcDS1307<TwoWire> *rtsl)
  : sound(BTN_BUZZER), button(BTN_CLICK), disp(TM_CLK_PIN, TM_DIO_PIN) {
  rtc = RTSL(rtsl);
  for (uint8_t i = 0; i < 5; i++)
    userData[i] = 0;
  for (uint8_t i = 0; i < 4; i++)
    timeDisp[i] = 0;
}

void StateMachine::Reset() {
  for (uint8_t i = 0; i < 5; i++)
    userData[i] = 0;
  for (uint8_t i = 0; i < 4; i++)
    timeDisp[i] = 0;
  cur_status = prev_status = STATUS_IDLE;
  redraw = seconds_event = false;
  second = 59;
  idle_time = halfsecond = 0;
  pause_second = pause_minute = 0;
  lastClickDisplay = 0;
  pomodoro_minute = pomodoro_minute_last = DEFAULT_TIME;
  pomodoro_completed = 0;
  button = Click(BTN_CLICK);
  disp.ResetShadow();
}

void StateMachine::Init() {
  bool hardwareReset = digitalRead(BTN_CLICK) == HIGH ? true : false;
  rtc.Init();
  LoadUserData();
  disp.Init();
  sound.PlayInit();
  disp.DisplayString("HELL0");
  if (hardwareReset) {
    rtc.HardResetData();
  }
  Idle();
}

byte StateMachine::CalcWeekday(int year, int month, int day)
/* Calculate day of week in proleptic Gregorian calendar. Sunday == 0. */
{
  int adjustment, mm, yy;
  if (year < 2000) year += 2000;
  adjustment = (14 - month) / 12;
  mm = month + 12 * adjustment - 2;
  yy = year - adjustment;
  return (day + (13 * mm - 1) / 5 + yy + yy / 4 - yy / 100 + yy / 400) % 7;
}

void StateMachine::PrintStats() {
  disp.DisplayString("today _");
  disp.DisplayNum(userData[1]);
  paced_delay(1500);
  disp.DisplayString("week _");
  disp.DisplayNum(userData[2] + userData[1]);
  paced_delay(1500);
  disp.DisplayString("month _");
  disp.DisplayNum(userData[3] + userData[2] + userData[1]);
  paced_delay(1500);
  disp.DisplayString("all _");
  disp.DisplayNum(userData[4] + userData[3] + userData[2] + userData[1]);
  paced_delay(1500);
}

void StateMachine::LoadUserData() {
  rtc.GetData(userData);
  RtcDateTime now = rtc.GetTime();
  if (userData[0] == now.Day()) {
    pomodoro_completed = userData[1];  // load todays pomodoros
  } else {
    ApplyRollover(userData, now.Year(), now.Month(), now.Day(), now.DayOfWeek());
    rtc.SaveData(userData);
  }
}

// Statistics rollover when the stored day differs from today (data[0] == day: nothing to do).
void StateMachine::ApplyRollover(unsigned int *data, int year, int month, int day, int dayOfWeek) {
  if (data[0] == (unsigned int)day)
    return;
  bool rollover_month = false;
  bool rollover_week = false;
  bool rollover_day = false;

  if ((unsigned int)day > data[0]) {
    // same month, next day
    byte prev_day_of_week = CalcWeekday(year, month, data[0]);
    if (dayOfWeek <= prev_day_of_week || day - data[0] > 7) {
      rollover_week = true;
    }
    rollover_day = true;
  } else {
    // next month: rollover today, week, month
    rollover_month = rollover_week = rollover_day = true;
  }
  data[0] = day;  // set today
  if (rollover_day) {
    data[2] = data[2] + data[1];  // copy today`s pomodoro (old) to this week
    data[1] = 0;                  // reset today`s pomodoro
  }
  if (rollover_week) {
    data[3] = data[3] + data[2];  // copy this week`s pomodoro to this month
    data[2] = 0;                  // reset this week`s pomodoro
  }
  if (rollover_month) {
    data[4] = data[4] + data[3];  // copy this month`s pomodoro to all time total
    data[3] = 0;                  // reset this month`s pomodoro
  }
}

void StateMachine::UpdateStatus(int8_t newStatus) {
  prev_status = cur_status;
  cur_status = newStatus;
}

byte StateMachine::GetStatus() {
  return cur_status;
}

void StateMachine::Tick() {
  if (!seconds_event) {
    halfsecond++;
    if (halfsecond == 2) {
      halfsecond = 0;
      if (lastClickDisplay == 0) {
        second--;
        if (second < 0) {
          seconds_event = true;
          second = 59;
        }
      }
    }
    redraw = true;
  }
}

void StateMachine::IncreaseTime() {
  pomodoro_minute = pomodoro_minute + INCREASE_TIME;

  if (pomodoro_minute > MAX_TIME)
    pomodoro_minute = DEFAULT_TIME;
  pomodoro_minute_last = pomodoro_minute;
}

void StateMachine::Prepare() {
  second = 10;
  UpdateStatus(STATUS_PREPARE);
}

void StateMachine::Start() {
  pomodoro_minute--;
  second = 59;
  pause_minute = 5;
  UpdateStatus(STATUS_RUN_WORK);
  disp.DisplayDot(false);
  disp.ClearDisplay();
  sound.PlayStart();
}

void StateMachine::Rest() {
  disp.DisplayDot(false);
  disp.ClearDisplay();
  UpdatePomodoro();
  if (pomodoro_completed % 3 == 0)
    pomodoro_minute = 30;
  else
    pomodoro_minute = 5;
  UpdateStatus(STATUS_RUN_REST);
  pomodoro_minute--;
  second = 59;
  sound.PlayFinish();
}

void StateMachine::Pause() {
  if (cur_status != STATUS_RUN_PAUSE) {
    pause_second = second;
    second = 59;
    UpdateStatus(STATUS_RUN_PAUSE);
    disp.DisplayString("PAUSE");
  } else {
    second = pause_second;
    UpdateStatus(STATUS_RUN_WORK);
  }
  disp.DisplayDot(false);
  ClearDisplay();
}

void StateMachine::Cancel() {
  if (cur_status == STATUS_RUN_WORK)
    UpdatePomodoro(false);

  pomodoro_minute = pomodoro_minute_last;
  disp.DisplayDot(false);
  ClearDisplay();
  sound.PlayCancel();
  Idle();
}

void StateMachine::Idle() {
  UpdateStatus(STATUS_IDLE);
  redraw = true;
  pomodoro_minute = pomodoro_minute_last;
  disp.DisplayDot(false);
  disp.ClearDisplay();
}

void StateMachine::UpdatePomodoro(bool success) {
  if (success) {
    pomodoro_completed++;
    userData[1]++;
    rtc.SaveData(userData);
  } else
    pomodoro_completed = pomodoro_completed > 0 ? pomodoro_completed - 1 : 0;
}

void StateMachine::HandleClick() {
  HandleAction(button.GetClick());
}

void StateMachine::HandleAction(int8_t action) {
  if (action)
    switch (cur_status) {
      case STATUS_IDLE:
        if (action == 2)
          Prepare();
        else
          IncreaseTime();
        break;
      case STATUS_RUN_WORK:
        if (action == 2)
          Cancel();
        else
          Pause();
        break;
      case STATUS_PREPARE:
      case STATUS_RUN_REST:
        Cancel();
        break;
      case STATUS_RUN_PAUSE:
        Pause();
        break;
      default:
        break;
    }
}

void StateMachine::ClearDisplay() {
  for (int8_t i = 0; i < 4; i++)
    timeDisp[i] = 0x40;
}

bool StateMachine::DisplayClick() {
  int8_t clickDisplay = button.GetDisplay();

  if (clickDisplay <= 0) {
    if (clickDisplay != lastClickDisplay) {
      lastClickDisplay = clickDisplay;
      ClearDisplay();

      for (uint8_t i = 2; i <= clickDisplay; i++)
        timeDisp[i - 2] = '-';

      disp.DisplayDot(false);
      disp.DisplayClock(timeDisp);

      if (clickDisplay == 2)
        sound.PlayNote(NOTE_LOW);
      else if (clickDisplay == 5)
        sound.PlayNote(NOTE_HIGH);
    }
    return true;
  }
  lastClickDisplay = 0;
  return false;
}

void StateMachine::UpdateDisplay() {
  if (!DisplayClick())
    if (redraw) {
      switch (cur_status) {
        case STATUS_IDLE:
          disp.DisplayDot(false);
          timeDisp[0] = pomodoro_minute / 10;
          timeDisp[1] = pomodoro_minute % 10;
          timeDisp[2] = 0x40;
          timeDisp[3] = pomodoro_completed % 10;
          break;
        case STATUS_PREPARE:
          timeDisp[0] = 0x40;
          timeDisp[1] = 0x40;
          timeDisp[2] = second / 10;
          timeDisp[3] = second % 10;
          disp.DotNext();
          break;
        case STATUS_RUN_WORK:
        case STATUS_RUN_REST:
          timeDisp[0] = pomodoro_minute / 10;
          timeDisp[1] = pomodoro_minute % 10;
          timeDisp[2] = second / 10;
          timeDisp[3] = second % 10;
          disp.DotNext();
          break;
        case STATUS_RUN_PAUSE:
          ClearDisplay();
          disp.DotNext();
          break;
      }
      disp.DisplayClock(timeDisp);
      redraw = false;
    }
}

void StateMachine::MinusSecond() {
  switch (cur_status) {
    case STATUS_IDLE:
      idle_time++;
      if (idle_time == 5) {
        idle_time = 0;
        PrintStats();
      }
      break;
    case STATUS_PREPARE:
      Start();
      break;
    case STATUS_RUN_WORK:
    case STATUS_RUN_REST:
      if (pomodoro_minute == 0) {
        if (cur_status == STATUS_RUN_WORK) {
          Rest();
        } else {
          Idle();
          sound.PlaySecond();
        }
      } else {
        pomodoro_minute--;
      }
      break;
    case STATUS_RUN_PAUSE:
      pause_minute--;
      if (pause_minute < 0)
        Cancel();
      break;
  }
  seconds_event = false;
}

void StateMachine::Update() {
  if (seconds_event)
    MinusSecond();
  else {
    HandleClick();
    UpdateDisplay();
    if (second == 30) {
      if (cur_status == STATUS_RUN_WORK)
        disp.DisplayString("[11 11 11]");
      else if (cur_status == STATUS_RUN_REST)
        disp.DisplayString("[00 00 00]");
      else if (cur_status == STATUS_RUN_PAUSE)
        disp.DisplayString("PAUSE");
    }
  }
}
