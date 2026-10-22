// Hand-written infrastructure check (not shown to the LLM): LCD on I2C, buttons released, decoding reachable.
#include "logic.h"
#include "test_runner.h"
#include <Wire.h>

TEST(lcd_answers_and_buttons_released) {
  morse_init();
  Wire.beginTransmission(0x27);
  ASSERT_EQ(Wire.endTransmission(), 0);
  morse_loop();
  ASSERT_EQ(getToneFromButtonStates(), 0);
}

TEST(dot_dash_separator_types_A) {
  morse_init();
  morse_loop_with(0, 1, 0);
  morse_loop_with(0, 0, 0);   // dot released
  morse_loop_with(1, 0, 0);
  morse_loop_with(0, 0, 0);   // dash released
  morse_loop_with(0, 0, 1);
  morse_loop_with(0, 0, 0);   // separator released
  ASSERT_TRUE(strcmp(morse_text(), "A") == 0);
  ASSERT_TRUE(morse_lcd_line(0)[0] == 'A');
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
