#include "logic.h"

uint8_t cnt_digit(uint8_t pos) {
  uint8_t v = cnt_value();
  if (pos == 0) {
    return v % 10;
  }
  return v / 10;
}
