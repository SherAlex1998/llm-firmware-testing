#include "logic.h"

#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RotaryEncoder.h>

static LiquidCrystal_I2C lcd(0x27, MENU_COLS, MENU_ROWS);
static RotaryEncoder encoder(ENC_CLK_PIN, ENC_DT_PIN);

static bool Enable;
static bool Ready;
static uint16_t Time1;
static uint16_t Time2;
static float Setpoint;
static float Kp;
static float Ki;
static float Kd;

// Addresses of all menu variables: the pages are built by iterating this array.
// The array is uint16_t*, so bool and float variables are reached through casts.
static uint16_t* VarPtrs[MENU_NUM_VARS] = {(uint16_t*)&Enable, (uint16_t*)&Ready, &Time1, &Time2,
                                           (uint16_t*)&Setpoint, (uint16_t*)&Kp, (uint16_t*)&Ki, (uint16_t*)&Kd};
static const MenuVarType VarType[MENU_NUM_VARS] = {MENU_BOOL, MENU_BOOL, MENU_UINT, MENU_UINT,
                                                   MENU_FLOAT, MENU_FLOAT, MENU_FLOAT, MENU_FLOAT};

// Labels live in flash (PROGMEM) to save RAM.
static const char L0[] PROGMEM = "Enable:     ";
static const char L1[] PROGMEM = "Ready:      ";
static const char L2[] PROGMEM = "Time 1:     ";
static const char L3[] PROGMEM = "Time 2:     ";
static const char L4[] PROGMEM = "Setpoint:   ";
static const char L5[] PROGMEM = "Constant kp:";
static const char L6[] PROGMEM = "Constant ki:";
static const char L7[] PROGMEM = "Constant kd:";
static const char* const VarLabels[MENU_NUM_VARS] PROGMEM = {L0, L1, L2, L3, L4, L5, L6, L7};

static int varSel = 0, oldVarSel = -1;
static char lcdBuffer[MENU_COLS + 1];
static bool SetupMode = false;
static char shadow[MENU_ROWS][MENU_COLS + 1];   // what the LCD shows (the display cannot be read back)

static void shadow_clear() {
  for (uint8_t r = 0; r < MENU_ROWS; r++) {
    memset(shadow[r], ' ', MENU_COLS);
    shadow[r][MENU_COLS] = '\0';
  }
}

static void lcd_clear() {
  lcd.clear();
  shadow_clear();
}

static void lcd_print_at(uint8_t col, uint8_t row, const char* text) {
  lcd.setCursor(col, row);
  lcd.print(text);
  for (uint8_t i = 0; text[i] != '\0' || col + i < MENU_COLS; i++) {
    shadow[row][col + i] = text[i];
  }
}

static void lcd_print_at_P(uint8_t col, uint8_t row, const __FlashStringHelper* text) {
  char buf[MENU_COLS + 1];
  strncpy_P(buf, (const char*)text, MENU_COLS);
  buf[MENU_COLS] = '\0';
  lcd_print_at(col, row, buf);
}

void menu_init() {
  Enable = true;
  Ready = true;
  Time1 = 2500;
  Time2 = 5000;
  Setpoint = 150.0F;
  Kp = 4.50F;
  Ki = 0.10F;
  Kd = 0.80F;
  varSel = 0;
  oldVarSel = -1;
  SetupMode = false;

  pinMode(ENC_SW_PIN, INPUT_PULLUP);
  pinMode(ENC_DT_PIN, INPUT_PULLUP);
  pinMode(ENC_CLK_PIN, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd_clear();
  encoder.setPosition(0);
  // The encoder is on pins 2 and 3, so both can use external interrupts.
  attachInterrupt(digitalPinToInterrupt(ENC_DT_PIN), menu_encoder_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_CLK_PIN), menu_encoder_isr, CHANGE);
}

void menu_encoder_isr() {
  encoder.tick();
}

// Short or long press of the encoder button.
void menu_check_button(uint16_t enter_pause_ms, uint16_t save_pause_ms) {
  uint32_t pressTime = millis();
  if (digitalRead(ENC_SW_PIN) == LOW) {
    delay(50);
    bool xLong = false;
    while (digitalRead(ENC_SW_PIN) == LOW) {
      if (millis() - pressTime > MENU_LONGCLICK_MS) {
        xLong = true;
        break;
      }
    }
    xLong ? menu_long_click(enter_pause_ms, save_pause_ms) : menu_single_click();
  }
}

void menu_button_event(uint16_t held_ms, uint16_t enter_pause_ms, uint16_t save_pause_ms) {
  if (held_ms > MENU_LONGCLICK_MS) {
    menu_long_click(enter_pause_ms, save_pause_ms);
  } else {
    menu_single_click();
  }
}

