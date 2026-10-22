#include "logic.h"

#include <LiquidCrystal.h>
#include <Stepper.h>
#include <EEPROM.h>

static LiquidCrystal lcd(4, 5, 6, 7, 8, 9);
//Stepper 라이브러리는 (IN1, IN3, IN2, IN4) 순서로 넣어야 정방향 회전한다
//실제 배선: A2=IN1, A3=IN2, A4=IN3, A5=IN4
static Stepper stepper(2048, A2, A4, A3, A5);

static State currentState = IDLE;

//---------------- 물높이 (실측 기반) ----------------
//실측값: 건조 0 / 소량 410 / 중간 520 / 목표 수위(종이컵 절반) 660
static const int waterAdcPoints[]     = {0, 410, 520, 660};
static const int waterPercentPoints[] = {0, 33, 67, 100};
static const int WATER_POINTS_COUNT   = 4;

#define DOOR_CLOSED_LEVEL LOW

static unsigned long lastBtnTime = 0;
static unsigned long doorOpenSince = 0;
static bool doorWasClosed = true;
static State pausedFromState;
static unsigned long pausedElapsed = 0;
static bool pausedByDoor = false;
static bool pausedByImbalance = false;
static bool fillCompleteShown = false;
static unsigned long fillCompleteTime = 0;
static bool showCloseDoorMsg = false;
static unsigned long closeDoorMsgTime = 0;
static unsigned long lastLcdUpdate = 0;
static unsigned long stateTimer = 0;

// Inputs of the current loop pass (testability: washer_loop() reads them from the hardware,
// washer_loop_with() takes them as arguments). The original called millis()/digitalRead/analogRead in place.
static unsigned long g_now = 0;
static int g_btn_adc = 0;
static bool g_door_closed = true;
static bool g_tilt_high = false;
static int g_water_adc = 0;

// LCD shadow (the display cannot be read back)
static char shadow[2][17];
static uint8_t cur_col = 0, cur_row = 0;
static bool lcdOn = true;

static void lcd_clear() {
  lcd.clear();
  for (uint8_t r = 0; r < 2; r++) {
    memset(shadow[r], ' ', 16);
    shadow[r][16] = '\0';
  }
  cur_col = cur_row = 0;
}

static void lcd_set_cursor(uint8_t col, uint8_t row) {
  lcd.setCursor(col, row);
  cur_col = col;
  cur_row = row;
}

static void lcd_print(const char* text) {
  lcd.print(text);
  for (uint8_t i = 0; text[i] != '\0'; i++, cur_col++) {
    if (cur_col < 16) shadow[cur_row][cur_col] = text[i];
  }
}

static void lcd_print(long value) {
  char buf[12];
  ltoa(value, buf, 10);
  lcd_print(buf);
}

//---------------- 부저 멜로디 (수동부저, tone 사용) ----------------
static const int startNotes[] = {880, 1175}; //시작음: 짧게 상승
static const unsigned int startDur[] = {100, 150};
static const int startLen = 2;
static const int alarmNotes[] = {1800, 0, 1800, 0}; //경보음: 높은음 반복 (0은 무음 구간)
static const unsigned int alarmDur[] = {120, 80, 120, 400};
static const int alarmLen = 4;
static const int completeNotes[] = {523, 659, 784, 1047}; //완료음: 도-미-솔-도
static const unsigned int completeDur[] = {150, 150, 150, 300};
static const int completeLen = 4;
static int activeMelody = -1; //-1=없음, 0=시작, 1=경보, 2=완료
static int melodyIndex = 0;
static unsigned long melodyNoteStart = 0;
static bool melodyPlaying = false;

static void playNoteAt(int melodyType, int idx) {
  int freq = 0;
  if (melodyType == 0) freq = startNotes[idx];
  else if (melodyType == 1) freq = alarmNotes[idx];
  else if (melodyType == 2) freq = completeNotes[idx];
  if (freq > 0) tone(PIN_BUZZER, freq);
  else noTone(PIN_BUZZER);
}

static unsigned int getDurationAt(int melodyType, int idx) {
  if (melodyType == 0) return startDur[idx];
  if (melodyType == 1) return alarmDur[idx];
  if (melodyType == 2) return completeDur[idx];
  return 100;
}

static int getMelodyLen(int melodyType) {
  if (melodyType == 0) return startLen;
  if (melodyType == 1) return alarmLen;
  if (melodyType == 2) return completeLen;
  return 0;
}

static void startMelodyPlay(int melodyType) {
  activeMelody = melodyType;
  melodyIndex = 0;
  melodyPlaying = true;
  melodyNoteStart = g_now;
  playNoteAt(activeMelody, 0);
}

static void stopMelodyPlay() {
  melodyPlaying = false;
  activeMelody = -1;
  noTone(PIN_BUZZER);
}

