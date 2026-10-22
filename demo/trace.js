// Records what a test firmware does on the simulated Arduino Uno, for the test player (demo/build_demo.py).
//   node demo/trace.js firmware.hex linemap.json [timeout_ms] [options_json]
// The board model is the one of framework/harness/avr8js/run.js (same avr8js version, same pull-up emulation,
// same peripherals), so the firmware behaves exactly as in the experiment; the default time limit is the harness one.
// linemap.json: {"rows": [[byte_address, file_code, line], ...]} from the ELF line table, file_code 1 = test
// sketch, 2.. = module source files, 0 = anything else (core, test runner).
// options_json: {"servo": [pins driven by the Servo library; their pulses are recorded as widths],
//   "lcd": {"rs": pin, "e": pin, "d": [d4, d5, d6, d7]}: a 16x2 HD44780 in 4-bit mode on these pins is decoded from
//   the pin levels at each falling edge of E (nothing is attached in the experiment; this is what an LCD would show)}
// stdout: JSON {freq, cycles, timeout, truncated,
//   pins:   [[cycle, pin, value]]    value 0..255: 255 = driven HIGH, 0 = LOW or not driven, else the PWM duty
//   servo:  [[cycle, pin, width_us]] pulse width, recorded when it changes
//   lcd:    [[cycle, text, flags]]   32 character codes (row 0 then row 1), flags: 1 display on, 2 cursor, 4 blink;
//                                    cursor position = flags >> 8
//   glyphs: [[cycle, hex]]           CGRAM (custom characters 0-7, 8 rows each), recorded when it changes
//   lines:  [[cycle, file_code, line]]
//   serial: [[first_byte_cycle, newline_cycle, text]]}
// pins, servo, lcd and lines hold changes only; servo and LCD pins are not repeated in pins.
"use strict";
const fs = require("fs");
const path = require("path");
const {
  CPU, avrInstruction, AVRTimer, timer0Config, timer1Config, timer2Config, AVRIOPort, PinState, portBConfig,
  portCConfig, portDConfig, AVRUSART, usart0Config, AVRADC, adcConfig, AVREEPROM, EEPROMMemoryBackend, AVRSPI, spiConfig,
} = require(path.join(__dirname, "..", "framework", "harness", "avr8js", "node_modules", "avr8js"));

const FREQ_HZ = 16e6;
const FLASH_BYTES = 0x8000;
const SRAM_BYTES = 2048;
const END_MARK = "@RUN_END";
const MAX_EVENTS = 20000;           // per kind; a hanging firmware loops forever, the player needs only the start of it
const PORTS = [[portDConfig, 0], [portBConfig, 8], [portCConfig, 14]];   // AVR port -> first Arduino pin number
// analogWrite() pins of the Uno: [TCCRnA address, COMnx1 bit, OCRnx address (low byte)]
const PWM = { 6: [0x44, 0x80, 0x47], 5: [0x44, 0x20, 0x48], 9: [0x80, 0x80, 0x88], 10: [0x80, 0x20, 0x8a],
  11: [0xb0, 0x80, 0xb3], 3: [0xb0, 0x20, 0xb4] };

function loadHex(text) {
  const flash = new Uint8Array(FLASH_BYTES);
  let base = 0;
  for (const raw of text.split(/\r?\n/)) {
    const line = raw.trim();
    if (!line.startsWith(":")) continue;
    const count = parseInt(line.substr(1, 2), 16);
    const addr = parseInt(line.substr(3, 4), 16);
    const type = parseInt(line.substr(7, 2), 16);
    const data = [];
    for (let i = 0; i < count; i++) data.push(parseInt(line.substr(9 + i * 2, 2), 16));
    if (type === 0) data.forEach((b, i) => { flash[base + addr + i] = b; });
    else if (type === 2) base = ((data[0] << 8) | data[1]) << 4;
    else if (type === 4) base = ((data[0] << 8) | data[1]) << 16;
    else if (type === 1) break;
  }
  return flash;
}

// word address -> file_code * 65536 + line; every address up to the next row belongs to the row before it
function loadLineMap(rows) {
  const map = new Int32Array(FLASH_BYTES / 2);
  rows.sort((a, b) => a[0] - b[0]);
  for (let i = 0; i < rows.length; i++) {
    const [addr, file, line] = rows[i];
    const end = i + 1 < rows.length ? rows[i + 1][0] : addr + 2;
    for (let w = addr >> 1; w < end >> 1; w++) map[w] = file ? file * 65536 + line : 0;
  }
  return map;
}

