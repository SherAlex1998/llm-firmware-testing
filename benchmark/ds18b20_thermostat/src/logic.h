// Fermentation thermostat: DS18B20 temperature, cooling/heating relays, 20x4 I2C LCD menu with three buttons,
// settings kept in EEPROM. Source: https://github.com/IoBeer/thermostat-arduino (Thermostat/Thermostat.ino),
// Apache License 2.0, (C) Copyright 2016 Smart Homebrew by Antonio Marin Neto. Refactored for testability
// (see module.yaml notes). LCD texts are in Portuguese, as in the original.
#pragma once
#include <Arduino.h>

#define CONFIG_VERSION "ls1"   // ID of the settings block
#define CONFIG_START 32        // settings start address in EEPROM

struct StoreStruct {
  float fermentationTemp;      // target temperature, default 19.0
  float tempDelta;             // allowed deviation, default 0.5
  char tempUnit;
  long startedWhen;
  char version_of_program[4];
};

extern StoreStruct settings;
extern int stage;              // 0 reading temperature, 1 default values message, 2 configure temperature, 3 configure delta
extern bool waitingCommand;    // true while a message/configuration screen waits for buttons
extern long pressedSince;      // when SET started being held (0 = not held)
extern int stateButton;
extern long waiting;           // time of the last configuration change (idle timeout 5 s)
extern float tempAux;          // value being configured
extern bool invalidateLCD;
extern int blueLed, redLed;            // 8, 9
extern int setButton, upButton, downButton;  // 2, 3, 4 (INPUT, HIGH = pressed)
extern int SensorPin;                  // 12 (OneWire)
extern int relay1, relay2;             // 14 (A0) cooling, 15 (A1) heating; LOW = relay on

void thermo_reset_state(void);         // stage, flags and settings back to power-up values (thermo_init calls it)
void thermo_init(void);                // setup(): pins, settings from EEPROM (defaults + stage 1 if none), LCD
void thermo_loop(void);                // one loop() pass reading the buttons, millis() and the sensor
void thermo_loop_with(unsigned long now_ms, int set, int up, int down, float temp_c);  // the same pass with given inputs
void readingTemperature();
void confirmDefaultValuesLoaded();
void startConfigurationWizard();
void configureMaxDelta();
void updateTempConfiguration();
void loadConfig();
void saveConfig();
void resetEEPROM();                    // writes 0 to all 1024 EEPROM bytes
float getTemp();                       // DS18B20 reading in degC, -1000 if no valid sensor
const char *thermo_lcd_line(uint8_t row);  // text on LCD row 0..3: 20 characters, the degree sign is char 223
