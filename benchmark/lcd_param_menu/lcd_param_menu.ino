#include "src/logic.h"

void setup() {
  menu_init();
  // menu_read_eeprom();  // the original leaves this commented out until values have been saved once
}

void loop() {
  menu_check_button(1000, 2000);
  if (menu_setup_mode()) {
    menu_edit_vars();
    menu_render();
  } else {
    menu_idle_screen();
    delay(100);
  }
}
