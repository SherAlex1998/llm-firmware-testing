/**
 * (C) Copyright 2016 Smart Homebrew by Antonio Marin Neto
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Author: Neto Marin
* E-mail: netomarin@gmail.com
*/
#include "logic.h"
#include <EEPROM.h>
#include <avr/eeprom.h>
#include <OneWire.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Testability: inputs of the current loop pass (thermo_loop() reads the pins and millis(), thermo_loop_with()
// takes them as arguments; the original called digitalRead()/millis() in place) and an LCD shadow.
static unsigned long g_now = 0;
static int g_set = LOW, g_up = LOW, g_down = LOW;
static bool g_temp_fixed = false;
static float g_temp = 0;
static char shadow[4][21];
static uint8_t cur_col = 0, cur_row = 0;

static int btn(int pin);
static void lcd_clear();
static void lcd_set_cursor(uint8_t col, uint8_t row);
static void lcd_print(const char *text);
static void lcd_put(char c);

// ID of the settings block
#define CONFIG_VERSION "ls1"
// Tell it where to store your config data in EEPROM
#define CONFIG_START 32

//Setup configuration (struct StoreStruct is in logic.h)
StoreStruct settings = {
  // The default values
  19.0,
  0.5,
  'C',
  0,
  CONFIG_VERSION
};

/**
 * Variable to control stage/flow
 * 0 - Reading temperature
 * 1 - Default value loaded message
 * 2 - Configuration screen
 */
int stage = 0;
bool waitingCommand = false;
long pressedSince;
int stateButton;
long waiting;

//Aux variables
float tempAux;

//LCD message state
bool invalidateLCD;

//indicator leds configuration
int blueLed = 8;
int redLed = 9;

//Push buttons
int setButton = 2;
int upButton = 3;
int downButton = 4;

//Temperature sensor configuration
int SensorPin = 12;
static char outstr[8];   // was [4]: dtostrf(x, 4, 1) writes 5+ bytes
OneWire ds(SensorPin);

//relay 1 connected to port A0
int relay1 = 14;
//relay 2 connected to port A1
int relay2 = 15;

//LCD setup
#define I2C_ADDR    0x27

static LiquidCrystal_I2C lcd(I2C_ADDR, 20, 4);

//Operational variables
float fermentationTemp;
float tempVariation;

void thermo_init(void) {
  thermo_reset_state();

  //Indicator leds outputs
  pinMode(blueLed, OUTPUT);
  pinMode(redLed, OUTPUT);

  //push buttons
  pinMode(setButton, INPUT);
  pinMode(upButton, INPUT);
  pinMode(downButton, INPUT);

  //Defining relay outputs
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);

  //configuring temperature parameters
  loadConfig();

  //Starting LCD
  lcd.init();
  lcd.backlight();
  invalidateLCD = true; //first time, invalidating to be drawn
}

static void loop_pass(void) {

  if (!(!waitingCommand)) {
    /*
     * Application's flow control
     */
    switch (stage) {
      case 0:
        readingTemperature();
        break;
      case 1:
        confirmDefaultValuesLoaded();
        break;
      case 2:
        startConfigurationWizard();
        break;
      case 3:
        configureMaxDelta();
        break;
    }
  } else {
    /**
     * Command's control
     */

    /**
     * SET button
     */
    if (btn(setButton) == HIGH) {
      waitingCommand = false;
      if (stage == 1) {
        stage = 0;
      } else if (stage == 2) {
        settings.fermentationTemp = tempAux;
        saveConfig();
        stage = 3;
      } else if (stage == 3) {
        settings.tempDelta = tempAux;
        saveConfig();
        stage = 0;
        invalidateLCD = true;
      }
    }

    /**
     * UP button
     */
    if (btn(upButton) == HIGH) {
      if (stage == 2  || stage == 3) {
        tempAux = (tempAux + 0.1);

        //checking if new value and reseting if it's too high
        if (stage == 2 && tempAux >= 40.1) {
          tempAux = -5.0;
        } else if (stage == 3 && tempAux >= 5.1) {
          tempAux = 5.0;
        }
        //updating LCD
        updateTempConfiguration();
      }
    }

    /**
     * DOWN button
     */
    if (btn(downButton) == HIGH) {
      if (stage == 2 || stage == 3) {
        tempAux = (tempAux - 0.1);

        //checking if new value and reseting if it's too low
        if (stage == 2 && tempAux <= -5.1) {
          tempAux = 40.0;
        } else if (stage == 3 && tempAux <= 0.1) {
          tempAux = 0.2;
        }
        //updating LCD
        updateTempConfiguration();
      }
    }

    /*
     * return to stage 0 (read and show temperature) after 5 seconds of idle
     */
    if (stage > 1 && (g_now - waiting) >= 5000) {
      stage = 0;
      invalidateLCD = true;
      waitingCommand = false;
    }
  }

  //Detecting hold on set button
  stateButton = btn(setButton);
  if ( stateButton == HIGH) {
    if (pressedSince == 0) {
      //started holding
      pressedSince = g_now;
    } else {
      //set button pressed during 3 seconds
      if ((g_now - pressedSince) >= 3000) {
        stage = 2;
      }
    }
  } else if (pressedSince > 0) {
    //reseting after release the button
    pressedSince = 0;
  }

  //Hidden command to reset configuration
  if (btn(setButton) == HIGH && btn(upButton) == HIGH && btn(downButton) == HIGH) {
    resetEEPROM();
  }
}

