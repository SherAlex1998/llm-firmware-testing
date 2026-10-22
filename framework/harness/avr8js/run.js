// Headless ATmega328P (Arduino Uno, 16 MHz) on avr8js, the open-source core of the Wokwi simulator.
//   node run.js firmware.hex [timeout_ms]
// stdout: everything the firmware sent to Serial (USART0).
// exit 0: "@RUN_END" line received; 42: simulated timeout; 2: bad arguments or hex file.
// stderr (last line): {"sim_s": <simulated seconds>}
"use strict";
const fs = require("fs");
const {
  CPU, avrInstruction, AVRTimer, timer0Config, timer1Config, timer2Config, AVRIOPort, PinState, portBConfig,
  portCConfig, portDConfig, AVRUSART, usart0Config, AVRADC, adcConfig, AVREEPROM, EEPROMMemoryBackend, AVRSPI, spiConfig,
} = require("avr8js");

const FREQ_HZ = 16e6;
const FLASH_BYTES = 0x8000;
const SRAM_BYTES = 2048;
const END_MARK = "@RUN_END";
const TIMEOUT_EXIT_CODE = 42;

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
    else if (type === 2) base = ((data[0] << 8) | data[1]) << 4;      // extended segment address
    else if (type === 4) base = ((data[0] << 8) | data[1]) << 16;     // extended linear address
    else if (type === 1) break;                                        // end of file
  }
  return flash;
}

function main() {
  const [hexPath, timeoutArg] = process.argv.slice(2);
  if (!hexPath || !fs.existsSync(hexPath)) {
    process.stderr.write(`usage: node run.js firmware.hex [timeout_ms]; not found: ${hexPath}\n`);
    return 2;
  }
  const timeoutMs = Number(timeoutArg || 15000);
  const cpu = new CPU(new Uint16Array(loadHex(fs.readFileSync(hexPath, "utf8")).buffer), SRAM_BYTES);
  [timer0Config, timer1Config, timer2Config].forEach((cfg) => new AVRTimer(cpu, cfg));
  for (const cfg of [portBConfig, portCConfig, portDConfig]) {
    // avr8js does not model the internal pull-up, so an input with its PORT bit set is forced HIGH here.
    // A floating input keeps its last level (LOW if never driven) instead of reading random values as in Wokwi.
    // Listeners run before PIN is updated, so cpu.data[PIN] still holds the previous levels.
    const port = new AVRIOPort(cpu, cfg);
    port.addListener(() => {
      const before = cpu.data[cfg.PIN];
      for (let i = 0; i < 8; i++) {
        const state = port.pinState(i);
        if (state === PinState.InputPullUp) port.setPin(i, true);
        else if (state === PinState.Input) port.setPin(i, (before & (1 << i)) !== 0);
      }
    });
  }
  new AVRADC(cpu, adcConfig);            // analogRead() hangs without it; unconnected inputs read 0
  // EEPROM writes wait for EEPE and SPI.transfer() for SPIF. EEPROM starts erased; MISO reads 0.
  new AVREEPROM(cpu, new EEPROMMemoryBackend(1024));
  new AVRSPI(cpu, spiConfig, FREQ_HZ);
  const usart = new AVRUSART(cpu, usart0Config, FREQ_HZ);
  let serial = "";
  usart.onByteTransmit = (byte) => { serial += String.fromCharCode(byte); };

  const limit = (timeoutMs / 1000) * FREQ_HZ;
  const done = () => { const i = serial.indexOf(END_MARK); return i >= 0 && serial.indexOf("\n", i) >= 0; };
  while (cpu.cycles < limit && !done()) {
    for (let i = 0; i < 1000; i++) {
      avrInstruction(cpu);
      cpu.tick();
    }
  }
  process.stdout.write(serial);
  process.stderr.write(JSON.stringify({ sim_s: cpu.cycles / FREQ_HZ }) + "\n");
  return done() ? 0 : TIMEOUT_EXIT_CODE;
}

process.exitCode = main();