// HD44780 controller as LiquidCrystal drives it: 8-bit mode after power-up, single nibbles until "function set 4-bit"
class LCD {
  constructor() {
    this.ddram = new Uint8Array(128).fill(0x20);
    this.glyphs = new Uint8Array(64);               // CGRAM: 8 custom characters x 8 rows, bit 4 = leftmost pixel
    this.addr = 0; this.inc = 1; this.eightBit = true; this.half = -1; this.cgram = false; this.cgAddr = 0;
    this.on = false; this.cursor = false; this.blink = false;
  }
  nibble(rs, value) {
    if (this.eightBit) return this.exec(rs, value << 4);
    if (this.half < 0) { this.half = value; return; }
    const byte = (this.half << 4) | value;
    this.half = -1;
    this.exec(rs, byte);
  }
  exec(rs, b) {
    if (rs && this.cgram) {
      this.glyphs[this.cgAddr] = b & 0x1f;
      this.cgAddr = (this.cgAddr + this.inc) & 0x3f;
    } else if (rs) {
      this.ddram[this.addr] = b;
      this.addr = (this.addr + this.inc) & 0x7f;
    } else if (b & 0x80) { this.addr = b & 0x7f; this.cgram = false; }
    else if (b & 0x40) { this.cgAddr = b & 0x3f; this.cgram = true; }
    else if (b & 0x20) { this.eightBit = (b & 0x10) !== 0; this.half = -1; }
    else if (b & 0x10) { /* cursor or display shift: LiquidCrystal scrolling, not used by the modules */ }
    else if (b & 0x08) { this.on = (b & 4) !== 0; this.cursor = (b & 2) !== 0; this.blink = (b & 1) !== 0; }
    else if (b & 0x04) { this.inc = b & 2 ? 1 : -1; }
    else if (b & 0x02) { this.addr = 0; }
    else if (b & 0x01) { this.ddram.fill(0x20); this.addr = 0; }
  }
  snapshot() {
    const text = String.fromCharCode(...this.ddram.subarray(0, 16), ...this.ddram.subarray(0x40, 0x50));
    const pos = this.addr >= 0x40 ? 16 + Math.min(this.addr - 0x40, 15) : Math.min(this.addr, 15);
    return [text, (this.on ? 1 : 0) | (this.cursor ? 2 : 0) | (this.blink ? 4 : 0) | (pos << 8)];
  }
}

