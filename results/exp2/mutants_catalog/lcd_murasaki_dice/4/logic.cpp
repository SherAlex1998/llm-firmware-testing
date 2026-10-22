#include "logic.h"
#include <LiquidCrystal.h>

// Connections to the circuit: potentiometer
const int SELECTOR_PIN = A0;

// Connections to the circuit: movement detector
const int MOVE_DETECT_PIN = 6;

// Connections to the circuit: LCD screen
const int LCD_RS_PIN = 12;
const int LCD_ENABLE_PIN = 11;
const int LCD_DATA_PIN_0 = 5;
const int LCD_DATA_PIN_1 = 4;
const int LCD_DATA_PIN_2 = 3;
const int LCD_DATA_PIN_3 = 2;

static LiquidCrystal lcd(LCD_RS_PIN, LCD_ENABLE_PIN, LCD_DATA_PIN_0, LCD_DATA_PIN_1, LCD_DATA_PIN_2, LCD_DATA_PIN_3);

const int LCD_ROWS = 2;
const int LCD_COLS = 16;

const int N_DICES = 15;
const int dices[N_DICES] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 24, 20, 22, 24, 30, 100 };

int selectedDiceIndex = -1;
int selectedDiceType = 0;
int prevSwitchState = LOW;

unsigned long secondsSinceStart = 0;

// Murusaki digits logic is: the value is the number of angles
byte murasakiDigits[5][8] = {
  // zero:
  {
    B00000,
    B00001,
    B00010,
    B00100,
    B01000,
    B10000,
    B00000,
  },
  // one:
  {
    B00000,
    B00001,
    B00011,
    B00101,
    B01001,
    B10001,
    B00000,
  },
  // two:
  {
    B00000,
    B11111,
    B10001,
    B10001,
    B10001,
    B10001,
    B00000,
  },
  // three:
    {
    B00000,
    B00001,
    B00011,
    B00101,
    B01001,
    B11111,
    B00000,
  },
  // four
  {
    B00000,
    B11111,
    B10001,
    B10001,
    B10001,
    B11111,
    B00000,
  },
}
;

// Testability: scale of the roll animation delays, last roll and a shadow of the 16x2 LCD (it cannot be read back).
static uint8_t delay_scale = 100;
int lastRoll = 0;
static char shadow[2][17];
static uint8_t cur_col = 0, cur_row = 0;

static void paced_delay(unsigned long ms) {
  delay(ms * delay_scale / 100);
}

void jdr_set_delay_scale(uint8_t percent) {
  delay_scale = percent;
}

static void lcd_set_cursor(int col, int row) {
  lcd.setCursor(col, row);
  cur_col = col;
  cur_row = row;
}

// custom characters 0..4 (Murasaki digits) are kept as '0'..'4' in the shadow, the full block 0xFF as '#'
static void lcd_write(uint8_t c) {
  lcd.write(c);
  if (cur_col < 16 && cur_row < 2) shadow[cur_row][cur_col] = c < 5 ? '0' + c : (c == 0xFF ? '#' : (char)c);
  cur_col++;
}

static void lcd_print(const char *text) {
  for (uint8_t i = 0; text[i]; i++) {
    lcd.print(text[i]);
    if (cur_col < 16 && cur_row < 2) shadow[cur_row][cur_col] = text[i];
    cur_col++;
  }
}

static void lcd_print_num(int value) {
  char buf[8];
  itoa(value, buf, 10);
  lcd_print(buf);
}

void jdr_reset_state() {
  selectedDiceIndex = -1;
  selectedDiceType = 0;
  prevSwitchState = LOW;
  secondsSinceStart = 0;
  lastRoll = 0;
  for (uint8_t r = 0; r < 2; r++) {
    memset(shadow[r], ' ', 16);
    shadow[r][16] = 0;
  }
}

void jdr_loop() {
  selectDice();
  rollDice();
  updateMurasakiClock();
}

const char *jdr_lcd_line(uint8_t row) {
  return shadow[row % 2];
}

void jdr_init() {
  jdr_reset_state();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(SELECTOR_PIN, INPUT);
  pinMode(MOVE_DETECT_PIN, INPUT);
  lcd.begin(LCD_COLS, LCD_ROWS);


  // Create Murasaki numbers
  for (int i=0; i<5; i++) {
    lcd.createChar(i, murasakiDigits[i]);
  }
}

void printMurasakiDigit(int value) {
  lcd_write(byte(value));
}

// Pass -1 as littleDigitCol if you want to just print "here" without setCursor
void printMurasakiNumber(unsigned long value, int littleDigitCol, int row) {
  bool leadingZeros = true;
  unsigned long divider = 1220703125; // 5 to the 13
  for (int i=13; i>=0; i--) {
    int digit = value / divider;
    if (leadingZeros == true && (digit != 0 || i == 0)) {
      leadingZeros = false;
      if (littleDigitCol >= 0) {
        lcd_set_cursor(littleDigitCol - i, row);
      }
    }
    if (! leadingZeros) {
      printMurasakiDigit(digit);
    }
    value -= digit * divider;
    divider = divider / 5;
  }
}

void printDiceType(int col, int row) {
  lcd_set_cursor(col, row);
  // clear old displayed value
  lcd_print("       ");
  lcd_set_cursor(col, row);
  lcd_print("\xBB\xB2\xBA\xDB");
  lcd_print_num(selectedDiceType);
}

void selectDice() {
  selectDiceWith(analogRead(SELECTOR_PIN));
}

void selectDiceWith(int selectorRawValue) {
  // Dice selection
  int newSelection = map(selectorRawValue, 1000, 20, 0, N_DICES-1);
  if (selectedDiceIndex != newSelection) {
    // Switch on internal LED for debug
    digitalWrite(LED_BUILTIN, HIGH);
    
    selectedDiceIndex = newSelection;

    selectedDiceType = dices[selectedDiceIndex];

    printDiceType(0, 0);
    
    // Switch off internal LED for debug
    digitalWrite(LED_BUILTIN, LOW);
  }
}


void rollDice() {
  rollDiceWith(digitalRead(MOVE_DETECT_PIN), millis());
}

void rollDiceWith(int switchState, unsigned long now) {
  if (switchState != prevSwitchState) {
    prevSwitchState = switchState;
    if (switchState == LOW) {
      digitalWrite(LED_BUILTIN, HIGH);

      randomSeed(now);
      int value = random(1, selectedDiceType + 1);
      lastRoll = value;

      // animation
      lcd_set_cursor(0, 1);
      for (int i=0; i<LCD_COLS; i++) {
        lcd_write(0xFF);
        paced_delay(25);
      }
      lcd_set_cursor(0, 1);
      for (int i=0; i<LCD_COLS; i++) {
        lcd_write(' ');
        paced_delay(25);
      }

      // print actual result
      printDiceType(0, 1);
      lcd_print(": ");
      lcd_print_num(value);
      lcd_print(" ");
      printMurasakiNumber(value, -1, -1);
      paced_delay(500);

      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}

void updateMurasakiClock() {
  updateMurasakiClockAt(millis());
}

void updateMurasakiClockAt(unsigned long now) {
  unsigned long newSeconds = now / 1000;
  if (newSeconds != secondsSinceStart) {
    secondsSinceStart = newSeconds;
    printMurasakiNumber(secondsSinceStart, LCD_COLS-1, 0);
  }
}
