"""Write the experiment 2 section of benchmark/INDEX.md from the module.yaml files (after benchmark/metrics.py).

    python benchmark/exp2_index.py
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

import yaml

BENCH = Path(__file__).resolve().parent
INDEX = BENCH / "INDEX.md"
START, END = "<!-- exp2:start -->", "<!-- exp2:end -->"
OFFICIAL = {"LiquidCrystal", "EEPROM", "Wire", "SPI", "Servo", "Stepper", "SoftwareSerial"}

INTRO = """## Experiment 2: medium and complex modules

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
"""

REJECTED = [
    ("dotb/SQGarageDoorController", "Particle Photon firmware (Particle.function, WKP/DAC pins), not Arduino"),
    ("sdjmchattie/ArduinoSimonSays", "1385 B of globals in RAM plus a ~810 B temporary Melody object on the stack"),
    ("alirezapirverdi/Arduino-pid-climate-control", "LLM-generated (\"project template - customize the header\"), 2026"),
    ("Sallu-k/Ultrasonic-Water-Level-Sensor-and-Controller, Raaif-Yousuf/Syringe_Pump",
     "created in 2026, signs of LLM rewriting"),
    ("thealanfan/Battleship (copy of Dliang9/Arduino-Battleships)", "two boards exchange state over Serial, blocking UI"),
    ("warp007x/Elevator-Control-System-using-Arduino-UNO", "two boards over Serial"),
    ("joaocmd/Arduino-Says, EfcyLab/mySimonGame, staudt/Genius", "blocking or recursive game logic waiting for buttons"),
    ("a2gs/lcd16x2MenuArduino, nlamprian/AlarmClock",
     "blocking button loops (AlarmClock also needs OSEPP LCDKeypad, not in the Library Manager)"),
    ("AbdAlazezAhmed/Arduinoa-Lift-Elevator", "blocking loops until a sensor reading that never comes in simulation"),
    ("mads256h/ReactionGameArduino, marcusharpe/arduino-metronome", "Arduino Mega (pins 22-49)"),
    ("CoolCreasu/Traffic-Light", "not for the Uno: interrupt on pin 5, LEDs on 16-18"),
    ("nevin469/Arduino-Keypad-Door-Lock-Project", "uses all 20 Uno pins including 0/1 (Serial)"),
    ("mads256h/PongArduino", "drives pins 0-7 through DDRD (Serial)"),
    ("simonprickett/arduino-memory-game", "LEDs on pins 0/1, 1.6 KB array"),
    ("designedByKorir/arduino-incubator", "DS3231 and DHT11 (Wokwi has DS1307 and DHT22 only), 2 s watchdog"),
    ("JFClaeys/RainbowWithButton", "missing private headers of the author (alphas.h, RainbowDef.h)"),
    ("timkettering/DigitalTachometer, cr0ybot/fermistat, azogue/bobinadora-arduino",
     "libraries outside the Library Manager (SmoothAnalogInput, SwitecX25, CheapLCD, ClickEncoder)"),
    ("jcyfkimi/MorseduinoCollection", "loop() body is commented out, the firmware does nothing"),
    ("patrickmblack/OpenCV-Tic-Tac-Toe", "nine identical servo-angle functions driven by Serial commands"),
    ("infratag/irsniff, ashvnv/smart-vending-machine, boos1993/fermduino, serifpersia/pianolux-arduino",
     "logic built around Serial (output, commands, MIDI)"),
    ("NatanaelAntonioli/MorseCodeArduino, nochecksum/arduino-iambic-keyer", "no medium-tier feature"),
    ("CruisingGeek/HeadUnitVolumeKnob", "Arduino Pro Micro (ATmega32U4)"),
    ("rahools/ArduinoSnakeGame", "same task and device as max7219_snake"),
    ("silviobacci/Arduino_Line_Follower_and_Obstacle_Avoider", "Arduino M0 Pro (ARM SAMD21)"),
    ("ektiropoulos/PcRGB", "README says CC BY 2.0 while LICENSE says MIT; blocking animations"),
    ("galval28/arduino-alarm-clock", "Arduino 101 (CurieBLE)"),
]


def row(meta: dict) -> str:
    libs = [lib["name"] for lib in meta.get("libraries") or []]
    third = [n for n in libs if n not in OFFICIAL]
    devices = [d["part"].replace("wokwi-", "") for d in meta.get("devices") or []]
    m = meta.get("metrics") or {}
    repo = meta["source_url"].split("github.com/")[1].strip("/")
    return (f"| `{meta['id']}` | {meta['complexity']} | {m.get('nloc', '')} | {m.get('ccn_sum', '')} | "
            f"{len(meta.get('sources') or [])} | {', '.join(libs) or '-'} | {'yes' if third else 'no'} | "
            f"{', '.join(devices) or '-'} | {meta['backend']} | [{repo}]({meta['source_url']}) | "
            f"{meta['license']} | {meta.get('repo_created')} |")


def main() -> int:
    metas = []
    for p in sorted(BENCH.glob("*/module.yaml")):
        meta = yaml.safe_load(p.read_text(encoding="utf-8"))
        if meta.get("experiment") == 2:
            metas.append(meta)
    metas.sort(key=lambda m: (m["complexity"] != "medium", m["id"]))
    counts = {t: sum(m["complexity"] == t for m in metas) for t in ("medium", "complex")}
    on = {b: sum(m["backend"] == b for m in metas) for b in ("wokwi", "avr8js")}
    table = ["| id | tier | NLOC | CCN | files | libraries | third-party | Wokwi devices | backend | source | license | created |",
             "|---|---|---|---|---|---|---|---|---|---|---|---|"] + [row(m) for m in metas]
    rejected = ["| Repository | Reason |", "|---|---|"] + [f"| {r} | {why} |" for r, why in REJECTED]
    body = "\n".join([START, INTRO,
                      f"{len(metas)} modules: {counts['medium']} medium, {counts['complex']} complex; "
                      f"{on['wokwi']} run on Wokwi, {on['avr8js']} on avr8js.", "", *table, "",
                      "### Rejected candidates (experiment 2)", "", *rejected, END, ""])
    text = INDEX.read_text(encoding="utf-8")
    if START in text:
        text = re.sub(re.escape(START) + r".*?" + re.escape(END) + r"\n?", lambda _: body, text, flags=re.S)
    else:
        text = text.rstrip() + "\n\n" + body
    INDEX.write_text(text, encoding="utf-8")
    print(f"INDEX.md: {len(metas)} experiment 2 modules")
    return 0


if __name__ == "__main__":
    sys.exit(main())
