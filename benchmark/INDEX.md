# Benchmark

Open-source Arduino Uno projects adapted for unit testing. Every module compiles with `arduino-cli` for
`arduino:avr:uno`.

```
benchmark/<id>/
  <id>.ino        # setup()/loop() only, includes src/logic.h
  src/logic.h     # functions under test, pin numbers, state enums
  src/logic.cpp   # implementation
  module.yaml     # source, license, pins, functions under test, adaptation notes
```

The original file and what was changed are in the `notes` field of each `module.yaml`; the source and license are
also repeated in the header of `logic.h`. `python benchmark/compile_all.py` compiles every module.

## Experiment 1: simple modules

15 modules: 8 LED, 5 motor, 2 LED + motor.

| # | id | category | complexity | LOC | functions | source | license |
|---|---|---|---|---|---|---|---|
| 1 | `traffic_light` | led | simple | 79 | 7 | [sspeev/Arduino-Traffic-Light](https://github.com/sspeev/Arduino-Traffic-Light) | MIT |
| 2 | `led_chaser` | led | simple | 66 | 7 | [Projectslearner/arduino-uno-led-chaser](https://github.com/Projectslearner/arduino-uno-led-chaser) | MIT |
| 3 | `led_breathing` | led | simple | 52 | 7 | [italianproject423/Fade-In-Fade-Out](https://github.com/italianproject423/Fade-In-Fade-Out) | MIT |
| 4 | `two_color_crossfade` | led | simple | 63 | 7 | [Projectslearner/Arduino-uno-2-color-LED-module](https://github.com/Projectslearner/Arduino-uno-2-color-LED-module) | MIT |
| 5 | `button_toggle_led` | led | simple | 40 | 6 | [harsh-hw-dev/Embedded-Programming-Series](https://github.com/harsh-hw-dev/Embedded-Programming-Series) | MIT |
| 6 | `button_led_sequence` | led | simple | 61 | 8 | [rajeshThakare/Arduino-UNO-for-Absolute-Beginners](https://github.com/rajeshThakare/Arduino-UNO-for-Absolute-Beginners) | MIT |
| 7 | `debounce_toggle` | led | simple | 64 | 8 | [arduino/arduino-examples](https://github.com/arduino/arduino-examples) | CC0-1.0 |
| 8 | `traffic_intersection` | led | medium | 75 | 7 | [rajeshThakare/Arduino-UNO-for-Absolute-Beginners](https://github.com/rajeshThakare/Arduino-UNO-for-Absolute-Beginners) | MIT |
| 9 | `dc_motor_pot` | motor | simple | 71 | 8 | [AndreaLombardo/Arduino-L298N-Motore-DC](https://github.com/AndreaLombardo/Arduino-L298N-Motore-DC) | MIT |
| 10 | `dual_hbridge` | motor | simple | 98 | 7 | [Projectslearner/arduino-uno-h-bridge-module](https://github.com/Projectslearner/arduino-uno-h-bridge-module) | MIT |
| 11 | `joystick_motor` | motor | simple | 78 | 10 | [vchervanev/arduino-joystick-to-motor](https://github.com/vchervanev/arduino-joystick-to-motor) | MIT |
| 12 | `servo_sweep` | motor | simple | 64 | 7 | [gwappa/arduino-servo-sweep](https://github.com/gwappa/arduino-servo-sweep) | MIT |
| 13 | `stepper_28byj48` | motor | medium | 125 | 9 | [Moragor/Mora_28BYJ_48](https://github.com/Moragor/Mora_28BYJ_48) | MIT |
| 14 | `railway_gate` | led+motor | medium | 111 | 11 | [Venkatesh4707/Railway-Gate-Automation-Arduino](https://github.com/Venkatesh4707/Railway-Gate-Automation-Arduino) | MIT |
| 15 | `traffic_servo_gate` | led+motor | simple | 89 | 8 | [Rifat1003/Simple-Traffic-control-system-using-LED-and-servo-motor.](https://github.com/Rifat1003/Simple-Traffic-control-system-using-LED-and-servo-motor./tree/main) | MIT |

LOC counts non-empty, non-comment lines of `logic.h` + `logic.cpp`.

Adaptation rules:

- All logic lives in `src/logic.h` and `src/logic.cpp`; `<id>.ino` only calls it from `setup()` and `loop()`.
- No function loops forever. Long `delay()` calls moved to the `.ino`; functions that still wait take the duration or
  count as an argument.
- Pin access is kept as in the original. State that `digitalRead` cannot show (PWM level, direction, counters, state
  machine phase) has getters.
- Functions that read inputs have pure twins with explicit arguments (`toggle_update(bool)`,
  `db_update(reading, now_ms)`, `gate_update(s1, s2)`), so a test can feed inputs without pressing buttons.
- `Serial.print` calls were removed, Serial carries the test protocol.
- Bugs found in the originals were fixed and noted (out-of-bounds index in `led_chaser`, repeated `pinMode` in
  `button_led_sequence`, `map(..., 1024, ...)` in `dc_motor_pot`).

### Rejected candidates (experiment 1)

| Repository | License | Reason |
|---|---|---|
| vicenteraphael/SmartTrafficLight | MIT | 433 LOC, library |
| ajesler/Arduino-RGBEffects | MIT | 437 LOC, library |
| danmirror/traffic-light-arduino | MIT | 268-331 LOC per sketch |
| Cimanes/Stepper-menu | MIT | 304 LOC, interactive Serial menu |
| 10gabrielc/DeskFan-PCFan-Conversion | MIT | 357 LOC |
| MrElectrouino/arduino_74hc595_RGB_LEDs | MIT | 953 LOC |
| teemango/LED-controller-5-channels | MIT | DS1307 RTC over Wire |
| upiir/arduino_led_buttons | MIT | SSD1306 OLED / U8g2 |
| Agassias/pid-controller-emulated-feedback | MIT | I2C LCD |
| gsunea/28BYJ-48-Stepper-motor-control | MIT | 262 LOC, duplicates `stepper_28byj48` |
| nemanja994/Stepper-28BYJ-48 | MIT | 291 LOC, library, duplicate |
| Jatin24creator/Stepper-Motor-Optocoupler-ArduinoUNO | MIT | interrupts and Stepper.h, untestable structure |
| elxecutor/traffic-light | MIT | direct AVR registers instead of the Arduino API |
| OladapoAjala/Drive | MIT | L298 library, duplicates `dual_hbridge` |
| merna-saleh-ibrahim/DC-Motor-Speed-and-Direction-Control-L293D | MIT | duplicates `dc_motor_pot` |
| harsh-hw-dev/... MOTOR_SPEED_CONTROLLER | MIT | 23 LOC, duplicates `dc_motor_pot` |
| MohammedBosaid/Arduino-4-servos-sweep-hold | MIT | duplicates `servo_sweep` |
| mrohimilhadi/ArduinoUno-Blink-LED_RGB | Apache-2.0 | 27 LOC, duplicates `traffic_light` |
| Tharindu714/Arduino-Lighing-System-main | MIT | 292 LOC, relays |
| arduino-examples: Blink, Fade, Button, BlinkWithoutDelay | CC0-1.0 | too trivial or duplicates |

<!-- exp2:start -->
## Experiment 2: medium and complex modules

Selection: GitHub, MIT / Apache-2.0 / BSD / CC0 / Unlicense, repository created before 2023 (`washer_safety` is newer and
was checked by hand to be human-written), Arduino Uno, every device available in Wokwi or not needed. Tiers come from
`benchmark/metrics.py` (lizard): medium 130-300 NLOC, complex 300-850 NLOC, each with at least one feature such as a
third-party library, a bus device, interrupts or EEPROM. Third-party means any library except the core and the official
Arduino ones (LiquidCrystal, EEPROM, Wire, SPI, Servo, Stepper, SoftwareSerial). Every module has a hand-written
`tests/smoke_test.ino` that the model never sees.

Adaptation on top of the experiment 1 rules: several source files are allowed and `logic.h` is the only header the
tests include; pure twins take time and inputs as arguments (`xx_loop_with(now, ...)`, interrupt handler twins); shadow
buffers for displays that cannot be read back; a delay scale (`xx_set_delay_scale`) for long blocking animations; the
init function restores the power-on state. Changes and bugs found in the originals are listed in the `notes` of each
`module.yaml`.

19 modules: 10 medium, 9 complex; 9 run on Wokwi, 10 on avr8js.

| id | tier | NLOC | CCN | files | libraries | third-party | Wokwi devices | backend | source | license | created |
|---|---|---|---|---|---|---|---|---|---|---|---|
| `lcd_endless_runner` | medium | 295 | 71 | 2 | LiquidCrystal I2C | yes | lcd1602, pushbutton | wokwi | [jaswch/Arduino_LCD_16x2_Endless_Runner_Game](https://github.com/jaswch/Arduino_LCD_16x2_Endless_Runner_Game) | MIT | 2022-12-27 |
| `lcd_kitchen_timer` | medium | 268 | 41 | 2 | LiquidCrystal | no | - | avr8js | [SlimTim10/kitchen-timer](https://github.com/SlimTim10/kitchen-timer) | MIT | 2016-12-20 |
| `lcd_morse_writer` | medium | 176 | 29 | 2 | LiquidCrystal I2C | yes | lcd1602, pushbutton | wokwi | [tesar-tech/arduino_and_morse_code](https://github.com/tesar-tech/arduino_and_morse_code) | MIT | 2019-01-11 |
| `lcd_murasaki_dice` | medium | 243 | 40 | 2 | LiquidCrystal | no | - | avr8js | [rchampeimont/arduino_JDR_Murasaki](https://github.com/rchampeimont/arduino_JDR_Murasaki) | MIT | 2021-05-02 |
| `lcd_param_menu` | medium | 264 | 53 | 2 | LiquidCrystal I2C, RotaryEncoder | yes | lcd2004, ky-040 | wokwi | [cotestatnt/menu_lcd](https://github.com/cotestatnt/menu_lcd) | MIT | 2018-09-12 |
| `max7219_scoreboard` | medium | 165 | 35 | 2 | MD_MAX72XX | yes | - | avr8js | [TrevisanGMW/arduino-scoreboard](https://github.com/TrevisanGMW/arduino-scoreboard) | MIT | 2019-02-23 |
| `neopixel_pomodoro` | medium | 203 | 41 | 2 | Adafruit NeoPixel | yes | - | avr8js | [atbrask/NeoPomodorino](https://github.com/atbrask/NeoPomodorino) | BSD-2-Clause | 2017-07-15 |
| `pedestrian_traffic_light` | medium | 162 | 22 | 2 | - | no | - | avr8js | [digitalkarl/Arduino-Traffic-Light-Project](https://github.com/digitalkarl/Arduino-Traffic-Light-Project) | CC0-1.0 | 2015-08-12 |
| `pet_feeding_timer` | medium | 183 | 30 | 2 | LiquidCrystal | no | - | avr8js | [hlgirard/PetFeedingTimer](https://github.com/hlgirard/PetFeedingTimer) | MIT | 2018-11-14 |
| `ultrasonic_parking` | medium | 195 | 51 | 5 | NewPing, Adafruit NeoPixel, Button | yes | hc-sr04, neopixel, pushbutton | wokwi | [acheaito/arduino-garage-parking](https://github.com/acheaito/arduino-garage-parking) | MIT | 2016-10-19 |
| `ds18b20_thermostat` | complex | 387 | 74 | 2 | LiquidCrystal I2C, OneWire | yes | ds18b20, lcd2004, pushbutton | wokwi | [IoBeer/thermostat-arduino](https://github.com/IoBeer/thermostat-arduino) | Apache-2.0 | 2016-01-25 |
| `ir_wakeup_light` | complex | 676 | 97 | 2 | IRremote, LiquidCrystal I2C | yes | ds1307, lcd1602, ir-receiver | wokwi | [erhanalankus/Arduino-Wake-Up-Light](https://github.com/erhanalankus/Arduino-Wake-Up-Light) | MIT | 2016-12-16 |
| `lcd_pomoduino` | complex | 782 | 119 | 22 | LiquidCrystal | no | - | avr8js | [opokatech/pomoduino2](https://github.com/opokatech/pomoduino2) | MIT | 2016-04-08 |
| `led_cube_8x8x8` | complex | 531 | 145 | 2 | - | no | - | avr8js | [itsharryle/LED_CUBE](https://github.com/itsharryle/LED_CUBE) | MIT | 2017-08-04 |
| `max7219_snake` | complex | 338 | 75 | 4 | - | no | max7219-matrix, analog-joystick | wokwi | [nazarii-piontko/arduino-snake](https://github.com/nazarii-piontko/arduino-snake) | MIT | 2020-04-15 |
| `max7219_tetris` | complex | 335 | 94 | 2 | LedControl | yes | max7219-matrix, analog-joystick | wokwi | [vahidzee/aurdoino-tetris](https://github.com/vahidzee/aurdoino-tetris) | MIT | 2018-12-29 |
| `pcd8544_2048` | complex | 836 | 205 | 13 | - | no | - | avr8js | [zhaofengli/2048-arduino](https://github.com/zhaofengli/2048-arduino) | BSD-2-Clause | 2014-06-01 |
| `pomodoro_rtc` | complex | 767 | 168 | 14 | Rtc by Makuna, Grove 4-Digit Display, TimerOne | yes | ds1307, tm1637-7segment, pushbutton, buzzer | wokwi | [2CoderOK/pomodoro](https://github.com/2CoderOK/pomodoro) | MIT | 2022-12-12 |
| `washer_safety` | complex | 420 | 131 | 2 | LiquidCrystal | no | - | avr8js | [Lee-geonmu/arduino-washer-safety-system](https://github.com/Lee-geonmu/arduino-washer-safety-system) | MIT | 2026-07-12 |

### Rejected candidates (experiment 2)

| Repository | Reason |
|---|---|
| dotb/SQGarageDoorController | Particle Photon firmware (Particle.function, WKP/DAC pins), not Arduino |
| sdjmchattie/ArduinoSimonSays | 1385 B of globals in RAM plus a ~810 B temporary Melody object on the stack |
| alirezapirverdi/Arduino-pid-climate-control | LLM-generated ("project template - customize the header"), 2026 |
| Sallu-k/Ultrasonic-Water-Level-Sensor-and-Controller, Raaif-Yousuf/Syringe_Pump | created in 2026, signs of LLM rewriting |
| thealanfan/Battleship (copy of Dliang9/Arduino-Battleships) | two boards exchange state over Serial, blocking UI |
| warp007x/Elevator-Control-System-using-Arduino-UNO | two boards over Serial |
| joaocmd/Arduino-Says, EfcyLab/mySimonGame, staudt/Genius | blocking or recursive game logic waiting for buttons |
| a2gs/lcd16x2MenuArduino, nlamprian/AlarmClock | blocking button loops (AlarmClock also needs OSEPP LCDKeypad, not in the Library Manager) |
| AbdAlazezAhmed/Arduinoa-Lift-Elevator | blocking loops until a sensor reading that never comes in simulation |
| mads256h/ReactionGameArduino, marcusharpe/arduino-metronome | Arduino Mega (pins 22-49) |
| CoolCreasu/Traffic-Light | not for the Uno: interrupt on pin 5, LEDs on 16-18 |
| nevin469/Arduino-Keypad-Door-Lock-Project | uses all 20 Uno pins including 0/1 (Serial) |
| mads256h/PongArduino | drives pins 0-7 through DDRD (Serial) |
| simonprickett/arduino-memory-game | LEDs on pins 0/1, 1.6 KB array |
| designedByKorir/arduino-incubator | DS3231 and DHT11 (Wokwi has DS1307 and DHT22 only), 2 s watchdog |
| JFClaeys/RainbowWithButton | missing private headers of the author (alphas.h, RainbowDef.h) |
| timkettering/DigitalTachometer, cr0ybot/fermistat, azogue/bobinadora-arduino | libraries outside the Library Manager (SmoothAnalogInput, SwitecX25, CheapLCD, ClickEncoder) |
| jcyfkimi/MorseduinoCollection | loop() body is commented out, the firmware does nothing |
| patrickmblack/OpenCV-Tic-Tac-Toe | nine identical servo-angle functions driven by Serial commands |
| infratag/irsniff, ashvnv/smart-vending-machine, boos1993/fermduino, serifpersia/pianolux-arduino | logic built around Serial (output, commands, MIDI) |
| NatanaelAntonioli/MorseCodeArduino, nochecksum/arduino-iambic-keyer | no medium-tier feature |
| CruisingGeek/HeadUnitVolumeKnob | Arduino Pro Micro (ATmega32U4) |
| rahools/ArduinoSnakeGame | same task and device as max7219_snake |
| silviobacci/Arduino_Line_Follower_and_Obstacle_Avoider | Arduino M0 Pro (ARM SAMD21) |
| ektiropoulos/PcRGB | README says CC BY 2.0 while LICENSE says MIT; blocking animations |
| galval28/arduino-alarm-clock | Arduino 101 (CurieBLE) |
<!-- exp2:end -->