//매 루프 호출. tone()은 백그라운드로 소리를 내므로 음 전환 타이밍만 millis로 관리하면 루프를 막지 않는다
static void updateMelody() {
  if (!melodyPlaying || activeMelody == -1) return;
  unsigned int dur = getDurationAt(activeMelody, melodyIndex);
  if (g_now - melodyNoteStart >= dur) {
    melodyIndex++;
    int len = getMelodyLen(activeMelody);
    if (melodyIndex >= len) {
      if (activeMelody == 1) {
        melodyIndex = 0; //경보음만 무한 반복
      } else {
        noTone(PIN_BUZZER);
        melodyPlaying = false;
        return;
      }
    }
    melodyNoteStart = g_now;
    playNoteAt(activeMelody, melodyIndex);
  }
}

//---------------- 도어 / LED ----------------
static void setDoorLED(bool locked) {
  digitalWrite(PIN_LED_LOCK, locked ? HIGH : LOW);
  digitalWrite(PIN_LED_OPEN, locked ? LOW : HIGH);
}

//---------------- 상태 관리 ----------------
bool washer_is_running_state(State s) {
  return (s == FILLING || s == WASHING || s == SPINNING);
}

static void saveState(State s) { EEPROM.update(EEPROM_ADDR_STATE, (byte)s); }

static void changeState(State s) {
  currentState = s;
  stateTimer = g_now;
  saveState(s);
}

//일시정지 진입. 원인 구분: 0 = 사용자가 버튼으로 멈춤 (경보 없음), 1 = 동작 중 문 열림 (경보), 2 = 탈수 중 진동 감지 (경보)
static void enterPaused(int reason) {
  pausedFromState = currentState;
  pausedElapsed = g_now - stateTimer;
  pausedByDoor = (reason == 1);
  pausedByImbalance = (reason == 2);
  digitalWrite(PIN_RELAY, LOW);
  currentState = PAUSED;
  saveState(PAUSED);
  if (reason == 1 || reason == 2) startMelodyPlay(1);
}

//절전 진입: 대기 상태에서만 가능. 릴레이/LED를 끄고 LCD 표시를 끈다.
static void enterSleepMode() {
  stopMelodyPlay();
  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_LED_LOCK, LOW);
  digitalWrite(PIN_LED_OPEN, LOW);
  lcd.noDisplay(); //백라이트는 결선상 못 끄고, 화면 표시만 끈다
  lcdOn = false;
  currentState = SLEEPING;
}

//절전 해제: 버튼2를 다시 누르면 대기 상태로 복귀
static void exitSleepMode() {
  lcd.display();
  lcdOn = true;
  currentState = IDLE;
  setDoorLED(g_door_closed);
}

void washer_init() {
  pinMode(PIN_DOOR, INPUT); //3핀 모듈이라 자체 풀업 있음
  pinMode(PIN_TILT, INPUT_PULLUP); //2핀 단품이라 내부 풀업 사용
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED_LOCK, OUTPUT);
  pinMode(PIN_LED_OPEN, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);
  lcd.begin(16, 2);
  lcd.display();
  lcdOn = true;
  stepper.setSpeed(10);
  // power-up values of the loop state (testability: washer_init() can be called again)
  lastBtnTime = doorOpenSince = pausedElapsed = fillCompleteTime = closeDoorMsgTime = lastLcdUpdate = stateTimer = 0;
  doorWasClosed = true;
  pausedByDoor = pausedByImbalance = fillCompleteShown = showCloseDoorMsg = false;
  activeMelody = -1;
  melodyIndex = 0;
  melodyNoteStart = 0;
  melodyPlaying = false;
  noTone(PIN_BUZZER);
  g_now = millis();
  //마지막 상태를 EEPROM에서 복원한다. 동작 중이거나 완료/알람/일시정지/절전으로 저장돼 있었다면 전부 대기로 되돌린다.
  byte saved = EEPROM.read(EEPROM_ADDR_STATE);
  if (saved <= SLEEPING && (washer_is_running_state((State)saved) || saved == DONE || saved == ALARM || saved == PAUSED || saved == SLEEPING)) {
    currentState = IDLE;
  } else if (saved <= SLEEPING) {
    currentState = (State)saved;
  } else {
    currentState = IDLE; //에러 방지 (EEPROM 초기값 255 등)
  }
  g_door_closed = (digitalRead(PIN_DOOR) == DOOR_CLOSED_LEVEL);
  setDoorLED(g_door_closed);
  lcd_clear();
  lcd_print("Washer Ready");
}

