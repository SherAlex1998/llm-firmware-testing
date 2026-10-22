#include "logic.h"
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
static LiquidCrystal_I2C lcd(0x27, 16, 2);
char terrainUpper[TERRAIN_WIDTH + 1];
char terrainLower[TERRAIN_WIDTH + 1];
volatile bool buttonPushed = false;
// Testability: scale of the 100 ms blink delay and a shadow of the 16x2 LCD (it cannot be read back)
static uint8_t delay_scale = 100;
static char shadow[2][17];
static uint8_t cur_col = 0, cur_row = 0;

static void paced_delay(unsigned long ms) {
  delay(ms * delay_scale / 100);
}

void runner_set_delay_scale(uint8_t percent) {
  delay_scale = percent;
}

static void lcd_set_cursor(uint8_t col, uint8_t row) {
  lcd.setCursor(col, row);
  cur_col = col;
  cur_row = row;
}

static void lcd_put(char c) {
  if (cur_col < 16 && cur_row < 2) shadow[cur_row][cur_col] = c;
  cur_col++;
}

static void lcd_print(const char *text) {
  lcd.print(text);
  for (uint8_t i = 0; text[i]; i++) lcd_put(text[i]);
}

static void lcd_print(unsigned int value) {
  char buf[7];
  utoa(value, buf, 10);
  lcd_print(buf);
}

const char *runner_lcd_line(uint8_t row) {
  return shadow[row % 2];
}

void initializeGraphics() {
  static byte graphics[] = {
    // Run position 1
    0b00000,
    0b00000,
    0b00100,
    0b11110,
    0b11110,
    0b11111,
    0b01001,
    0b00000,
    // Run position 2
    0b00000,
    0b00000,
    0b00100,
    0b11110,
    0b11110,
    0b11111,
    0b01001,
    0b00000,
    // Jump
    0b00000,
    0b00000,
    0b00100,
    0b11110,
    0b11110,
    0b11111,
    0b01001,
    0b00000,
    // Jump lower
    0b00000,
    0b00000,
    0b00100,
    0b11110,
    0b11110,
    0b11111,
    0b01001,
    0b00000,
    // Ground
    0b01110,
    0b11111,
    0b10101,
    0b11111,
    0b10101,
    0b11111,
    0b10101,
    0b11111,
    // Ground right
    0b01110,
    0b11111,
    0b10101,
    0b11111,
    0b10101,
    0b11111,
    0b10101,
    0b11111,
    // Ground left
    0b01110,
    0b11111,
    0b10101,
    0b11111,
    0b10101,
    0b11111,
    0b10101,
    0b11111,
  };
  int i;
  // Skip using character 0, this allows lcd_print() to be used to
  // quickly draw multiple characters
  for (i = 0; i < 8; ++i) {
    lcd.createChar(i + 1, &graphics[i * 8]);
  }
  for (i = 0; i < TERRAIN_WIDTH; ++i) {
    terrainUpper[i] = SPRITE_TERRAIN_EMPTY;
    terrainLower[i] = SPRITE_TERRAIN_EMPTY;
  }
}

// Slide the terrain to the left in half-character increments
//
void advanceTerrain(char* terrain, byte newTerrain) {
  for (int i = 0; i < TERRAIN_WIDTH; ++i) {
    char current = terrain[i];
    char next = (i == TERRAIN_WIDTH - 1) ? newTerrain : terrain[i + 1];
    switch (current) {
      case SPRITE_TERRAIN_EMPTY:
        terrain[i] = (next == SPRITE_TERRAIN_SOLID) ? SPRITE_TERRAIN_SOLID_RIGHT : SPRITE_TERRAIN_EMPTY;
        break;
      case SPRITE_TERRAIN_SOLID:
        terrain[i] = (next == SPRITE_TERRAIN_EMPTY) ? SPRITE_TERRAIN_SOLID_LEFT : SPRITE_TERRAIN_SOLID;
        break;
      case SPRITE_TERRAIN_SOLID_RIGHT:
        terrain[i] = SPRITE_TERRAIN_SOLID;
        break;
      case SPRITE_TERRAIN_SOLID_LEFT:
        terrain[i] = SPRITE_TERRAIN_EMPTY;
        break;
    }
  }
}