/*******************************************************************************************************
 * Stage 0: Reading and showing information about temperatures
 *******************************************************************************************************/
void readingTemperature() {
  if (invalidateLCD) {
    invalidateLCD = false;
    // Starting and configuring LCD
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Temperatura:");
    lcd_set_cursor(0, 1);
    lcd_print("Temp. ideal: ");
    dtostrf(settings.fermentationTemp, 4, 1, outstr);
    lcd_set_cursor(14, 1);
    lcd_print(outstr);
    lcd_put(223);
    lcd_print("C");
    lcd_set_cursor(0, 2);
    lcd_print("Delta max.: ");
    dtostrf(settings.tempDelta, 4, 1, outstr);
    lcd_set_cursor(14, 2);
    lcd_print(outstr);
    lcd_put(223);
    lcd_print("C");
    lcd_set_cursor(0, 3);
    lcd_print("SmartHomebrew@GitHub");
  }

  float temp = g_temp_fixed ? g_temp : getTemp();
  lcd_set_cursor(14, 0);
  dtostrf(temp, 4, 1, outstr);
  lcd_print(outstr);
  lcd_put(223);
  lcd_print("C");

  if (temp >= (settings.fermentationTemp + settings.tempDelta)) {
    //Temperature is higher than desired, let's cool down!
    //turn on relay 1 - turn off relay 2 - turn on blue led
    digitalWrite(relay1, LOW);
    digitalWrite(relay2, HIGH);
    digitalWrite(blueLed, HIGH);
    digitalWrite(redLed, LOW);
  } else if (temp <= (settings.fermentationTemp - settings.tempDelta)) {
    //Temperature is lower than desired, let's warm up!
    //turn on relay 2 - turn off relay 1 - turn on red led
    digitalWrite(relay1, HIGH);
    digitalWrite(relay2, LOW);
    digitalWrite(blueLed, LOW);
    digitalWrite(redLed, HIGH);
  }
}

/*******************************************************************************************************
 * Stage 1: Warning user about default values
 *          It could be dangerous to the fermentation proccess.
 *******************************************************************************************************/
void confirmDefaultValuesLoaded() {
  lcd_clear();
  lcd_print("***** ATENCAO! *****");
  lcd_set_cursor(0, 1);
  lcd_print("  Inicializado com");
  lcd_set_cursor(0, 2);
  lcd_print("  valores padrao !");
  lcd_set_cursor(0, 3);
  lcd_print(" Press. set p/ sair");
  waitingCommand = true;
}

/*******************************************************************************************************
 * Stage 2: Configuring temperature to control.
 *******************************************************************************************************/
void startConfigurationWizard() {
  lcd_clear();
  lcd_print("*** Configuracao ***");
  lcd_set_cursor(0, 1);
  lcd_print("Temperatura: ");
  dtostrf(settings.fermentationTemp, 4, 1, outstr);
  lcd_set_cursor(14, 1);
  lcd_print(outstr);
  lcd_put(223);
  lcd_print("C");
  lcd_set_cursor(0, 2);
  lcd_print(" + e - para ajustar");
  lcd_set_cursor(0, 3);
  lcd_print(" Set para continuar");
  tempAux = settings.fermentationTemp;
  pressedSince = 0;
  waitingCommand = true;
  delay(500);
  waiting = g_now;
}

/*******************************************************************************************************
 * Stage 3: Configuring temperature delta
 *******************************************************************************************************/
void configureMaxDelta() {
  lcd_clear();
  lcd_print("*** Configuracao ***");
  lcd_set_cursor(0, 1);
  lcd_print("Delta Max.: ");
  dtostrf(settings.tempDelta, 4, 1, outstr);
  lcd_set_cursor(14, 1);
  lcd_print(outstr);
  lcd_put(223);
  lcd_print("C");
  lcd_set_cursor(0, 2);
  lcd_print(" + e - para ajustar");
  lcd_set_cursor(0, 3);
  lcd_print(" Set para finalizar");
  tempAux = settings.tempDelta;
  pressedSince = 0;
  waitingCommand = true;
  delay(200);
  waiting = g_now;
}