int washer_decode_button(int adc) {
  int btn = 0;
  if (abs(adc - BTN1_ADC) < BTN_TOLERANCE) btn = 1;
  else if (abs(adc - BTN2_ADC) < BTN_TOLERANCE) btn = 2;
  else if (abs(adc - BTN3_ADC) < BTN_TOLERANCE) btn = 3;
  return btn;
}

//버튼 읽기. 실측 ADC값과 비교해서 어느 버튼인지 판별
static int readButton() {
  int adc = g_btn_adc;
  if (g_now - lastBtnTime < BTN_DEBOUNCE) return 0;
  int btn = washer_decode_button(adc);
  if (btn != 0) lastBtnTime = g_now;
  return btn;
}

//기울기센서: 평상시 LOW, 흔들리면 HIGH (실측 확인)
static bool checkImbalance() {
  return g_tilt_high;
}

//ADC -> % 변환. 실측 4점 사이를 구간별로 직선 보간
int washer_water_percent(int adc) {
  if (adc <= waterAdcPoints[0]) return waterPercentPoints[0];
  if (adc >= waterAdcPoints[WATER_POINTS_COUNT - 1]) return waterPercentPoints[WATER_POINTS_COUNT + 1];
  for (int i = 0; i < WATER_POINTS_COUNT - 1; i++) {
    if (adc >= waterAdcPoints[i] && adc <= waterAdcPoints[i + 1]) {
      long adcRange = waterAdcPoints[i + 1] - waterAdcPoints[i];
      long pctRange = waterPercentPoints[i + 1] - waterPercentPoints[i];
      long offset = adc - waterAdcPoints[i];
      return waterPercentPoints[i] + (offset * pctRange) / adcRange;
    }
  }
  return 0;
}

static void updateLcd() {
  if (currentState == SLEEPING) return; // 절전 중엔 화면 자체를 안 씀
  if (g_now - lastLcdUpdate < LCD_INTERVAL) return;
  lastLcdUpdate = g_now;
  lcd_clear();
  lcd_set_cursor(0, 0);
  switch (currentState) {
    case IDLE: lcd_print("1.Standby"); break;
    case FILLING: lcd_print("2.Filling..."); break;
    case WASHING: lcd_print("3.Washing..."); break;
    case SPINNING: lcd_print("4.Spinning..."); break;
    case DONE: lcd_print("5.Complete!"); break;
    case ALARM: lcd_print("! ALARM !"); break;
    case PAUSED:
      if (pausedByDoor) lcd_print("!PAUSED(Door)!");
      else if (pausedByImbalance) lcd_print("!PAUSED(Shake)!");
      else lcd_print("-- PAUSED --");
      break;
    default: break;
  }
  lcd_set_cursor(0, 1);
  if (currentState == IDLE) {
    if (showCloseDoorMsg && g_now - closeDoorMsgTime < CLOSE_DOOR_MSG_DURATION) {
      lcd_print("Close the door");
    } else {
      showCloseDoorMsg = false;
      lcd_print("Btn2:Sleep");
    }
  } else if (currentState == PAUSED) {
    //문이 열려 있으면 재개 불가 상태라는 걸 안내
    lcd_print(g_door_closed ? "Btn3:Resume" : "Close the door");
  } else if (currentState == FILLING) {
    //5회 평균으로 표시 잔떨림 완화 (one reading per pass here: g_water_adc)
    int avgWater = 0;
    for (int i = 0; i < 5; i++) avgWater += g_water_adc;
    avgWater /= 5;
    lcd_print("Water:");
    lcd_print((long)washer_water_percent(avgWater));
    lcd_print("%");
  } else if (currentState == WASHING || currentState == SPINNING) {
    //남은 시간(초) 표시. 일시정지 후 재개해도 이어서 계산된다
    unsigned long duration = (currentState == WASHING) ? WASH_DURATION : SPIN_DURATION;
    unsigned long elapsed = g_now - stateTimer;
    long remaining = (long)(duration - elapsed) / 1000;
    if (remaining < 0) remaining = 0;
    lcd_print("Time left:");
    lcd_print(remaining);
    lcd_print("s");
  }
}