bool drawCAR(byte position, char* terrainUpper, char* terrainLower, unsigned int score) {
  bool collide = false;
  char upperSave = terrainUpper[CAR_HORIZONTAL_POSITION];
  char lowerSave = terrainLower[CAR_HORIZONTAL_POSITION];
  byte upper, lower;
  switch (position) {
    case CAR_POSITION_OFF:
      upper = lower = SPRITE_TERRAIN_EMPTY;
      break;
    case CAR_POSITION_RUN_LOWER_1:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_RUN1;
      break;
    case CAR_POSITION_RUN_LOWER_2:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_RUN2;
      break;
    case CAR_POSITION_JUMP_1:
    case CAR_POSITION_JUMP_8:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_JUMP;
      break;
    case CAR_POSITION_JUMP_2:
    case CAR_POSITION_JUMP_7:
      upper = SPRITE_JUMP_UPPER;
      lower = SPRITE_JUMP_LOWER;
      break;
    case CAR_POSITION_JUMP_3:
    case CAR_POSITION_JUMP_4:
    case CAR_POSITION_JUMP_5:
    case CAR_POSITION_JUMP_6:
      upper = SPRITE_JUMP;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
    case CAR_POSITION_RUN_UPPER_1:
      upper = SPRITE_RUN1;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
    case CAR_POSITION_RUN_UPPER_2:
      upper = SPRITE_RUN2;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
  }
  if (upper != ' ') {
    terrainUpper[CAR_HORIZONTAL_POSITION] = upper;
    collide = (upperSave == SPRITE_TERRAIN_EMPTY) ? false : true;
  }
  if (lower != ' ') {
    terrainLower[CAR_HORIZONTAL_POSITION] = lower;
    collide |= (lowerSave == SPRITE_TERRAIN_EMPTY) ? false : true;
  }

  byte digits = (score > 9999) ? 5 : (score > 999) ? 4 : (score > 99) ? 3 : (score > 9) ? 2 : 1;

  // Draw the scene
  terrainUpper[TERRAIN_WIDTH] = '\0';
  terrainLower[TERRAIN_WIDTH] = '\0';
  char temp = terrainUpper[16 - digits];
  terrainUpper[16 - digits] = '\0';
  lcd_set_cursor(0, 0);
  lcd_print(terrainUpper);
  terrainUpper[16 - digits] = temp;
  lcd_set_cursor(0, 1);
  lcd_print(terrainLower);

  lcd_set_cursor(16 - digits, 0);
  lcd_print(score);

  terrainUpper[CAR_HORIZONTAL_POSITION] = upperSave;
  terrainLower[CAR_HORIZONTAL_POSITION] = lowerSave;
  return collide;
}

// Handle the button push as an interrupt
void buttonPush() {
  buttonPushed = true;
}

void runner_init() {
  runner_reset_state();
  pinMode(PIN_BUTTON, INPUT);
  digitalWrite(PIN_BUTTON, HIGH);
  pinMode(PIN_AUTOPLAY, OUTPUT);
  digitalWrite(PIN_AUTOPLAY, HIGH);

  // Digital pin 2 maps to interrupt 0
  attachInterrupt(0/*PIN_BUTTON*/, buttonPush, FALLING);

  initializeGraphics();

  lcd.init();
  lcd.backlight();
}

// former static locals of loop(), global for testability (runner_reset_state() restores them)
byte CARPos = CAR_POSITION_RUN_LOWER_1;
byte newTerrainType = TERRAIN_EMPTY;
byte newTerrainDuration = 1;
bool playing = false;
bool blink = false;
unsigned int distance = 0;

void runner_loop() {

  if (!playing) {
    drawCAR((blink) ? CAR_POSITION_OFF : CARPos, terrainUpper, terrainLower, distance >> 3);
    if (blink) {
      lcd_set_cursor(0, 0);
      lcd_print("Press Start");
    }
    paced_delay(100);
    blink = !blink;
    if (buttonPushed) {
      initializeGraphics();
      CARPos = CAR_POSITION_RUN_LOWER_1;
      playing = true;
      buttonPushed = false;
      distance = 0;
    }
    return;
  }

  // Shift the terrain to the left
  advanceTerrain(terrainLower, newTerrainType == TERRAIN_LOWER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);
  advanceTerrain(terrainUpper, newTerrainType == TERRAIN_UPPER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);

  // Make new terrain to enter on the right
  if (--newTerrainDuration == 0) {
    if (newTerrainType == TERRAIN_EMPTY) {
      newTerrainType = (random(3) == 0) ? TERRAIN_UPPER_BLOCK : TERRAIN_LOWER_BLOCK;
      newTerrainDuration = 10 + random(6);
    } else {
      newTerrainType = TERRAIN_EMPTY;
      newTerrainDuration = 10 + random(6);
    }
  }

  if (buttonPushed) {
    if (CARPos <= CAR_POSITION_RUN_LOWER_2) CARPos = CAR_POSITION_JUMP_1;
    buttonPushed = false;
  }

  if (drawCAR(CARPos, terrainUpper, terrainLower, distance >> 3)) {
    playing = false; // The CAR collided with something. Too bad.
    for (int i = 0; i <= 2; i++) {
    }
  } else {
    if (CARPos == CAR_POSITION_RUN_LOWER_2 || CARPos == CAR_POSITION_JUMP_8) {
      CARPos = CAR_POSITION_RUN_LOWER_1;
    } else if ((CARPos >= CAR_POSITION_JUMP_3 && CARPos <= CAR_POSITION_JUMP_5) && terrainLower[CAR_HORIZONTAL_POSITION] != SPRITE_TERRAIN_EMPTY) {
      CARPos = CAR_POSITION_RUN_UPPER_1;
    } else if (CARPos >= CAR_POSITION_RUN_UPPER_1 && terrainLower[CAR_HORIZONTAL_POSITION] == SPRITE_TERRAIN_EMPTY) {
      CARPos = CAR_POSITION_JUMP_5;
    } else if (CARPos == CAR_POSITION_RUN_UPPER_2) {
      CARPos = CAR_POSITION_RUN_UPPER_1;
    } else {
      ++CARPos;
    }
    ++distance;

    digitalWrite(PIN_AUTOPLAY, terrainLower[CAR_HORIZONTAL_POSITION + 2] == SPRITE_TERRAIN_EMPTY ? HIGH : LOW);
  }
}

void runner_reset_state() {
  CARPos = CAR_POSITION_RUN_LOWER_1;
  newTerrainType = TERRAIN_EMPTY;
  newTerrainDuration = 1;
  playing = false;
  blink = false;
  distance = 0;
  buttonPushed = false;
  for (uint8_t r = 0; r < 2; r++) {
    memset(shadow[r], ' ', 16);
    shadow[r][16] = 0;
  }
}
