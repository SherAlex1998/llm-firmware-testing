// Test player logic; demo/build_demo.py inlines this file into test_player.html together with the run data.
"use strict";
const INDEX = JSON.parse(document.getElementById("player-index").textContent);
const SVGNS = "http://www.w3.org/2000/svg";
const FREQ = 16e6;
const SPEEDS = [2, 4, 8, 16, 32, 128];
const OPS = {
  const_change: "changed constant", flip_cmp: "flipped comparison", off_by_one: "off-by-one boundary",
  negate_cond: "negated condition", drop_call: "removed call", swap_hilo: "HIGH and LOW swapped",
  swap_arith: "arithmetic operator swapped", swap_logic: "logical operator swapped",
};
const LED_COLORS = { red: "#e53935", yellow: "#f2b400", green: "#2e9d43", blue: "#1e6fd9", white: "#9a9a94",
  orange: "#f07c00", purple: "#8e44ad" };
const HALF_STEPS = [8, 12, 4, 6, 2, 3, 1, 9];   // coil patterns (bit0 = IN1) of the 8 half-steps of a 4-coil stepper
const STEPPER_DEG = 360 / 64;                   // 28BYJ-48 rotor: 64 half-steps per turn (before the 1:64 gearbox)
const $ = (id) => document.getElementById(id);
const esc = (s) => String(s).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
const pinName = (p) => (p >= 14 ? "A" + (p - 14) : "D" + p);
const shortModel = (label) => label.replace(/^Claude /, "");
// Servo library default: 544 us = 0 deg, 2400 us = 180 deg
const servoAngle = (us) => (us ? Math.max(0, Math.min(180, ((us - 544) / (2400 - 544)) * 180)) : 0);

const S = { mod: null, run: null, blob: null, fw: null, tr: null, steps: [], cur: 0, playing: false, speed: 8, acc: 0,
  last: 0, timer: 0, parts: {}, pinParts: {}, uno: null, stepper: null, serial: [], tl: null, firstStep: [] };

/* ---------- run data: one JSON block per run, parsed when the run is opened ---------- */
const blobs = new Map();
function loadBlob(i) {
  if (!blobs.has(i)) blobs.set(i, JSON.parse(document.getElementById("run-" + i).textContent));
  return blobs.get(i);
}
function decode(t) {
  if (t.decoded) return t.decoded;
  const unflat = (flat, width) => {
    const out = [];
    let c = 0;
    for (let i = 0; i < flat.length; i += width) { c += flat[i]; out.push([c, ...flat.slice(i + 1, i + width)]); }
    return out;
  };
  // lines: [cycle, 1 = test | 2 = module, line, module file index from 1]
  t.decoded = {
    cycles: t.c, timeout: t.to, truncated: t.tr, pins: unflat(t.p, 3), servo: unflat(t.v, 3),
    lcd: unflat(t.d || [], 3), glyphs: unflat(t.g || [], 2), serial: t.s,
    lines: unflat(t.l, 2).map(([c, v]) => (v > 0 ? [c, 1, v, 0] : [c, 2, -v % 10000, Math.floor(-v / 10000)])),
  };
  return t.decoded;
}

