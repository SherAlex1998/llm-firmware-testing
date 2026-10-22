// Hand-written infrastructure check (not shown to the LLM): LCD present on I2C, menu logic reachable.
#include "logic.h"
#include "test_runner.h"
#include <Wire.h>

TEST(lcd_answers_on_i2c) {
  menu_init();
  Wire.beginTransmission(0x27);
  ASSERT_EQ(Wire.endTransmission(), 0);
}

TEST(long_click_enters_setup_and_renders_first_page) {
  menu_init();
  menu_long_click(0, 0);
  ASSERT_TRUE(menu_setup_mode());
  menu_render();
  ASSERT_TRUE(strncmp(menu_lcd_line(0), "Enable:       true", 18) == 0);
  ASSERT_TRUE(strncmp(menu_lcd_line(2), "Time 1:       02500", 19) == 0);
}

TEST(delta_edits_selected_variable) {
  menu_init();
  menu_long_click(0, 0);
  menu_single_click();
  menu_single_click();
  menu_apply_delta(-3);
  ASSERT_EQ(menu_value(2), 2497);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
