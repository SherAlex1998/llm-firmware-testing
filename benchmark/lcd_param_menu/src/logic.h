// Parameter menu on a 20x4 I2C LCD driven by a rotary encoder, with EEPROM persistence.
// Source: https://github.com/cotestatnt/menu_lcd (menu_lcd2.ino), MIT License, Copyright (c) 2018 cotestatnt.
// Refactored for testability (see module.yaml notes).
#pragma once
#include <Arduino.h>

#define MENU_ROWS 4
#define MENU_COLS 20
#define ENC_CLK_PIN 3
#define ENC_DT_PIN 2
#define ENC_SW_PIN 4
#define MENU_NUM_VARS 8
#define MENU_LONGCLICK_MS 1000

// Menu variables, in this order: 0 Enable, 1 Ready, 2 Time 1, 3 Time 2, 4 Setpoint, 5 kp, 6 ki, 7 kd
enum MenuVarType { MENU_BOOL = 0, MENU_UINT = 1, MENU_FLOAT = 3 };

void menu_init();                          // pins, LCD, encoder interrupts; variables and menu state back to power-up values
void menu_check_button(uint16_t enter_pause_ms, uint16_t save_pause_ms);  // reads ENC_SW_PIN (LOW = pressed) like the original
void menu_button_event(uint16_t held_ms, uint16_t enter_pause_ms, uint16_t save_pause_ms);  // press lasting held_ms: long (> MENU_LONGCLICK_MS) or single click
void menu_long_click(uint16_t enter_pause_ms, uint16_t save_pause_ms);    // toggles setup mode; leaving it saves to EEPROM (original pauses: 1000 / 2000 ms)
void menu_single_click();                  // setup mode: select the next variable (wraps around)
void menu_edit_vars();                     // applies the encoder position to the selected variable, then zeroes the encoder
void menu_apply_delta(int16_t delta);      // same with an explicit encoder delta: uint += delta, float += delta * 0.01, bool toggles
void menu_render();                        // setup mode: draws the page of the selected variable if the selection or a value changed
void menu_idle_screen();                   // normal mode screen
void menu_write_eeprom();                  // all variables, packed in menu order from address 0
void menu_read_eeprom();
void menu_encoder_isr();                   // interrupt handler of both encoder pins

bool menu_setup_mode();
uint8_t menu_selected();
MenuVarType menu_var_type(uint8_t index);
float menu_value(uint8_t index);           // value of a menu variable; bool -> 0 or 1
const char* menu_lcd_line(uint8_t row);    // text shown on an LCD row: MENU_COLS characters, empty cells are spaces