/* ---------- syntax highlight (small C++ subset) ---------- */
const TOKEN = /(\/\/.*$)|("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)')|(\b(?:TEST|ASSERT_[A-Z_]+|RUN_ALL_TESTS)\b)|(#\s*\w+)|(\b(?:void|static|const|unsigned|signed|int|long|char|bool|for|if|else|return|while|do|switch|case|break|default|true|false)\b)|(\b(?:u?int(?:8|16|32)_t|HIGH|LOW|OUTPUT|INPUT|INPUT_PULLUP|Serial)\b)|(\b(?:0x[0-9a-fA-F]+|\d+)\b)/g;
function highlight(line) {
  let out = "", at = 0, m;
  TOKEN.lastIndex = 0;
  while ((m = TOKEN.exec(line))) {
    out += esc(line.slice(at, m.index));
    const cls = m[1] ? "c" : m[2] ? "s" : m[3] ? "m" : m[4] ? "p" : m[5] ? "k" : m[6] ? "t" : "n";
    out += `<span class="tok-${cls}">${esc(m[0])}</span>`;
    at = m.index + m[0].length;
  }
  return out + esc(line.slice(at));
}
function renderCode(el, source) {
  el.innerHTML = source.replace(/\s+$/, "").split(/\r?\n/).map((l, i) =>
    `<div class="ln" data-n="${i + 1}"><span class="no">${i + 1}</span><span class="src">${highlight(l) || " "}</span></div>`).join("");
  return [null, ...el.children];                          // index = line number
}
function scrollIntoPanel(panel, row) {
  if (!row) return;
  const top = row.offsetTop - panel.offsetTop, h = panel.clientHeight;
  if (top < panel.scrollTop + h * 0.15 || top > panel.scrollTop + h * 0.75) panel.scrollTop = Math.max(0, top - h * 0.35);
}

/* ---------- board ---------- */
const unoPin = (name) => (/^A\d$/.test(name) ? 14 + Number(name.slice(1)) : /^\d+$/.test(name) ? Number(name) : null);
async function buildBoard(mod) {
  const wrap = $("boardWrap");
  const stage = document.createElement("div");
  stage.className = "stage";
  const parts = {};
  for (const p of mod.diagram.parts) {
    const el = document.createElement(p.type);
    for (const [k, v] of Object.entries(p.attrs || {})) el.setAttribute(k, v);
    el.classList.add("part");
    el.style.left = p.left + "px"; el.style.top = p.top + "px";
    parts[p.id] = { el, p };
    stage.append(el);
  }
  const wires = document.createElementNS(SVGNS, "svg");
  wires.classList.add("wires");
  stage.append(wires);
  const note = document.createElement("div");
  note.className = "board-note";
  wrap.replaceChildren(stage, note);
  await Promise.all([...new Set(mod.diagram.parts.map((p) => p.type))].map((t) => customElements.whenDefined(t)));
  await Promise.all(Object.values(parts).map(({ el }) => el.updateComplete));

  const pinXY = (ref) => {
    const [id, pin] = ref.split(":");
    const part = parts[id];
    const info = part && (part.el.pinInfo || []).find((i) => i.name === pin);
    return info ? { x: part.p.left + info.x, y: part.p.top + info.y } : null;
  };
  let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
  const grow = (x, y) => { minX = Math.min(minX, x); minY = Math.min(minY, y); maxX = Math.max(maxX, x); maxY = Math.max(maxY, y); };
  for (const { el, p } of Object.values(parts)) { grow(p.left, p.top); grow(p.left + el.offsetWidth, p.top + el.offsetHeight); }

  const paths = [];
  const pinParts = {};
  for (const [from, to, color, hints] of mod.diagram.connections || []) {
    const a = pinXY(from), b = pinXY(to);
    if (!a || !b) continue;
    let pt = { ...a }, lastDir = "";
    const pts = [pt];
    for (const h of hints || []) {
      const m = /^([vh])(-?\d+(?:\.\d+)?)$/.exec(h);
      if (!m) continue;
      pt = m[1] === "v" ? { x: pt.x, y: pt.y + Number(m[2]) } : { x: pt.x + Number(m[2]), y: pt.y };
      pts.push(pt); lastDir = m[1];
    }
    pts.push(lastDir === "h" ? { x: pt.x, y: b.y } : { x: b.x, y: pt.y }, b);
    pts.forEach((q) => grow(q.x, q.y));
    paths.push({ pts, color });
    // which Arduino pin goes to which part pin
    for (const [x, y] of [[from, to], [to, from]]) {
      const [xid, xpin] = x.split(":"), [yid, ypin] = y.split(":");
      if (parts[xid]?.p.type !== "wokwi-arduino-uno" || !parts[yid] || parts[yid].p.type === "wokwi-arduino-uno") continue;
      const n = unoPin(xpin);
      if (n !== null) (pinParts[n] = pinParts[n] || []).push({ part: parts[yid], pin: ypin });
    }
  }
  const pad = 24, ox = minX - pad, oy = minY - pad, W = maxX - minX + 2 * pad, H = maxY - minY + 2 * pad;
  stage.style.width = W + "px"; stage.style.height = H + "px";
  for (const { el, p } of Object.values(parts)) { el.style.left = p.left - ox + "px"; el.style.top = p.top - oy + "px"; }
  wires.setAttribute("width", W); wires.setAttribute("height", H);
  wires.setAttribute("viewBox", `${ox} ${oy} ${W} ${H}`);
  wires.innerHTML = paths.map(({ pts, color }) =>
    `<polyline points="${pts.map((q) => q.x + "," + q.y).join(" ")}" fill="none" stroke="${esc(color)}" stroke-width="2.4" stroke-linejoin="round" stroke-linecap="round" opacity=".9"/>`).join("");
  S.parts = parts; S.pinParts = pinParts; S.stage = { el: stage, W, H }; S.note = note;
  S.uno = Object.values(parts).find(({ p }) => p.type === "wokwi-arduino-uno")?.el || null;
  S.stepper = Object.values(parts).find(({ p }) => p.type === "wokwi-stepper-motor")?.el || null;
  S.lcd = Object.values(parts).find(({ p }) => p.type === "wokwi-lcd1602")?.el || null;
  S.lcdFont = S.lcd ? S.lcd.font : null;
  S.lcdGlyphs = "";
  if (S.uno) S.uno.ledPower = true;
  fitBoard();
}
// the panel keeps resizing after the board is built, so the board scale follows it
function fitBoard() {
  if (!S.stage) return;
  const wrap = $("boardWrap"), { el, W, H } = S.stage;
  const noteH = S.note && S.note.textContent ? S.note.offsetHeight + 10 : 0;
  const ww = wrap.clientWidth - 16, wh = wrap.clientHeight - noteH - 16;
  if (ww <= 0 || wh <= 0) return;
  const k = Math.min(ww / W, wh / H, 2.4);
  el.style.transform = `translate(${8 + (ww - W * k) / 2}px, ${8 + (wh - H * k) / 2}px) scale(${k})`;
}
const boardResize = new ResizeObserver(() => fitBoard());
boardResize.observe($("boardWrap"));

/* ---------- steps: one per visible event (test line, output change, servo pulse change, serial line) ---------- */
function testIndex(line) { return S.run.tests.findIndex((t) => line >= t.from && line <= t.to); }
function parseSerial(text) {
  let m = /^@TEST (\S+) PASS/.exec(text);
  if (m) return { test: m[1], ok: true };
  m = /^@TEST (\S+) FAIL (.*)$/.exec(text);
  if (m) { const l = /^line (\d+): (.*)$/.exec(m[2]); return { test: m[1], ok: false, msg: m[2], line: l ? Number(l[1]) : 0, why: l ? l[2] : m[2] }; }
  m = /^@RUN_END (\d+)\/(\d+)/.exec(text);
  if (m) return { end: true, passed: Number(m[1]), total: Number(m[2]) };
  return {};
}
// bug = {file: module file index, line} of the injected change, or null for the original firmware
function buildSteps(tr, bug, coils) {
  const set = new Set([0]);
  for (const [c, f] of tr.lines) if (f === 1) set.add(c);
  for (const [c] of tr.pins) set.add(c);
  for (const [c] of tr.servo) set.add(c);
  for (const s of tr.serial) set.add(s[1]);
  // an LCD is written character by character: one step per burst of writes, at its last write
  const others = [...set].sort((a, b) => a - b);
  for (let i = 0; i < tr.lcd.length; i++) {
    const c = tr.lcd[i][0], next = i + 1 < tr.lcd.length ? tr.lcd[i + 1][0] : Infinity;
    let lo = 0, hi = others.length;
    while (lo < hi) { const mid = (lo + hi) >> 1; if (others[mid] <= c) lo = mid + 1; else hi = mid; }
    if (next === Infinity || (lo < others.length && others[lo] < next)) set.add(c);
  }
  const cycles = [...set].sort((a, b) => a - b);
  const lv = new Uint8Array(20), sv = new Uint16Array(20);
  let pi = 0, vi = 0, li = 0, si = 0, di = 0, gi = 0, testLine = 0, modLine = 0, modFile = 0, lastFile = 0, phase = -1, half = 0;
  const out = [];
  for (const c of cycles) {
    let hit = false;
    while (pi < tr.pins.length && tr.pins[pi][0] <= c) { lv[tr.pins[pi][1]] = tr.pins[pi][2]; pi++; }
    while (vi < tr.servo.length && tr.servo[vi][0] <= c) { sv[tr.servo[vi][1]] = tr.servo[vi][2]; vi++; }
    while (li < tr.lines.length && tr.lines[li][0] <= c) {
      const [, f, l, file] = tr.lines[li++];
      lastFile = f;
      if (f === 1) testLine = l;
      else { modLine = l; modFile = file; if (bug && l === bug.line && file === bug.file) hit = true; }
    }
    while (si < tr.serial.length && tr.serial[si][1] <= c) si++;
    while (di < tr.lcd.length && tr.lcd[di][0] <= c) di++;
    while (gi < tr.glyphs.length && tr.glyphs[gi][0] <= c) gi++;
    if (coils.length === 4) {
      const code = coils.reduce((acc, pin, i) => acc | ((lv[pin] ? 1 : 0) << i), 0);
      const k = HALF_STEPS.indexOf(code);
      if (k >= 0) {
        if (phase >= 0) { let d = (k - phase + 8) % 8; if (d > 4) d -= 8; if (Math.abs(d) <= 2) half += d; }
        phase = k;
      }
    }
    out.push({ c, lv: lv.slice(), sv: sv.slice(), testLine, modLine: lastFile === 2 ? modLine : 0, modFile, bug: hit,
      serialDone: si, lcd: di - 1, glyph: gi - 1, testIdx: testIndex(testLine), angle: half * STEPPER_DEG });
  }
  return out;
}

/* ---------- module / run / firmware selection ---------- */
async function selectModule(mi, opts = {}) {
  pause();
  S.mod = INDEX.modules[mi];
  const mod = S.mod;
  $("module").value = String(mi);
  $("moduleCount").textContent = `${mi + 1} / ${INDEX.modules.length}`;
  $("runs").innerHTML = mod.runs.map((r, i) => {
    const st = !r.built ? "test did not compile"
      : r.valid ? `test valid &middot; caught ${r.killed}/${r.n_mutants}`
      : `test failed on correct code (${r.passed}/${r.total})`;
    const cls = !r.built ? "nobuild" : r.valid ? "valid" : "invalid";
    return `<button class="run ${cls}" data-i="${i}"${r.built ? "" : ' disabled title="The generated test did not compile after the repair attempts; there is nothing to run."'}>` +
      `<span><b>${esc(shortModel(r.model_label))}</b> <span class="r">RAG ${esc(r.rag)}</span></span><span class="st"><i></i>${st}</span></button>`;
  }).join("");
  $("fileSelect").innerHTML = mod.files.map((f, i) => `<option value="${i + 1}">${esc(f.name)}</option>`).join("");
  $("fileSelect").style.display = mod.files.length > 2 ? "" : "none";
  await buildBoard(mod);
  const startRun = mod.runs.findIndex((r) => r.built && r.model === INDEX.start.model && r.rag === INDEX.start.rag);
  const ri = opts.run ?? (startRun >= 0 ? startRun : mod.runs.findIndex((r) => r.built));
  selectRun(ri, opts.fw ?? 0, opts.step ?? 0);
}
// module code panel: shows one file of the module and follows the execution into other files
function showModuleFile(fileIdx) {
  const mod = S.mod, fw = S.fw;
  const file = mod.files[fileIdx - 1];
  if (!file) return;
  S.shownFile = fileIdx;
  const mutated = fw.id && fw.file === file.name;
  S.moduleRows = renderCode($("moduleCode"), mutated ? fw.source : file.source);
  if (mutated) S.moduleRows[fw.line]?.classList.add("mut");
  $("moduleFile").textContent = file.name;
  $("fileSelect").value = String(fileIdx);
  $("moduleHint").textContent = mutated ? `module under test, with bug #${fw.id} on line ${fw.line}`
    : mod.files.length > 2 ? "module under test (follows the execution)" : "module under test";
}
function selectRun(ri, fwId = 0, step = 0) {
  pause();
  const mod = S.mod;
  S.run = mod.runs[ri];
  S.blob = loadBlob(S.run.blob);
  const run = S.run;
  [...$("runs").children].forEach((b, i) => b.setAttribute("aria-pressed", i === ri));
  $("runinfo").innerHTML = `<b>${esc(mod.name)}</b><br><code>${esc(run.path)}</code>`;
  $("boardHint").textContent = `${mod.id} · avr8js, the board model of the experiment`;
  $("testHint").textContent = `generated by ${run.model_label}, RAG ${run.rag} · ${run.tests.length} tests`;
  const fws = [{ id: 0 }, ...mod.mutants.filter((m) => S.blob.m[m.id])];
  $("chips").innerHTML = fws.map((f) => {
    if (!f.id) return `<button class="chip" data-fw="0" title="Original module, no bug"><span class="dot"></span><b>Original</b></button>`;
    const r = S.blob.m[f.id];
    const title = `${OPS[f.operator] || f.operator}, line ${f.line}: ${r.k ? (r.vd === "timeout" ? "caught (firmware hung)" : "caught") : "missed"}`;
    return `<button class="chip ${r.k ? "caught" : "missed"}" data-fw="${f.id}" title="${esc(title)}"><span class="dot"></span>` +
      `<b>#${f.id}</b><span class="op">${esc(f.operator)}</span></button>`;
  }).join("") + (run.valid ? "" : `<span class="hint" style="color:var(--muted);font-size:.85rem;align-self:center">no mutants: the test already fails on the original module</span>`);
  S.testRows = renderCode($("testCode"), run.test_source);
  $("tests").innerHTML = run.tests.map((t, k) => `<li data-k="${k}"><span class="ic"></span><span class="name" title="${esc(t.name)}">${esc(t.name)}</span></li>`).join("");
  selectFirmware(S.blob.m[fwId] ? fwId : 0, step);
}
function selectFirmware(id, step = 0) {
  pause();
  const mod = S.mod, run = S.run;
  const meta = id ? mod.mutants.find((m) => m.id === id) : null;
  const rec = id ? S.blob.m[id] : null;
  S.fw = meta ? { ...meta, killed: rec.k, verdict: rec.vd } : { id: 0 };
  S.tr = decode(id ? rec.t : S.blob.o);
  const fw = S.fw;
  const bugFile = fw.id ? mod.files.findIndex((f) => f.name === fw.file) + 1 : 0;
  S.steps = buildSteps(S.tr, fw.id ? { file: bugFile, line: fw.line } : null, mod.coil_pins || []);
  S.serial = S.tr.serial.map(([t0, t1, text]) => ({ t0, t1, text, ...parseSerial(text) }));
  S.firstStep = run.tests.map((_, k) => S.steps.findIndex((s) => s.testIdx === k));
  [...$("chips").querySelectorAll(".chip")].forEach((b) => b.setAttribute("aria-pressed", Number(b.dataset.fw) === fw.id));
  S.bugFile = bugFile;
  showModuleFile(bugFile || 1);
  const bug = $("bug");
  if (!fw.id) {
    bug.className = "bug";
    bug.innerHTML = run.valid
      ? `<div class="what"><b>Original module</b><br><span class="sub">No injected bug: a valid test must pass here.</span></div><div class="outcome" id="outcome"></div>`
      : `<div class="what"><b>Original module</b><br><span class="sub">No injected bug, yet this test fails here: its expectations are wrong, so the experiment counted it as invalid and did not run it against the mutants.</span></div><div class="outcome" id="outcome"></div>`;
  } else {
    bug.className = "bug mutant";
    bug.innerHTML = `<div class="what"><b>Injected bug #${fw.id} of ${mod.mutants.length}</b><br>` +
      `<span class="sub">${esc(OPS[fw.operator] || fw.operator)} · ${esc(fw.file)} line ${fw.line}</span></div>` +
      `<div class="diff"><div class="del">${esc(fw.before.trim())}</div><div class="add">${esc(fw.after.trim())}</div></div>` +
      `<div class="outcome" id="outcome"></div>`;
  }
  const notes = [];
  if (S.tr.timeout) notes.push("This firmware never finishes; the harness stops it after 15 s of simulated time and counts it as failed.");
  if (S.tr.truncated) notes.push("The recording is shortened: only the first 20 000 events of each kind are kept.");
  if (mod.servo_pins.length) notes.push("Servo angle from the pulse width (Servo library scale: 544 µs = 0°, 2400 µs = 180°).");
  if (S.stepper) notes.push("Stepper shows the rotor angle from the coil sequence (64 half-steps per turn, before the 1:64 gearbox).");
  if (S.lcd) notes.push("Nothing was attached in the experiment (bare avr8js board): the LCD shows what a 16x2 HD44780 would display for the signals recorded on its pins. Circuit drawn for the player from the module's pin map.");
  if (Object.values(S.pinParts).flat().some(({ part }) => /button|switch|potentiometer|joystick/.test(part.p.type)))
    notes.push("Inputs are not operated: on the bare simulated board, tests set input levels themselves.");
  S.note.textContent = notes.join(" ");
  boardResize.observe(S.note);
  drawTimeline();
  fitBoard();
  seek(Math.min(step, S.steps.length - 1));
}

/* ---------- LCD: decoded text, cursor and the custom characters (CGRAM) ---------- */
function renderLcd(st) {
  const el = S.lcd;
  const [, text, flags] = st.lcd >= 0 ? S.tr.lcd[st.lcd] : [0, " ".repeat(32), 0];
  const on = (flags & 1) !== 0;
  el.characters = new Uint8Array([...text].map((ch) => (on ? ch.charCodeAt(0) : 32)));
  el.cursor = on && (flags & 2) !== 0;
  el.blink = on && (flags & 4) !== 0;
  el.cursorX = (flags >> 8) % 16;
  el.cursorY = (flags >> 8) >= 16 ? 1 : 0;
  const hex = st.glyph >= 0 ? S.tr.glyphs[st.glyph][1] : "";
  if (hex !== S.lcdGlyphs) {
    S.lcdGlyphs = hex;
    const font = new Uint8Array(S.lcdFont);
    // HD44780 rows have the leftmost pixel in bit 4, the element draws bit 0 leftmost; codes 8-15 mirror 0-7
    const rev5 = (v) => ((v & 1) << 4) | ((v & 2) << 2) | (v & 4) | ((v & 8) >> 2) | ((v & 16) >> 4);
    for (let i = 0; i < 64 && hex; i++) {
      const row = rev5(parseInt(hex.substr(i * 2, 2), 16));
      font[i] = row;
      font[64 + i] = row;
    }
    el.font = font;
  }
}

/* ---------- rendering of one step ---------- */
function seek(i) {
  S.cur = Math.max(0, Math.min(i, S.steps.length - 1));
  render();
}
function render() {
  const st = S.steps[S.cur], run = S.run, fw = S.fw;
  // board
  for (const [pin, list] of Object.entries(S.pinParts)) {
    for (const { part, pin: partPin } of list) {
      const { el, p } = part;
      if (p.type === "wokwi-led" && partPin === "A") { const v = st.lv[pin]; el.value = v > 0; el.brightness = v >= 255 ? 1 : v / 255; }
      else if (p.type === "wokwi-servo" && partPin === "PWM") el.angle = servoAngle(st.sv[pin]);
      else if (p.type === "wokwi-buzzer") el.hasSignal = st.lv[pin] > 0;
      else if (p.type === "wokwi-lcd1602" && partPin === "A") el.backlight = st.lv[pin] > 0;
    }
  }
  if (S.stepper) S.stepper.angle = st.angle;
  if (S.lcd) renderLcd(st);
  const tx = S.serial[st.serialDone];
  const sending = !!tx && tx.t0 <= st.c;
  if (S.uno) { S.uno.led13 = st.lv[13] > 0; S.uno.ledTX = sending; }

  // serial monitor
  const lines = S.serial.slice(0, st.serialDone).map((s) => {
    const cls = s.test ? (s.ok ? "pass" : "fail") : "meta";
    return `<div class="${cls}">${esc(s.text)}</div>`;
  });
  if (sending) {
    const n = Math.round(tx.text.length * Math.min(1, (st.c - tx.t0) / Math.max(1, tx.t1 - tx.t0)));
    lines.push(`<div class="typing">${esc(tx.text.slice(0, n))}</div>`);
  }
  const ser = $("serial");
  ser.innerHTML = lines.join("");
  ser.scrollTop = ser.scrollHeight;

  // test statuses: a test is done once its @TEST line has been printed
  const done = {};
  let ended = null;
  for (const s of S.serial.slice(0, st.serialDone)) { if (s.test) done[s.test] = s; if (s.end) ended = s; }
  const atEnd = S.cur === S.steps.length - 1;
  const hung = S.tr.timeout && atEnd;
  const items = $("tests").children;
  run.tests.forEach((t, k) => {
    const li = items[k], d = done[t.name];
    li.className = d ? (d.ok ? "pass" : "fail") : k === st.testIdx && !hung ? "running" : ended || hung ? "skipped" : "";
    let msg = li.querySelector(".msg");
    if (d && !d.ok) {
      if (!msg) { msg = document.createElement("span"); msg.className = "msg"; li.append(msg); }
      msg.textContent = d.msg;
    } else if (msg) msg.remove();
  });
  const nDone = Object.keys(done).length, nFail = Object.values(done).filter((d) => !d.ok).length;
  const failed = nFail || (ended && ended.passed < ended.total) || hung;
  const vd = $("verdict"), oc = $("outcome");
  let cls = "", text = `${nDone} / ${run.tests.length} done`, out = "", outCls = "";
  if (ended || hung) {
    const why = hung ? "firmware hung" : `${nFail || ended.total - ended.passed} failed`;
    if (!fw.id) {
      cls = failed ? "fail" : "pass";
      text = failed ? `${why}: invalid test` : `all ${ended.total} passed`;
      out = failed ? "test is invalid" : "test is valid"; outCls = failed ? "fail" : "caught";
    } else {
      cls = failed ? "caught" : "missed";
      text = failed ? `${why}: bug caught` : `all ${ended.total} passed: bug missed`;
      out = failed ? "caught by the tests" : "missed by the tests"; outCls = cls;
    }
  }
  vd.className = "right verdict " + cls; vd.textContent = text;
  if (oc) { oc.className = "outcome " + outCls; oc.textContent = out || "result: run the tests"; }

  // test code: current line, current test block, failed assertions
  const rows = S.testRows;
  for (let n = 1; n < rows.length; n++) rows[n].classList.remove("cur", "block", "failed");
  const t = run.tests[st.testIdx];
  if (t) for (let n = t.from; n <= t.to && n < rows.length; n++) rows[n].classList.add("block");
  for (const d of Object.values(done)) if (!d.ok && rows[d.line]) { rows[d.line].classList.add("failed"); rows[d.line].dataset.msg = "← " + d.why; }
  if (rows[st.testLine]) { rows[st.testLine].classList.add("cur"); scrollIntoPanel($("testCode"), rows[st.testLine]); }

  // module code: the line running right now (only while execution is inside the module)
  if (st.modLine && st.modFile && st.modFile !== S.shownFile) showModuleFile(st.modFile);
  else if (S.cur === 0 && S.bugFile && S.shownFile !== S.bugFile) showModuleFile(S.bugFile);
  const mrows = S.moduleRows;
  for (let n = 1; n < mrows.length; n++) mrows[n].classList.remove("cur");
  if (st.modLine && st.modFile === S.shownFile && mrows[st.modLine]) { mrows[st.modLine].classList.add("cur"); scrollIntoPanel($("moduleCode"), mrows[st.modLine]); }
  else if (fw.id && S.shownFile === S.bugFile && mrows[fw.line] && S.cur === 0) scrollIntoPanel($("moduleCode"), mrows[fw.line]);

  updateTimeline();
  $("readout").innerHTML = `step <b>${S.cur + 1}</b> / ${S.steps.length} &middot; t = <b>${(st.c / FREQ * 1000).toFixed(3)}</b> ms simulated` +
    (st.testIdx >= 0 ? ` &middot; test <b>${st.testIdx + 1}</b> / ${run.tests.length}` : "");
}

/* ---------- timeline (outputs over steps) ---------- */
function partLabel({ part, pin }) {
  const t = part.p.type, a = part.p.attrs || {};
  if (t === "wokwi-buzzer") return "buzzer";
  if (t === "wokwi-lcd1602") return "LCD " + pin;
  if (t === "wokwi-pushbutton") return "button " + (a.label || "");
  return t.replace("wokwi-", "");
}
function timelineRows() {
  const rows = new Map();
  for (const [pinKey, list] of Object.entries(S.pinParts)) {
    const pin = Number(pinKey);
    for (const { part, pin: partPin } of list) {
      const a = part.p.attrs || {};
      if (part.p.type === "wokwi-led" && partPin === "A")
        rows.set(pin, { pin, kind: "level", label: a.label || a.color || "LED", color: LED_COLORS[a.color] || "#e53935" });
      else if (part.p.type === "wokwi-servo" && partPin === "PWM")
        rows.set(pin, { pin, kind: "servo", label: "servo", color: "#2a78d6" });
      else if (part.p.type === "wokwi-buzzer")
        rows.set(pin, { pin, kind: "level", label: "buzzer", color: "#c0392b" });
    }
  }
  (S.mod.coil_pins || []).forEach((pin, i) => rows.set(pin, { pin, kind: "level", label: "coil IN" + (i + 1), color: "#5b6b7a" }));
  for (const [, pin] of S.tr.pins) {
    if (pin <= 1 || rows.has(pin)) continue;
    const list = S.pinParts[pin];
    rows.set(pin, { pin, kind: "level", label: list ? partLabel(list[0]) : "not connected", color: "#8a8a84", loose: !list });
  }
  const out = [...rows.values()].sort((a, b) => a.pin - b.pin);
  if (S.lcd) out.unshift({ pin: -1, kind: "lcd", label: "text changes", color: "#2e7d6b" });
  return out;
}
function drawTimeline() {
  const svg = $("timeline"), steps = S.steps, N = steps.length, run = S.run;
  const W = svg.clientWidth || 1000;
  const rows = timelineRows();
  const short = window.innerHeight < 800;                  // laptop screens: leave the height to the board
  const rowH = rows.length > 6 ? (short ? 10 : 13) : short ? 15 : 20, LBL = 120, bandH = 22, bugH = S.fw.id ? 14 : 0;
  const top = bandH + 6 + bugH, H = top + rows.length * rowH + 4;
  const x = (i) => LBL + (i * (W - LBL - 6)) / N;
  svg.setAttribute("viewBox", `0 0 ${W} ${H}`);
  svg.style.height = H + "px";
  let h = "";
  run.tests.forEach((t, k) => {
    const a = S.firstStep[k];
    if (a < 0) return;
    let b = a;
    while (b + 1 < N && steps[b + 1].testIdx === k) b++;
    const x0 = x(a), w = x(b + 1) - x0;
    h += `<rect x="${x0}" y="0" width="${w}" height="${H}" fill="${k % 2 ? "#f6f6f3" : "#ffffff"}"/>`;
    const name = t.name, fit = Math.floor((w - 8) / 6.1);
    if (fit >= 3) h += `<text x="${x0 + 4}" y="13" font-size="10.5" fill="#55544f">${esc(fit < name.length ? name.slice(0, fit - 1) + "…" : name)}</text>`;
    h += `<rect class="bandstatus" data-k="${k}" x="${x0 + 1}" y="${bandH - 4}" width="${Math.max(1, w - 2)}" height="4" rx="2" fill="#e4e3de"/>`;
  });
  if (bugH) {
    h += `<text x="6" y="${bandH + 6 + bugH - 4}" font-size="10.5" fill="#c62828">bug line runs</text>`;
    for (let i = 0; i < N; i++) {
      if (!steps[i].bug) continue;
      let k = i;
      while (k + 1 < N && steps[k + 1].bug) k++;
      h += `<rect x="${x(i)}" y="${bandH + 7}" width="${Math.max(2, x(k + 1) - x(i))}" height="${bugH - 4}" fill="#c62828" rx="1"/>`;
      i = k;
    }
  }
  rows.forEach((r, j) => {
    const y = top + j * rowH;
    h += `<text x="6" y="${y + rowH * 0.72}" font-size="${rowH > 14 ? 11.5 : 10}" fill="${r.loose ? "#8a8a84" : "#33332f"}">${r.pin < 0 ? "LCD" : pinName(r.pin)}<tspan fill="#8a8a84"> ${esc(r.label)}</tspan></text>`;
    h += `<line x1="${LBL}" x2="${W - 6}" y1="${y + rowH - 2}" y2="${y + rowH - 2}" stroke="#e4e3de"/>`;
    if (r.kind === "lcd") {
      for (let i = 1; i < N; i++) if (steps[i].lcd !== steps[i - 1].lcd)
        h += `<rect x="${x(i)}" y="${y + 2}" width="${Math.max(1.5, x(i + 1) - x(i))}" height="${rowH - 4}" fill="${r.color}" rx="1"/>`;
      return;
    }
    if (r.kind === "servo") {
      const yy = (i) => y + rowH - 3 - (servoAngle(steps[i].sv[r.pin]) / 180) * (rowH - 5);
      const pts = [];
      for (let i = 0; i < N; i++) if (i === 0 || i === N - 1 || steps[i].sv[r.pin] !== steps[i - 1].sv[r.pin]) {
        if (i > 0) pts.push(`${x(i)},${yy(i - 1)}`);
        pts.push(`${x(i)},${yy(i)}`);
      }
      pts.push(`${x(N)},${yy(N - 1)}`);
      h += `<polyline points="${pts.join(" ")}" fill="none" stroke="${r.color}" stroke-width="1.8"/>`;
      return;
    }
    let i = 0;
    while (i < N) {
      const v = steps[i].lv[r.pin];
      if (!v) { i++; continue; }
      let k = i;
      while (k + 1 < N && steps[k + 1].lv[r.pin] === v) k++;
      const op = v >= 255 ? 1 : 0.2 + 0.8 * (v / 255);
      h += `<rect x="${x(i)}" y="${y + 2}" width="${Math.max(1.5, x(k + 1) - x(i))}" height="${rowH - 4}" rx="2" fill="${r.color}" fill-opacity="${op.toFixed(2)}"/>`;
      i = k + 1;
    }
  });
  h += `<rect id="tlFuture" y="0" height="${H}" fill="#ffffff" opacity=".62"/>`;
  h += `<line id="tlHead" y1="0" y2="${H}" stroke="#1c1c1a" stroke-width="1.5"/>`;
  svg.innerHTML = h;
  S.tl = { x, N, W, LBL, bands: [...svg.querySelectorAll(".bandstatus")] };
}
function updateTimeline() {
  if (!S.tl) return;
  const { x, W, bands } = S.tl, st = S.steps[S.cur];
  const xc = (x(S.cur) + x(S.cur + 1)) / 2;
  const head = $("tlHead"), fut = $("tlFuture");
  head.setAttribute("x1", xc); head.setAttribute("x2", xc);
  fut.setAttribute("x", x(S.cur + 1)); fut.setAttribute("width", Math.max(0, W - 6 - x(S.cur + 1)));
  const done = {};
  for (const s of S.serial.slice(0, st.serialDone)) if (s.test) done[s.test] = s;
  for (const r of bands) {
    const k = Number(r.dataset.k), d = done[S.run.tests[k].name];
    r.setAttribute("fill", d ? (d.ok ? "#2e7d32" : "#c62828") : k === st.testIdx ? "#2a78d6" : "#e4e3de");
  }
}

/* ---------- playback ---------- */
const PLAY = '<path d="M4 2l10 6-10 6z"/>', PAUSE = '<path d="M3 2h4v12H3zM9 2h4v12H9z"/>';
function play() {
  if (S.cur >= S.steps.length - 1) seek(0);
  S.playing = true; S.acc = 0; S.last = performance.now();
  $("playIcon").innerHTML = PAUSE;
  clearTimeout(S.timer);
  S.timer = setTimeout(tick, 16);
}
function pause() { S.playing = false; clearTimeout(S.timer); $("playIcon").innerHTML = PLAY; }
function tick() {
  if (!S.playing) return;
  const now = performance.now();
  S.acc += ((now - S.last) / 1000) * S.speed;
  S.last = now;
  if (S.acc >= 1) {
    const n = Math.floor(S.acc);
    S.acc -= n;
    seek(S.cur + n);
    if (S.cur >= S.steps.length - 1) { pause(); return; }
  }
  S.timer = setTimeout(tick, 16);
}
function jumpTest(dir) {
  const k = S.steps[S.cur].testIdx;
  const targets = S.firstStep.map((s, i) => ({ s, i })).filter(({ s }) => s >= 0);
  if (dir > 0) { const t = targets.find(({ s }) => s > S.cur); seek(t ? t.s : S.steps.length - 1); }
  else {
    const before = targets.filter(({ s }) => s < S.cur);
    const t = before.length && before[before.length - 1].i === k && S.cur - before[before.length - 1].s < 2 ? before[before.length - 2] : before[before.length - 1];
    seek(t ? t.s : 0);
  }
}
// switching firmware keeps the place: same test, same number of steps into it, so original and mutant compare directly
function switchFirmware(id) {
  const st = S.steps[S.cur], k = st ? st.testIdx : -1;
  const offset = k >= 0 ? S.cur - S.firstStep[k] : S.cur;
  selectFirmware(id);
  const start = k >= 0 ? S.firstStep[k] : 0;
  if (start >= 0) {
    let end = start;
    while (end + 1 < S.steps.length && (k < 0 || S.steps[end + 1].testIdx === k)) end++;
    seek(Math.min(start + offset, end));
  }
}
const runIndex = () => S.mod.runs.indexOf(S.run);
const moduleIndex = () => INDEX.modules.indexOf(S.mod);
// switching runs keeps the selected mutant when the other run has it; runs that did not compile are skipped
function switchRun(ri, dir = 1) {
  const n = S.mod.runs.length;
  for (let k = 0; k < n; k++) {
    const i = (((ri + k * dir) % n) + n) % n;
    if (S.mod.runs[i].built) return selectRun(i, S.fw.id);
  }
}
const switchModule = (mi) => selectModule((mi + INDEX.modules.length) % INDEX.modules.length);

/* ---------- wiring ---------- */
const groups = [...new Set(INDEX.modules.map((m) => m.experiment))];
$("module").innerHTML = groups.map((g) => `<optgroup label="${esc(g)}">` + INDEX.modules.map((m, i) => (m.experiment === g
  ? `<option value="${i}">${esc(m.name)} (${esc(m.id)})</option>` : "")).join("") + "</optgroup>").join("");
$("module").addEventListener("change", (e) => selectModule(Number(e.target.value)));
$("fileSelect").addEventListener("change", (e) => showModuleFile(Number(e.target.value)));
$("bPrevModule").onclick = () => switchModule(moduleIndex() - 1);
$("bNextModule").onclick = () => switchModule(moduleIndex() + 1);
$("runs").addEventListener("click", (e) => { const b = e.target.closest(".run"); if (b) switchRun(Number(b.dataset.i)); });
$("chips").addEventListener("click", (e) => { const b = e.target.closest(".chip"); if (b) switchFirmware(Number(b.dataset.fw)); });
$("tests").addEventListener("click", (e) => {
  const li = e.target.closest("li");
  if (li && S.firstStep[Number(li.dataset.k)] >= 0) { pause(); seek(S.firstStep[Number(li.dataset.k)]); }
});
$("speed").insertAdjacentHTML("beforeend", SPEEDS.map((v) => `<button data-v="${v}" aria-pressed="${v === S.speed}">${v}</button>`).join(""));
$("speed").addEventListener("click", (e) => {
  const b = e.target.closest("button");
  if (!b) return;
  S.speed = Number(b.dataset.v);
  [...$("speed").querySelectorAll("button")].forEach((x) => x.setAttribute("aria-pressed", x === b));
});
$("bPlay").onclick = () => (S.playing ? pause() : play());
$("bRestart").onclick = () => { pause(); seek(0); };
$("bBack").onclick = () => { pause(); seek(S.cur - 1); };
$("bFwd").onclick = () => { pause(); seek(S.cur + 1); };
$("bPrevTest").onclick = () => { pause(); jumpTest(-1); };
$("bNextTest").onclick = () => { pause(); jumpTest(1); };
const tl = $("timeline");
let dragging = false;
const seekAt = (e) => {
  if (!S.tl) return;
  const r = tl.getBoundingClientRect(), px = ((e.clientX - r.left) * S.tl.W) / r.width;
  const i = Math.floor(((px - S.tl.LBL) * S.tl.N) / (S.tl.W - S.tl.LBL - 6));
  if (i >= 0) seek(i);
};
tl.addEventListener("pointerdown", (e) => { dragging = true; tl.setPointerCapture(e.pointerId); pause(); seekAt(e); });
tl.addEventListener("pointermove", (e) => { if (dragging) seekAt(e); });
tl.addEventListener("pointerup", () => { dragging = false; });
document.addEventListener("keydown", (e) => {
  if (e.target.closest && e.target.closest("input, textarea, select")) return;
  if (e.code === "Space") { e.preventDefault(); S.playing ? pause() : play(); }
  else if (e.key === "ArrowRight") { e.preventDefault(); pause(); e.shiftKey ? jumpTest(1) : seek(S.cur + 1); }
  else if (e.key === "ArrowLeft") { e.preventDefault(); pause(); e.shiftKey ? jumpTest(-1) : seek(S.cur - 1); }
  else if (e.key === "Home") { pause(); seek(0); }
  else if (e.key === "End") { pause(); seek(S.steps.length - 1); }
  else if (e.key === "Tab") { e.preventDefault(); const d = e.shiftKey ? -1 : 1; switchRun(runIndex() + d, d); }
  else if (e.key === "PageDown") { e.preventDefault(); switchModule(moduleIndex() + 1); }
  else if (e.key === "PageUp") { e.preventDefault(); switchModule(moduleIndex() - 1); }
  else if (/^Digit\d$/.test(e.code)) {
    const d = Number(e.code.slice(5));
    const id = e.shiftKey && d === 0 ? 10 : d;
    if (id === 0 || S.blob.m[id]) switchFirmware(id);
  }
});
let resizeTimer = 0;
window.addEventListener("resize", () => {
  clearTimeout(resizeTimer);
  resizeTimer = setTimeout(() => { fitBoard(); if (S.fw) { drawTimeline(); updateTimeline(); } }, 80);
});

// start: #module=led_chaser&model=gpt-5.6-terra&rag=on&fw=1&step=167 opens a given state (steps count from 1, as in
// the readout); &test=6 jumps to the start of test 6; &play=1 starts playback. scene= is accepted for module=.
const params = new URLSearchParams(location.hash.slice(1));
const want = params.get("module") || params.get("scene") || INDEX.start.module;
const mi = Math.max(0, INDEX.modules.findIndex((m) => m.id === want));
const mod0 = INDEX.modules[mi];
const ri = mod0.runs.findIndex((r) => r.model === (params.get("model") || INDEX.start.model) && r.rag === (params.get("rag") || INDEX.start.rag));
selectModule(mi, { run: ri >= 0 ? ri : 0, fw: Number(params.get("fw") || 0), step: Math.max(0, Number(params.get("step") || 1) - 1) }).then(() => {
  const k = Number(params.get("test") || 0) - 1;
  if (k >= 0 && S.firstStep[k] >= 0) seek(S.firstStep[k]);
  if (params.get("play") === "1") play();
});
