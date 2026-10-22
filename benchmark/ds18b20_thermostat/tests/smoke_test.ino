// Hand-written infrastructure check (not shown to the LLM): DS18B20 and LCD on the bus, fresh EEPROM, hysteresis.
#include "logic.h"
#include "test_runner.h"

TEST(fresh_eeprom_loads_defaults_and_warns) {   // must run first: thermo_init() stores the defaults
  thermo_init();
  ASSERT_EQ(stage, 1);
  thermo_loop_with(1000, LOW, LOW, LOW, 19.0);
  ASSERT_TRUE(strncmp(thermo_lcd_line(0), "***** ATENCAO! *****", 20) == 0);
}

TEST(sensor_reads_fixed_temperature) {
  thermo_init();
  ASSERT_EQ(stage, 0);           // settings now valid in EEPROM
  float t = getTemp();
  if (t > 80) t = getTemp();     // the first read may return the power-on value
  ASSERT_NEAR(t * 10, 215, 2);
}

TEST(hot_turns_cooling_on) {
  thermo_init();
  thermo_loop_with(2000, LOW, LOW, LOW, 25.0);
  ASSERT_PIN(14, LOW);
  ASSERT_PIN(8, HIGH);
}

void setup() {
  Serial.begin(9600);
  RUN_ALL_TESTS();
}

void loop() {}
