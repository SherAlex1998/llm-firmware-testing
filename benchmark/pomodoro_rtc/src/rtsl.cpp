#include "rtsl.h"

RTSL::RTSL(RtcDS1307<TwoWire>* rtsl) {
  rtc = rtsl;
}

void RTSL::HardResetData() {
  RtcDateTime now = rtc->GetDateTime();
  unsigned int empty[] = { now.Day(), 0, 0, 0, 0 };
  SaveData(empty);
}

void RTSL::SaveData(unsigned int* num2) {
  unsigned int num[5] = { num2[0], num2[1], num2[2], num2[3], num2[4] };
  rtc->SetMemory(0, MEMORY_ADDRESS);
  uint8_t written = rtc->SetMemory(MEMORY_ADDRESS, (const uint8_t*)num, 10);
  rtc->SetMemory(1, written);
}

void RTSL::GetData(unsigned int* num) {
  uint8_t address = rtc->GetMemory(0);
  if (address != MEMORY_ADDRESS) {
    HardResetData();
  } else {
    uint8_t count = rtc->GetMemory(1);
    uint8_t buff[10];

    uint8_t gotten = rtc->GetMemory(address, buff, count);
    if (gotten == count) {
      byte y = 0;
      for (int8_t i = 0; i < 5; i++) {
        num[i] = buff[y + 1];
        num[i] = num[i] << 8;
        num[i] |= buff[y];
        y = y + 2;
      }
    }
  }
}

void RTSL::Init() {
  rtc->Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  if (!rtc->IsDateTimeValid()) {
    if (rtc->LastError() == 0) {
      // RTC lost confidence in the DateTime
      rtc->SetDateTime(compiled);
    }
  }

  if (!rtc->GetIsRunning()) {
    rtc->SetIsRunning(true);
  }

  if (rtc->GetDateTime() < compiled) {
    // RTC is older than compile time
    rtc->SetDateTime(compiled);
  }

  rtc->SetSquareWavePin(DS1307SquareWaveOut_Low);
}

RtcDateTime RTSL::GetTime() {
  return rtc->GetDateTime();
}
