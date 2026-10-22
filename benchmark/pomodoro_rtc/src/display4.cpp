#include "display4.h"
#include "pace.h"

Display4::Display4(int8_t clkPin, int8_t dioPin)
  : tm1637(clkPin, dioPin) {
}

void Display4::Init() {
  tm1637.set();
  tm1637.init();
}

void Display4::DisplayString(const char str[]) {
  strncpy(lastText, str, sizeof(lastText) - 1);
  lastText[sizeof(lastText) - 1] = '\0';
  tm1637.point(POINT_OFF);
  tm1637.displayStr((char*)str, paced(500));   // longer texts scroll, 500 ms per step originally
  tm1637.point(lastPointState);
}

void Display4::DisplayNum(int num) {
  tm1637.point(POINT_OFF);
  tm1637.displayNum(num);
  tm1637.point(lastPointState);
}

void Display4::DisplayClock(int8_t timeDisp[]) {
  for (uint8_t i = 0; i < 4; i++)
    lastDigits[i] = timeDisp[i];
  tm1637.display(timeDisp);
}

void Display4::ClearDisplay() {
  for (uint8_t i = 0; i < 4; i++)
    lastDigits[i] = 0x7f;   // blank (the library's clear code)
  tm1637.clearDisplay();
}

void Display4::DisplayDot(bool show) {
  if (show) {
    tm1637.point(POINT_ON);
    lastPointState = true;
  } else {
    tm1637.point(POINT_OFF);
    lastPointState = false;
  }
}

void Display4::DotNext() {
  DisplayDot(!lastPointState);
}
