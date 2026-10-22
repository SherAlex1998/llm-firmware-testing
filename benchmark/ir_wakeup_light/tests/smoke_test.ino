// Hand-written infrastructure check (not shown to the LLM): LCD at 0x3F, RTC with the fixed time, IR code handling.
#include "logic.h"
#include "test_runner.h"
#include <Wire.h>

TEST(lcd_and_rtc_on_the_bus) {
  wake_init();
  Wire.beginTransmission(0x3F);
  ASSERT_EQ(Wire.endTransmission(), 0);
  ASSERT_EQ(hour, 6);
  ASSERT_EQ(minute, 29);
}

TEST(play_button_turns_light_on) {
  wake_init();
  HandleIRCode(0xFFA25D);
  ASSERT_TRUE(lightOn);
  ASSERT_EQ(activeLED, WhiteLEDStrip);
}

TEST(next_button_increases_alarm_minute) {
  wake_init();
  wake_loop_with(1000, 0xFF906F);
  ASSERT_EQ(alarmMinute, 31);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