function main() {
  const [hexPath, mapPath, timeoutArg, optionsArg] = process.argv.slice(2);
  if (!hexPath || !fs.existsSync(hexPath) || !mapPath || !fs.existsSync(mapPath)) {
    process.stderr.write("usage: node trace.js firmware.hex linemap.json [timeout_ms] [options_json]\n");
    return 2;
  }
  const options = JSON.parse(optionsArg || "{}");
  const servoPins = new Set(options.servo || []);
  const lcdPins = options.lcd ? new Set([options.lcd.rs, options.lcd.e, ...options.lcd.d]) : new Set();
  const lcd = options.lcd ? new LCD() : null;
  const lcdEvents = [], glyphEvents = [];
  let lcdLast = "", glyphLast = "0".repeat(128);
  const lineMap = loadLineMap(JSON.parse(fs.readFileSync(mapPath, "utf8")).rows);
  const cpu = new CPU(new Uint16Array(loadHex(fs.readFileSync(hexPath, "utf8")).buffer), SRAM_BYTES);
  [timer0Config, timer1Config, timer2Config].forEach((cfg) => new AVRTimer(cpu, cfg));

  let truncated = false;
  const push = (list, item) => { if (list.length < MAX_EVENTS) list.push(item); else truncated = true; };
  const pins = [], servo = [];
  const value = new Int16Array(20);            // last recorded value per pin
  const digital = new Uint8Array(20);          // pin driven HIGH right now
  const rise = new Float64Array(20), width = new Float64Array(20);
  const pwmValue = (pin) => {
    const p = PWM[pin];
    return p && (cpu.data[p[0]] & p[1]) ? cpu.data[p[2]] : -1;
  };
  const update = (pin) => {
    if (servoPins.has(pin) || lcdPins.has(pin)) return;
    const pwm = pwmValue(pin);
    const v = pwm >= 0 ? pwm : digital[pin] ? 255 : 0;
    if (v !== value[pin]) { value[pin] = v; push(pins, [cpu.cycles, pin, v]); }
  };
  for (const [cfg, first] of PORTS) {
    const port = new AVRIOPort(cpu, cfg);
    port.addListener(() => {
      // pull-up emulation, unchanged from run.js
      const before = cpu.data[cfg.PIN];
      for (let i = 0; i < 8; i++) {
        const state = port.pinState(i);
        if (state === PinState.InputPullUp) port.setPin(i, true);
        else if (state === PinState.Input) port.setPin(i, (before & (1 << i)) !== 0);
      }
      for (let i = 0; i < 8 && first + i < 20; i++) {
        const pin = first + i;
        const high = port.pinState(i) === PinState.High ? 1 : 0;
        if (high === digital[pin]) continue;
        digital[pin] = high;
        if (lcdPins.has(pin)) {
          // the controller latches RS and D4..D7 on the falling edge of E
          if (pin === options.lcd.e && !high) {
            const nib = options.lcd.d.reduce((acc, p, k) => acc | (digital[p] << k), 0);
            lcd.nibble(digital[options.lcd.rs], nib);
            const [text, flags] = lcd.snapshot();
            const key = text + flags;
            if (key !== lcdLast) { lcdLast = key; push(lcdEvents, [cpu.cycles, text, flags]); }
            const glyphs = Buffer.from(lcd.glyphs).toString("hex");
            if (glyphs !== glyphLast) { glyphLast = glyphs; push(glyphEvents, [cpu.cycles, glyphs]); }
          }
        } else if (servoPins.has(pin)) {
          if (high) rise[pin] = cpu.cycles;
          else {
            const us = Math.round(((cpu.cycles - rise[pin]) * 1e6) / FREQ_HZ);
            if (Math.abs(us - width[pin]) > 4) { width[pin] = us; push(servo, [cpu.cycles, pin, us]); }
          }
        } else update(pin);
      }
    });
  }
  // analogWrite() switches a compare output on (COMnx1) and sets OCRnx: re-evaluate the PWM pins after such writes
  let pwmDirty = false;
  for (const addr of new Set(Object.values(PWM).flatMap(([tccr, , ocr]) => [tccr, ocr]))) {
    const prev = cpu.writeHooks[addr];
    cpu.writeHooks[addr] = (v, old, a, mask) => { pwmDirty = true; return prev ? prev(v, old, a, mask) : false; };
  }
  new AVRADC(cpu, adcConfig);
  new AVREEPROM(cpu, new EEPROMMemoryBackend(1024));
  new AVRSPI(cpu, spiConfig, FREQ_HZ);
  const usart = new AVRUSART(cpu, usart0Config, FREQ_HZ);
  const serial = [];
  let pending = null;
  let finished = false;
  usart.onByteTransmit = (byte) => {
    const ch = String.fromCharCode(byte);
    if (!pending) pending = [cpu.cycles, 0, ""];
    if (ch === "\n") {
      pending[1] = cpu.cycles;
      pending[2] = pending[2].replace(/\r$/, "");
      serial.push(pending);
      if (pending[2].startsWith(END_MARK)) finished = true;
      pending = null;
    } else {
      pending[2] += ch;
    }
  };

  const lines = [];
  let last = -1;
  const limit = (Number(timeoutArg || 15000) / 1000) * FREQ_HZ;
  while (cpu.cycles < limit && !finished) {
    for (let i = 0; i < 1000; i++) {
      const code = lineMap[cpu.pc];
      if (code && code !== last) { last = code; push(lines, [cpu.cycles, code >> 16, code & 0xffff]); }
      avrInstruction(cpu);
      cpu.tick();
      if (pwmDirty) { pwmDirty = false; for (const pin of Object.keys(PWM)) update(Number(pin)); }
    }
  }
  if (pending) serial.push([pending[0], cpu.cycles, pending[2]]);
  process.stdout.write(JSON.stringify({ freq: FREQ_HZ, cycles: cpu.cycles, timeout: !finished, truncated, pins, servo,
    lcd: lcdEvents, glyphs: glyphEvents, lines, serial }));
  return 0;
}

process.exitCode = main();
