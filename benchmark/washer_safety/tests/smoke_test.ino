// Hand-written infrastructure check (not shown to the LLM): EEPROM on avr8js, state machine through explicit inputs.
#include "logic.h"
#include "test_runner.h"

TEST(boots_idle_and_shows_ready) {
  washer_init();
  ASSERT_EQ(washer_state(), IDLE);
  ASSERT_TRUE(strncmp(washer_lcd_line(0), "Washer Ready", 12) == 0);
}

TEST(start_button_fills_with_relay_on_and_saves_state) {
  washer_init();
  washer_loop_with(1000, BTN1_ADC, true, false, 0);
  ASSERT_EQ(washer_state(), FILLING);
  washer_loop_with(1400, 0, true, false, 100);
  ASSERT_PIN(PIN_RELAY, HIGH);
  ASSERT_EQ(EEPROM_ADDR_STATE, 0);
}

TEST(restart_after_power_loss_goes_idle) {
  washer_init();
  ASSERT_EQ(washer_state(), IDLE);    // FILLING was saved by the previous test, init must not resume it
}

TEST(water_percent_interpolates) {
  ASSERT_EQ(washer_water_percent(465), 50);
  ASSERT_EQ(washer_decode_button(509), 2);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
