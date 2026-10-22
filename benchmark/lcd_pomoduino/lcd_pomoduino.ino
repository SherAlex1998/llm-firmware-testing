#include "src/logic.h"

void setup()
{
    pomo2_setup();
}

void loop()
{
    pomo2_loop_at(millis());
    delay(100);
}