/*******************************************************************************************************
 * Updating LCD with new value after pressing
 * up or down buttons
 *******************************************************************************************************/
void updateTempConfiguration() {
  dtostrf(tempAux, 4, 1, outstr);
  lcd_set_cursor(14, 1);
  lcd_print(outstr);
  lcd_put(223);
  lcd_print("C");
  delay(200);
  waiting = g_now;
}

/*******************************************************************************************************
 * Loading configuration data from EEPROM
 *******************************************************************************************************/
void loadConfig() {
  // To make sure there are settings, and they are YOURS!
  // If nothing is found it will use the default settings.
  if (//EEPROM.read(CONFIG_START + sizeof(settings) - 1) == settings.version_of_program[3] // this is '\0'
    EEPROM.read(CONFIG_START + sizeof(settings) - 2) == settings.version_of_program[2] &&
    EEPROM.read(CONFIG_START + sizeof(settings) - 3) == settings.version_of_program[1] &&
    EEPROM.read(CONFIG_START + sizeof(settings) - 4) == settings.version_of_program[0])
  { // reads settings from EEPROM
    for (unsigned int t = 0; t < sizeof(settings); t++)
      *((char*)&settings + t) = EEPROM.read(CONFIG_START + t);
  } else {
    // settings aren't valid! will overwrite with default settings
    saveConfig();
    stage = 1;
  }
}

/*******************************************************************************************************
 * Saving configuration data to EEPROM
 *******************************************************************************************************/
void saveConfig() {
  for (unsigned int t = 0; t < sizeof(settings); t++)
  { // writes to EEPROM
    EEPROM.write(CONFIG_START + t, *((char*)&settings + t));
    // and verifies the data
    if (EEPROM.read(CONFIG_START + t) != *((char*)&settings + t))
    {
      // error writing to EEPROM
    }
  }
}

/*******************************************************************************************************
 * Clear EEPROM, writing 0 into all bytes
 *******************************************************************************************************/
void resetEEPROM() {
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  digitalWrite(blueLed, HIGH);
  digitalWrite(redLed, HIGH);
  delay(500);
  digitalWrite(blueLed, LOW);
  digitalWrite(redLed, LOW);
}

/*******************************************************************************************************
 * Reading temperature from the sensor
 *******************************************************************************************************/
float getTemp() {

  byte data[12];
  byte addr[8];
  if ( !ds.search(addr)) {
    //no more sensors on chain, reset search
    ds.reset_search();
    return -1000;
  }
  if ( OneWire::crc8( addr, 7) != addr[7]) {
    return -1000;
  }
  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    return -1000;
  }
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);
  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);

  for (int i = 0; i < 9; i++) {
    data[i] = ds.read();
  }

  ds.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];
  float TRead = ((MSB << 8) | LSB);
  float Temperature = TRead / 16;

  return Temperature;

}


static int btn(int pin) {
  if (pin == setButton) return g_set;
  if (pin == upButton) return g_up;
  return g_down;
}

static void lcd_clear() {
  lcd.clear();
  for (uint8_t r = 0; r < 4; r++) {
    memset(shadow[r], ' ', 20);
    shadow[r][20] = 0;
  }
  cur_col = cur_row = 0;
}

static void lcd_set_cursor(uint8_t col, uint8_t row) {
  lcd.setCursor(col, row);
  cur_col = col;
  cur_row = row;
}

static void lcd_put(char c) {
  lcd.print(c);
  if (cur_col < 20 && cur_row < 4) shadow[cur_row][cur_col] = c;
  cur_col++;
}

static void lcd_print(const char *text) {
  for (uint8_t i = 0; text[i]; i++) lcd_put(text[i]);
}

void thermo_reset_state(void) {
  stage = 0;
  waitingCommand = false;
  pressedSince = 0;
  stateButton = LOW;
  waiting = 0;
  tempAux = 0;
  g_temp_fixed = false;
  StoreStruct defaults = {19.0, 0.5, 'C', 0, CONFIG_VERSION};
  settings = defaults;
}

void thermo_loop(void) {
  g_now = millis();
  g_set = digitalRead(setButton);
  g_up = digitalRead(upButton);
  g_down = digitalRead(downButton);
  g_temp_fixed = false;
  loop_pass();
}

void thermo_loop_with(unsigned long now_ms, int set, int up, int down, float temp_c) {
  g_now = now_ms;
  g_set = set;
  g_up = up;
  g_down = down;
  g_temp_fixed = true;
  g_temp = temp_c;
  loop_pass();
}

const char *thermo_lcd_line(uint8_t row) {
  return shadow[row % 4];
}