static void loop_pass() {
  bool doorClosedNow = g_door_closed;
  //도어 감시는 동작 중(급수/세탁/탈수)에만 한다.
  if (washer_is_running_state(currentState)) {
    if (!doorClosedNow) {
      if (doorWasClosed) {
        doorOpenSince = g_now; //열리기 시작한 시점 기록
        doorWasClosed = false;
      }
      //2초 이상 계속 열려 있어야 확정(짧은 접촉은 무시)
      if (g_now - doorOpenSince > DOOR_OPEN_CONFIRM_TIME) {
        enterPaused(1);
      }
    } else {
      doorWasClosed = true; //중간에 닫히면 타이머 리셋
    }
  } else {
    doorWasClosed = true;
  }
  int btn = readButton();
  switch (currentState) {
    case IDLE:
      digitalWrite(PIN_RELAY, LOW);
      setDoorLED(doorClosedNow); //LED는 오직 실제 문 상태만 반영
      if (btn == 2) {
        enterSleepMode();
        break;
      }
      if (btn == 1) {
        if (doorClosedNow) {
          changeState(FILLING);
          startMelodyPlay(0);
        } else {
          //문 열린 채 시작 시도 -> 안내만 띄우고 대기 유지 (도어 인터록)
          showCloseDoorMsg = true;
          closeDoorMsgTime = g_now;
        }
      }
      break;
    //절전 상태: 버튼2를 다시 누르면 즉시 복귀. 그 외엔 아무것도 안 함
    case SLEEPING:
      if (btn == 2) {
        exitSleepMode();
      }
      break;
    case FILLING: {
      //급수 완료 전까지만 릴레이 ON 유지.
      if (!fillCompleteShown) {
        digitalWrite(PIN_RELAY, HIGH);
      }
      int rawWater = g_water_adc;
      if (rawWater >= WATER_FULL_THRESHOLD) {
        if (!fillCompleteShown) {
          fillCompleteShown = true;
          fillCompleteTime = g_now;
          digitalWrite(PIN_RELAY, LOW); //밸브는 즉시 잠금
        }
        //100% 화면을 잠깐 보여준 뒤 다음 단계로
        if (g_now - fillCompleteTime > FILL_COMPLETE_HOLD) {
          fillCompleteShown = false;
          changeState(WASHING);
        }
      } else if (g_now - stateTimer > FILL_DURATION) {
        //제한시간 내에 목표 수위 도달 실패 = 밸브/센서 이상으로 간주
        digitalWrite(PIN_RELAY, LOW);
        changeState(ALARM);
        startMelodyPlay(1);
      }
      if (btn == 3) enterPaused(0);
      break;
    }
    case WASHING:
      if (g_now - stateTimer > WASH_DURATION) {
        changeState(SPINNING);
      }
      if (btn == 3) enterPaused(0);
      break;
    case SPINNING:
      //한 번에 50스텝씩만 돌려서 루프 지연을 체감 안 되는 수준으로 유지.
      stepper.step(50);
      if (checkImbalance()) {
        enterPaused(2);
        break; //같은 루프에서 아래 완료 판정이 PAUSED를 덮어쓰지 않도록
      }
      if (g_now - stateTimer > SPIN_DURATION) {
        changeState(DONE);
        startMelodyPlay(2);
        break;
      }
      if (btn == 3) enterPaused(0);
      break;
    case PAUSED:
      digitalWrite(PIN_RELAY, LOW);
      setDoorLED(doorClosedNow);
      //재개 조건: 문이 닫힌 상태에서 버튼3. 진동으로 멈춘 경우엔 탈수를 처음부터 다시 돌린다
      if (btn == 3 && doorClosedNow) {
        stopMelodyPlay();
        currentState = pausedFromState;
        if (pausedByImbalance) {
          stateTimer = g_now; //탈수 처음부터
        } else {
          stateTimer = g_now - pausedElapsed; //멈춘 지점부터 이어서
        }
        saveState(currentState);
      }
      break;
    case DONE:
      setDoorLED(false); //완료 후 도어 잠금 해제 표시
      if (btn == 1 || btn == 3) {
        stopMelodyPlay();
        changeState(IDLE);
      }
      break;
    case ALARM:
      digitalWrite(PIN_RELAY, LOW);
      if (btn == 1 || btn == 3) {
        stopMelodyPlay();
        changeState(IDLE);
      }
      break;
  }
  updateMelody();
  updateLcd();
}

void washer_loop() {
  washer_loop_with(millis(), analogRead(PIN_BTN), digitalRead(PIN_DOOR) == DOOR_CLOSED_LEVEL,
                   digitalRead(PIN_TILT) == HIGH, analogRead(PIN_WATER));
}

void washer_loop_with(uint32_t now_ms, int btn_adc, bool door_closed, bool tilt_high, int water_adc) {
  g_now = now_ms;
  g_btn_adc = btn_adc;
  g_door_closed = door_closed;
  g_tilt_high = tilt_high;
  g_water_adc = water_adc;
  loop_pass();
}

State washer_state() { return currentState; }
State washer_paused_from() { return pausedFromState; }
bool washer_paused_by_door() { return pausedByDoor; }
bool washer_paused_by_imbalance() { return pausedByImbalance; }
int washer_active_melody() { return activeMelody; }
bool washer_melody_playing() { return melodyPlaying; }
bool washer_lcd_on() { return lcdOn; }
const char* washer_lcd_line(uint8_t row) { return shadow[row % 2]; }
