"""Wokwi circuits of the experiment 1 modules for the demo player, built from module.yaml pins and roles.

    python demo/wokwi_diagrams.py              # all experiment 1 modules -> demo/diagrams/<id>.json
    python demo/wokwi_diagrams.py led_chaser

Parts are chosen by role keywords: servo, potentiometer, joystick, push-button, sensor (slide switch),
ULN2003 (stepper motor); any other output is an LED. Wokwi has no DC motor driver, so its inputs are shown as LEDs.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "demo" / "diagrams"

# wokwi-arduino-uno pin coordinates (wokwi-elements arduino-uno-element.ts)
TOP = {13: 125, 12: 134.5, 11: 144, 10: 153.5, 9: 163, 8: 173, 7: 189, 6: 198.5, 5: 208, 4: 217.5, 3: 227,
       2: 236.5, 1: 246, 0: 255.5}
BOTTOM = {"5V": 160, "GND.2": 169.5, "GND.3": 179, "A0": 208, "A1": 217.5, "A2": 227, "A3": 236.5}
GND_TOP = 115.5
TOP_Y, BOTTOM_Y = 9, 191.5
SIG, VCC, GND = "green", "red", "black"


def uno_pin(pin: int) -> tuple[str, tuple[float, float]]:
    """Wokwi pin name and coordinates; 14, 15 ... are A0, A1 ... on the bottom header."""
    if pin >= 14:
        name = f"A{pin - 14}"
        return name, (BOTTOM[name], BOTTOM_Y)
    return str(pin), (TOP[pin], TOP_Y)


def route(start: tuple[float, float], *points: tuple[float, float]) -> list[str]:
    """Wokwi path instructions from `start` through `points` (orthogonal moves)."""
    out, (x, y) = [], start
    for px, py in points:
        if abs(px - x) > 0.01:
            out.append(f"h{px - x:g}")
        if abs(py - y) > 0.01:
            out.append(f"v{py - y:g}")
        x, y = px, py
    return out


def short_label(role: str, pin: int) -> str:
    text = re.sub(r"\((PWM[^)]*|Servo library|barrier)\)|\b(LED|L298N|L293D|ULN2003|channel|signal|motor)\b", "", role)
    return " ".join(text.split()) or f"D{pin}"


class Diagram:
    def __init__(self, module_id: str):
        self.parts = [{"type": "wokwi-arduino-uno", "id": "uno", "top": 0, "left": 0, "attrs": {}}]
        self.conns: list[list] = []
        self.module_id = module_id
        self._top = self._bottom = self._lane = 0
        self._right_y = -60.0

    # lanes: distinct y above/below the board and distinct x right of it, so parallel wires never overlap
    def top_lane(self) -> float:
        self._top += 1
        return -16 - 8 * self._top

    def bottom_lane(self) -> float:
        self._bottom += 1
        return BOTTOM_Y + 8 + 7 * self._bottom

    def right_lane(self) -> float:
        self._lane += 1
        return 282 + 8 * self._lane

    def add(self, type_: str, id_: str, left: float, top: float, attrs: dict | None = None, rotate: int = 0):
        part = {"type": type_, "id": id_, "top": round(top, 2), "left": round(left, 2), "attrs": attrs or {}}
        if rotate:
            part["rotate"] = rotate
        self.parts.append(part)

    def wire(self, a: str, b: str, color: str, path: list[str]):
        self.conns.append([a, b, color, path])

    def right_slot(self, height: float) -> float:
        top, self._right_y = self._right_y, self._right_y + height + 40
        return top

    def leds(self, items: list[tuple[int, str, str]]):
        """Row of LEDs above the board, ordered like their pins; anodes to pins, cathodes chained to GND."""
        items = sorted(items, key=lambda it: TOP[it[0]])
        center = sum(TOP[p] for p, _, _ in items) / len(items)
        lefts = [center - 23 * (len(items) - 1) + 46 * i - 20 for i in range(len(items))]
        top, pin_y, chain_y = -210, -168, -140
        left_going = [i for i, (p, _, _) in enumerate(items) if lefts[i] + 25 < TOP[p]]
        right_going = [i for i in range(len(items)) if i not in left_going][::-1]
        lanes = {i: self.top_lane() for i in left_going + right_going}      # outer wires turn closest to the board
        for i, (pin, color, label) in enumerate(items):
            led = f"led{pin}"
            self.add("wokwi-led", led, lefts[i], top, {"color": color, "label": label})
            px = TOP[pin]
            self.wire(f"uno:{pin}", f"{led}:A", SIG, route((px, TOP_Y), (px, lanes[i]), (lefts[i] + 25, lanes[i])))
            if i:
                prev = f"led{items[i - 1][0]}"
                self.wire(f"{prev}:C", f"{led}:C", GND, route((lefts[i - 1] + 15, pin_y), (lefts[i - 1] + 15, chain_y),
                                                              (lefts[i] + 15, chain_y)))
        first = lefts[0] + 15
        self.wire(f"led{items[0][0]}:C", "uno:GND.1", GND, route((first, pin_y), (first, -16), (GND_TOP, -16)))

    def to_right(self, pin_name: str, pin_xy: tuple[float, float], target: str, target_xy: tuple[float, float], color):
        """Wire from a board pin (top or bottom header) around to a part on the right."""
        lane_y = self.top_lane() if pin_xy[1] == TOP_Y else self.bottom_lane()
        x = self.right_lane()
        self.wire(f"uno:{pin_name}", target, color,
                  route(pin_xy, (pin_xy[0], lane_y), (x, lane_y), (x, target_xy[1]), target_xy))

    def servo(self, pin: int, n: int):
        left, top = 400, self.right_slot(120)
        sid = f"servo{n}"
        self.add("wokwi-servo", sid, left, top, {"horn": "double"})
        name, xy = uno_pin(pin)
        self.to_right(name, xy, f"{sid}:PWM", (left, top + 69), SIG)
        self.to_right("5V", (BOTTOM["5V"], BOTTOM_Y), f"{sid}:V+", (left, top + 59.5), VCC)
        self.to_right("GND.2", (BOTTOM["GND.2"], BOTTOM_Y), f"{sid}:GND", (left, top + 50), GND)

    def button(self, pin: int, pullup: bool, n: int):
        left, top = 400, self.right_slot(45)
        bid = f"btn{n}"
        self.add("wokwi-pushbutton", bid, left, top, {"color": "green", "label": f"D{pin}"})
        name, xy = uno_pin(pin)
        self.to_right(name, xy, f"{bid}:2.l", (left, top + 32), SIG)
        if pullup:                                   # INPUT_PULLUP: pressed = LOW
            self.to_right("GND.2", (BOTTOM["GND.2"], BOTTOM_Y), f"{bid}:1.l", (left, top + 13), GND)
            return
        self.to_right("5V", (BOTTOM["5V"], BOTTOM_Y), f"{bid}:1.l", (left, top + 13), VCC)
        rid = f"r{n}"                                 # external 10k pull-down: released = LOW
        self.add("wokwi-resistor", rid, left + 85, top + 32 - 5.65, {"value": "10000"})
        self.wire(f"{bid}:2.r", f"{rid}:1", SIG, [])
        lane = self.bottom_lane()
        end = (left + 85 + 58.8, top + 32)
        self.wire(f"{rid}:2", "uno:GND.3", GND, route(end, (end[0] + 12, end[1]), (end[0] + 12, lane),
                                                      (BOTTOM["GND.3"], lane)))

    def stepper(self, pins: dict[str, int]):
        """28BYJ-48 through ULN2003 -> Wokwi bipolar stepper wired directly. Coils: {IN4, IN2} and {IN3, IN1}."""
        coil = {"A-": pins["IN4"], "A+": pins["IN2"], "B+": pins["IN3"], "B-": pins["IN1"]}
        xs = {"A-": 66.76, "A+": 76.36, "B+": 85.96, "B-": 95.56}      # size 17
        left = TOP[pins["IN4"]] - xs["A-"] + 24        # shifted right: the one unavoidable crossing stays readable
        pin_y = -70
        self.add("wokwi-stepper-motor", "stepper", left, pin_y - 178.8, {"size": "17", "display": "steps",
                                                                          "arrow": "orange"})
        for name, pin in sorted(coil.items(), key=lambda kv: -TOP[kv[1]]):   # outer wires turn closest to the board
            px, tx = TOP[pin], left + xs[name]
            lane = self.top_lane()
            self.wire(f"uno:{pin}", f"stepper:{name}", SIG, route((px, TOP_Y), (px, lane), (tx, lane)))

    def bottom_parts(self, items: list[tuple[str, int, str]]):
        """Inputs below the board, rotated 180 degrees so their pins face the analog header."""
        n_wires = 3 * len(items)
        top = BOTTOM_Y + 30 + 7 * (n_wires + self._bottom)
        left = 120
        for kind, pin, pid in items:
            if kind == "pot":
                w, h, pins = 75.6, 75.6, {"VCC": 49, "SIG": 39, "GND": 29}
                sig, ptype, attrs, pin_y = "SIG", "wokwi-potentiometer", {"value": "512"}, 68.5
            elif kind == "joystick":
                w, h, pins = 102.8, 120.2, {"VCC": 33, "HORZ": 52.2, "GND": 71.4}
                sig, ptype, attrs, pin_y = "HORZ", "wokwi-analog-joystick", {}, 115.8
            else:                                       # slide switch: handle at 1-2 = 5V (no object), 2-3 = GND
                w, h, pins = 32.1, 34.9, {"1": 6.5, "2": 16, "3": 25.5}
                sig, ptype, attrs, pin_y = "2", "wokwi-slide-switch", {}, 34
            self.add(ptype, pid, left, top, attrs, rotate=180)
            at = {k: (left + w - x, top + h - pin_y) for k, x in pins.items()}   # pin positions after rotation
            name, xy = uno_pin(pin)
            ends = {sig: (name, SIG), ("VCC" if kind != "switch" else "1"): ("5V", VCC),
                    ("GND" if kind != "switch" else "3"): ("GND.3", GND)}
            for part_pin, (board_pin, color) in ends.items():
                bx = xy[0] if board_pin == name else BOTTOM[board_pin]
                lane = self.bottom_lane()
                self.wire(f"uno:{board_pin}", f"{pid}:{part_pin}", color,
                          route((bx, BOTTOM_Y), (bx, lane), (at[part_pin][0], lane), at[part_pin]))
            left += w + 40

    def json(self) -> str:
        return json.dumps({"version": 1, "author": f"benchmark/{self.module_id} (generated by wokwi_diagrams.py)",
                           "editor": "wokwi", "parts": self.parts, "connections": self.conns, "dependencies": {}},
                          indent=2)


def led_color(role: str) -> str:
    low = role.lower()
    for c in ("red", "yellow", "green"):
        if c in low:
            return c
    if re.search(r"\b(in\d|input|enable|en)\b", low):
        return "white" if re.search(r"enable|\ben\b", low) else "blue"
    return "red"


def build(module_dir: Path) -> Diagram:
    meta = yaml.safe_load((module_dir / "module.yaml").read_text(encoding="utf-8"))
    d = Diagram(meta["id"])
    leds, bottom, stepper = [], [], {}
    for p in meta["pins"]:
        pin, role, mode = p["pin"], p.get("role", ""), p["mode"]
        low = role.lower()
        if "uln2003" in low:
            stepper[role.split()[-1]] = pin
        elif "servo" in low:
            d.servo(pin, sum(1 for part in d.parts if part["type"] == "wokwi-servo") + 1)
        elif "push-button" in low:
            d.button(pin, mode == "INPUT_PULLUP", sum(1 for part in d.parts if part["type"] == "wokwi-pushbutton") + 1)
        elif "potentiometer" in low:
            bottom.append(("pot", pin, "pot1"))
        elif "joystick" in low:
            bottom.append(("joystick", pin, "joystick1"))
        elif "sensor" in low:
            bottom.append(("switch", pin, f"sensor{len(bottom) + 1}"))
        elif mode == "OUTPUT":
            leds.append((pin, led_color(role), short_label(role, pin)))
    if stepper:
        d.stepper(stepper)
    if leds:
        d.leds(leds)
    if bottom:
        d.bottom_parts(bottom)
    return d


def export(module_dir: Path, out_root: Path = OUT) -> Path:
    out = out_root / f"{module_dir.name}.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(build(module_dir).json() + "\n", encoding="utf-8")
    return out


def main(ids: list[str]) -> int:
    dirs = [ROOT / "benchmark" / i for i in ids] if ids else sorted(
        p.parent for p in (ROOT / "benchmark").glob("*/module.yaml")
        if yaml.safe_load(p.read_text(encoding="utf-8")).get("experiment") != 2)
    for d in dirs:
        print(f"{d.name:22s} -> {export(d).relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