void menu_long_click(uint16_t enter_pause_ms, uint16_t save_pause_ms) {
  SetupMode = !SetupMode;
  if (SetupMode) {
    oldVarSel = -1;
    lcd_clear();
    lcd_print_at_P(5, 1, F("MENU SETUP"));
    delay(enter_pause_ms);
  } else {
    lcd_clear();
    lcd.noBlink();
    lcd_print_at_P(2, 1, F("SALVO PARAMETRI"));
    delay(save_pause_ms);
    // End of programming: store the new values
    menu_write_eeprom();
    lcd_clear();
  }
}

void menu_single_click() {
  if (SetupMode) {
    varSel = (varSel + 1) % MENU_NUM_VARS;
  }
}

// Shows the user-editable variables of the selected variable's page.
void menu_render() {
  if (varSel != oldVarSel) {
    oldVarSel = varSel;
    lcd_clear();
    uint8_t page = varSel / MENU_ROWS;
    for (uint8_t row = 0; row < MENU_ROWS; row++) {
      uint8_t varNum = page * MENU_ROWS + row;
      char label_buf[MENU_COLS];
      strcpy_P(label_buf, (char*)pgm_read_word(&(VarLabels[varNum])));
      switch (VarType[varNum]) {
        case MENU_UINT:
          sprintf(lcdBuffer, "%s  %05u", label_buf, *VarPtrs[varNum]);
          break;
        case MENU_BOOL: {
          bool* b_ptr = (bool*)VarPtrs[varNum];
          sprintf(lcdBuffer, "%s  %s", label_buf, *b_ptr ? "true" : "false");
          break;
        }
        case MENU_FLOAT: {
          float* f_ptr = (float*)VarPtrs[varNum];
          sprintf(lcdBuffer, "%s  %02u.%02u", label_buf, (uint16_t)*f_ptr, (uint16_t)(*f_ptr * 100) % 100);
          break;
        }
      }
      lcd_print_at(0, row, lcdBuffer);
    }
    lcd.setCursor(0, varSel % MENU_ROWS);
    lcd.blink();
  }
}

void menu_idle_screen() {
  lcd.noBlink();
  lcd_print_at_P(5, 1, F("TEST MENU"));
}

void menu_edit_vars() {
  int16_t encPos = (int16_t)encoder.getPosition();
  if (encPos != 0) {
    menu_apply_delta(encPos);
    encoder.setPosition(0);
  }
}

void menu_apply_delta(int16_t delta) {
  if (delta == 0) {
    return;
  }
  // varSel decides which parameter changes
  switch (VarType[varSel]) {
    case MENU_UINT:
      *VarPtrs[varSel] = *VarPtrs[varSel] + (uint16_t)delta;
      break;
    case MENU_BOOL: {
      bool* b_ptr = (bool*)VarPtrs[varSel];
      *b_ptr = !*b_ptr;
      break;
    }
    case MENU_FLOAT: {
      float* f_ptr = (float*)VarPtrs[varSel];
      *f_ptr = *f_ptr + (float)delta * 0.01;
      break;
    }
  }
  oldVarSel = -1;   // force an LCD update
}

void menu_read_eeprom() {
  uint8_t eeAddress = 0;
  for (uint8_t i = 0; i < MENU_NUM_VARS; i++) {
    switch (VarType[i]) {
      case MENU_UINT:
        EEPROM.get(eeAddress, *VarPtrs[i]);
        eeAddress += sizeof(uint16_t);
        break;
      case MENU_BOOL:
        EEPROM.get(eeAddress, *(bool*)VarPtrs[i]);
        eeAddress += sizeof(bool);
        break;
      case MENU_FLOAT:
        EEPROM.get(eeAddress, *(float*)VarPtrs[i]);
        eeAddress += sizeof(float);
        break;
    }
  }
}

void menu_write_eeprom() {
  uint8_t eeAddress = 0;
  for (uint8_t i = 0; i < MENU_NUM_VARS; i++) {
    switch (VarType[i]) {
      case MENU_UINT:
        EEPROM.put(eeAddress, *VarPtrs[i]);
        eeAddress += sizeof(uint16_t);
        break;
      case MENU_BOOL:
        EEPROM.put(eeAddress, *(bool*)VarPtrs[i]);
        eeAddress += sizeof(bool);
        break;
      case MENU_FLOAT:
        EEPROM.put(eeAddress, *(float*)VarPtrs[i]);
        eeAddress += sizeof(float);
        break;
    }
  }
}

bool menu_setup_mode() {
  return SetupMode;
}

uint8_t menu_selected() {
  return varSel;
}

MenuVarType menu_var_type(uint8_t index) {
  return VarType[index % MENU_NUM_VARS];
}

float menu_value(uint8_t index) {
  index %= MENU_NUM_VARS;
  switch (VarType[index]) {
    case MENU_BOOL:
      return *(bool*)VarPtrs[index] ? 1.0F : 0.0F;
    case MENU_UINT:
      return *VarPtrs[index];
    default:
      return *(float*)VarPtrs[index];
  }
}

const char* menu_lcd_line(uint8_t row) {
  return shadow[row % MENU_ROWS];
}
