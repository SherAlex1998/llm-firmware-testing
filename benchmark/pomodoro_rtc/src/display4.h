#ifndef Display4_h
#define Display4_h

#include <TM1637.h>

class Display4 {
public:
  Display4(int8_t clkPin, int8_t dioPin);

  void Init();
  void DisplayString(const char str[]);
  void DisplayNum(int num);
  void DisplayClock(int8_t timeDisp[]);
  void ClearDisplay();
  void DisplayDot(bool show);
  void DotNext();

  // testability: the display cannot be read back, so the last output is kept here
  const int8_t* LastDigits() const { return lastDigits; }
  bool Dot() const { return lastPointState; }
  const char* LastText() const { return lastText; }
  void ResetShadow() {
    lastPointState = false;
    for (uint8_t i = 0; i < 4; i++) lastDigits[i] = 0;
    lastText[0] = '\0';
  }

private:
  bool lastPointState = false;
  TM1637 tm1637;
  int8_t lastDigits[4] = {0, 0, 0, 0};
  char lastText[13] = "";
};

#endif
