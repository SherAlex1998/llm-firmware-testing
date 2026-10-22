"""One-page overview of the experiment matrix (modules x conditions) as a self-contained HTML file.

    python analysis/matrix_overview.py [--results results/main_v3] [--out analysis/matrix_overview.html] [--note ...]

Reads results.parquet, mutants.parquet and every run.jsonl (failed checks); the charts are drawn in the browser
from the embedded JSON (inline SVG, no external libraries).
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import pandas as pd
import yaml

ROOT = Path(__file__).resolve().parents[1]
MODELS = ["claude-sonnet-5", "gpt-5.6-terra", "claude-haiku-4-5"]
CATEGORY_ORDER = {"led": 0, "led+motor": 1, "motor": 2}


def collect(results_dir: Path) -> dict:
    runs = pd.read_parquet(results_dir / "results.parquet")
    mutants = pd.read_parquet(results_dir / "mutants.parquet")
    modules = []
    for mid in sorted(runs.module_id.unique()):
        meta = yaml.safe_load((ROOT / "benchmark" / mid / "module.yaml").read_text(encoding="utf-8"))
        modules.append({"id": mid, "category": meta["category"], "name": meta["name"]})
    modules.sort(key=lambda m: (CATEGORY_ORDER.get(m["category"], 9), m["id"]))
    out_runs = []
    for r in runs.itertuples():
        events = [json.loads(line) for line in
                  (results_dir / "runs" / r.run_id / "run.jsonl").read_text(encoding="utf-8").splitlines()]
        ex = next((e for e in events if e["event"] == "execution"), {})
        out_runs.append({
            "module": r.module_id, "model": r.model, "rag": r.rag, "compile_ok": bool(r.compile_ok),
            "attempts": int(r.gen_attempts), "verdict": r.exec_verdict, "passed": int(r.tests_passed),
            "total": int(r.tests_total), "killed": None if pd.isna(r.mutants_killed) else int(r.mutants_killed),
            "mutants": None if pd.isna(r.mutants_total) else int(r.mutants_total),
            "score": None if pd.isna(r.mutation_score) else round(float(r.mutation_score), 3),
            "cost": round(float(r.cost_usd), 4), "failed": ex.get("failed", [])})
    ops = (mutants.dropna(subset=["killed"]).groupby("operator")["killed"]
           .agg(killed="sum", total="count").reset_index())
    return {"modules": modules, "models": MODELS, "runs": out_runs,
            "operators": [{"op": o.operator, "killed": int(o.killed), "total": int(o.total)} for o in ops.itertuples()],
            "spent": round(float(runs.cost_spent_usd.sum()), 2), "cost": round(float(runs.cost_usd.sum()), 2)}


PAGE = r"""<title>Результаты матрицы 15×6</title>
<style>
:root{
  --surface:#fcfcfb; --plane:#f9f9f7; --ink:#0b0b0b; --ink2:#52514e; --muted:#898781; --grid:#e1e0d9; --axis:#c3c2b7;
  --ring:rgba(11,11,11,.10); --cell-empty:#f0efec; --off:#898781; --on:#2a78d6; --critical:#d03b3b;
  --s150:#b7d3f6; --s250:#86b6ef; --s350:#5598e7; --s450:#2a78d6; --s550:#1c5cab; --s650:#104281;
}
@media (prefers-color-scheme: dark){ :root:not([data-theme="light"]){
  --surface:#1a1a19; --plane:#0d0d0d; --ink:#ffffff; --ink2:#c3c2b7; --muted:#898781; --grid:#2c2c2a; --axis:#383835;
  --ring:rgba(255,255,255,.10); --cell-empty:#262624; --off:#898781; --on:#3987e5;
  --s150:#184f95; --s250:#1c5cab; --s350:#256abf; --s450:#2a78d6; --s550:#3987e5; --s650:#6da7ec; } }
:root[data-theme="dark"]{
  --surface:#1a1a19; --plane:#0d0d0d; --ink:#ffffff; --ink2:#c3c2b7; --muted:#898781; --grid:#2c2c2a; --axis:#383835;
  --ring:rgba(255,255,255,.10); --cell-empty:#262624; --off:#898781; --on:#3987e5;
  --s150:#184f95; --s250:#1c5cab; --s350:#256abf; --s450:#2a78d6; --s550:#3987e5; --s650:#6da7ec; }
body{background:var(--plane);color:var(--ink);font:14px/1.5 system-ui,-apple-system,"Segoe UI",sans-serif;
  padding-block:32px 56px;padding-inline:20px;margin:0}
.wrap{max-width:1040px;margin:0 auto;display:grid;gap:28px}
h1{font-size:1.7rem;line-height:1.15;margin:0;text-wrap:balance;font-weight:650}
h2{font-size:1.05rem;margin:0 0 4px;font-weight:600}
.sub{color:var(--ink2);margin:6px 0 0;max-width:72ch}
.note{color:var(--ink2);font-size:.9rem;margin:0 0 12px;max-width:78ch}
.kpis{display:grid;grid-template-columns:repeat(auto-fit,minmax(170px,1fr));gap:12px}
.kpi{background:var(--surface);border:1px solid var(--ring);border-radius:8px;padding:12px 14px}
.kpi .label{color:var(--ink2);font-size:.82rem}
.kpi .value{font-size:1.6rem;font-weight:600;line-height:1.2}
.kpi .hint{color:var(--muted);font-size:.8rem}
.card{background:var(--surface);border:1px solid var(--ring);border-radius:8px;padding:18px 18px 14px}
.scroll{overflow-x:auto}
svg{display:block;font-family:inherit}
svg text{fill:var(--ink2);font-size:12px}
svg .ink{fill:var(--ink)} svg .muted{fill:var(--muted)}
.legend{display:flex;flex-wrap:wrap;gap:6px 18px;color:var(--ink2);font-size:.85rem;margin-top:10px;align-items:center}
.sw{display:inline-block;width:12px;height:12px;border-radius:3px;vertical-align:-1px;margin-right:6px}
.ramp{display:inline-block;width:120px;height:10px;border-radius:3px;vertical-align:0;margin:0 6px;
  background:linear-gradient(90deg,var(--s150),var(--s350),var(--s550),var(--s650))}
.grid2{display:grid;grid-template-columns:repeat(auto-fit,minmax(320px,1fr));gap:28px}
#tip{position:fixed;pointer-events:none;background:var(--surface);color:var(--ink);border:1px solid var(--ring);
  border-radius:6px;padding:8px 10px;font-size:.82rem;box-shadow:0 4px 16px rgba(0,0,0,.12);max-width:340px;z-index:5}
#tip[hidden]{display:none}
#tip b{font-weight:600} #tip .f{color:var(--ink2);margin-top:4px}
details{background:var(--surface);border:1px solid var(--ring);border-radius:8px;padding:10px 14px}
summary{cursor:pointer;color:var(--ink2)}
table{border-collapse:collapse;font-size:.82rem;margin-top:10px;font-variant-numeric:tabular-nums}
th,td{border-bottom:1px solid var(--grid);padding:4px 8px;text-align:left;white-space:nowrap}
th{color:var(--muted);font-weight:500}
.hit{cursor:default} .hit:focus{outline:2px solid var(--on);outline-offset:1px}
</style>
<div class="wrap">
  <header>
    <h1>Результаты матрицы 15×6</h1>
    <p class="sub">Каждый из 15 модулей бенчмарка прогнан в шести условиях: три модели, база знаний (RAG) выключена или
    включена. Тест исполнялся в офлайн-симуляторе <code>avr8js</code>; если он проходил на исходном коде, его прогоняли
    против 10 мутантов модуля.</p>
    <p class="sub">__NOTE__</p>
  </header>
  <section class="kpis" id="kpis"></section>
  <section class="card">
    <h2>Вся матрица</h2>
    <p class="note">Клетка — один прогон. Синяя клетка: тест прошёл на исходном коде, число — доля убитых мутантов
    (цвет по шкале под матрицей). Серая клетка с ✕: тест сам упал на правильном коде, число — сколько проверок прошло.
    ↻ — тест скомпилировался только после починки. Наведите на клетку, чтобы увидеть детали.</p>
    <div class="scroll"><svg id="matrix" role="img" aria-label="Матрица 15 модулей на 6 условий: исход каждого прогона"></svg></div>
    <div class="legend"><span>Mutation score 0<span class="ramp"></span>1</span>
      <span><span class="sw" style="background:var(--cell-empty)"></span>✕ тест упал на исходном коде</span>
      <span>↻ понадобилась починка</span></div>
  </section>
  <div class="grid2">
    <section class="card">
      <h2>Валидные тесты</h2>
      <p class="note">Сколько из 15 сгенерированных тестов прошли на исходном, правильном коде.</p>
      <svg id="valid" role="img" aria-label="Доля валидных тестов по моделям, RAG выключен и включён"></svg>
      <div class="legend"><span><span class="sw" style="background:var(--off)"></span>RAG выключен</span>
        <span><span class="sw" style="background:var(--on)"></span>RAG включён</span></div>
    </section>
    <section class="card">
      <h2>Mutation score</h2>
      <p class="note">Точка — прогон с валидным тестом, черта — среднее. Сколько из 10 мутантов тест поймал.</p>
      <svg id="score" role="img" aria-label="Распределение mutation score по условиям"></svg>
      <div class="legend"><span><span class="sw" style="background:var(--off)"></span>RAG выключен</span>
        <span><span class="sw" style="background:var(--on)"></span>RAG включён</span></div>
    </section>
  </div>
  <section class="card">
    <h2>Какие баги тесты ловят хуже всего</h2>
    <p class="note">Доля убитых мутантов по оператору мутации, по всем прогонам с валидным тестом.</p>
    <svg id="ops" role="img" aria-label="Доля убитых мутантов по оператору мутации"></svg>
  </section>
  <section class="card">
    <h2>Почему тесты падали на исходном коде</h2>
    <p class="note">Все прогоны, где сгенерированный тест не прошёл на правильном коде, и его упавшие проверки.</p>
    <div class="scroll"><table id="fails"></table></div>
  </section>
  <details><summary>Таблица всех прогонов</summary><div class="scroll"><table id="table"></table></div></details>
</div>
<div id="tip" hidden></div>
<script>
const DATA = __DATA__;
const NS = "http://www.w3.org/2000/svg";
const SHORT = {"claude-sonnet-5": "Sonnet 5", "gpt-5.6-terra": "GPT-5.6 Terra", "claude-haiku-4-5": "Haiku 4.5"};
const el = (tag, attrs = {}, parent) => { const e = document.createElementNS(NS, tag);
  for (const [k, v] of Object.entries(attrs)) e.setAttribute(k, v); if (parent) parent.appendChild(e); return e; };
const text = (parent, x, y, s, attrs = {}) => { const t = el("text", {x, y, ...attrs}, parent); t.textContent = s; return t; };
const run = (m, model, rag) => DATA.runs.find(r => r.module === m && r.model === model && r.rag === rag);
const pct = (a, b) => b ? Math.round(100 * a / b) + "%" : "–";
const tip = document.getElementById("tip");
function hover(node, html) {
  node.classList.add("hit"); node.setAttribute("tabindex", "0");
  const show = (x, y) => { tip.innerHTML = html; tip.hidden = false;
    const w = tip.offsetWidth, h = tip.offsetHeight;
    tip.style.left = Math.min(x + 14, innerWidth - w - 8) + "px"; tip.style.top = Math.min(y + 14, innerHeight - h - 8) + "px"; };
  node.addEventListener("mousemove", e => show(e.clientX, e.clientY));
  node.addEventListener("mouseleave", () => tip.hidden = true);
  node.addEventListener("focus", () => { const b = node.getBoundingClientRect(); show(b.right, b.top); });
  node.addEventListener("blur", () => tip.hidden = true);
}
const rampVar = s => s >= 0.95 ? "--s650" : s >= 0.85 ? "--s550" : s >= 0.75 ? "--s450" : s >= 0.6 ? "--s350" : s >= 0.45 ? "--s250" : "--s150";
function runHtml(r) {
  let h = `<b>${r.module}</b> · ${SHORT[r.model]} · RAG ${r.rag}<br>`;
  h += r.compile_ok ? `скомпилировался ${r.attempts === 1 ? "с первой попытки" : "с попытки " + r.attempts}` : "не скомпилировался";
  if (r.verdict === "pass") h += `<br>тест прошёл: ${r.passed}/${r.total} проверок<br>убито мутантов: ${r.killed}/${r.mutants}`;
  else if (r.verdict) h += `<br>тест упал на исходном коде: ${r.passed}/${r.total}` +
    r.failed.slice(0, 3).map(f => `<div class="f">✕ ${f.replace(/</g, "&lt;")}</div>`).join("");
  return h + `<div class="f">стоимость $${r.cost.toFixed(4)}</div>`;
}

// KPIs
(() => {
  const R = DATA.runs, valid = R.filter(r => r.verdict === "pass"), scored = valid.filter(r => r.score !== null);
  const repaired = R.filter(r => r.attempts > 1).length;
  const kill = scored.reduce((a, r) => a + r.killed, 0), mut = scored.reduce((a, r) => a + r.mutants, 0);
  const items = [["Прогонов", R.length, "15 модулей × 6 условий"],
    ["Скомпилировались", `${R.filter(r => r.compile_ok).length}/${R.length}`, repaired ? `${repaired} после починки` : "все с первой попытки"],
    ["Валидные тесты", `${valid.length}/${R.length}`, "прошли на исходном коде"],
    ["Убито мутантов", pct(kill, mut), `${kill} из ${mut} по валидным тестам`],
    ["Потрачено на API", "$" + DATA.spent.toFixed(2), "стоимость ответов модели"]];
  document.getElementById("kpis").innerHTML = items.map(([l, v, h]) =>
    `<div class="kpi"><div class="label">${l}</div><div class="value">${v}</div><div class="hint">${h}</div></div>`).join("");
})();

// Matrix (redrawn on theme change: label ink follows the resolved fill)
const luminance = c => { const [r, g, b] = (c.match(/\d+(\.\d+)?/g) || [0, 0, 0]).slice(0, 3).map(v => {
  v = v / 255; return v <= 0.03928 ? v / 12.92 : ((v + 0.055) / 1.055) ** 2.4; }); return 0.2126 * r + 0.7152 * g + 0.0722 * b; };
function drawMatrix() {
  const svg = document.getElementById("matrix"); svg.innerHTML = ""; const cw = 104, ch = 30, gap = 2, left = 170, top = 58;
  const cols = DATA.models.flatMap(m => ["off", "on"].map(rag => ({m, rag})));
  const W = left + cols.length * (cw + gap) + 8, H = top + DATA.modules.length * (ch + gap) + 6;
  svg.setAttribute("viewBox", `0 0 ${W} ${H}`); svg.setAttribute("width", W); svg.style.maxWidth = "none";
  DATA.models.forEach((m, i) => { const x = left + i * 2 * (cw + gap);
    text(svg, x + cw + gap / 2, 18, SHORT[m], {"text-anchor": "middle", class: "ink", "font-weight": 600});
    el("line", {x1: x, x2: x + 2 * cw + gap, y1: 26, y2: 26, stroke: "var(--axis)"}, svg); });
  cols.forEach((c, j) => text(svg, left + j * (cw + gap) + cw / 2, 46, "RAG " + c.rag, {"text-anchor": "middle", class: "muted"}));
  let prevCat = null;
  DATA.modules.forEach((mod, i) => {
    const y = top + i * (ch + gap);
    text(svg, left - 10, y + ch / 2 + 4, mod.id, {"text-anchor": "end", class: "ink"});
    if (mod.category !== prevCat) { text(svg, 4, y + ch / 2 + 4, mod.category, {class: "muted", "font-size": 11}); prevCat = mod.category; }
    cols.forEach((c, j) => {
      const r = run(mod.id, c.m, c.rag), x = left + j * (cw + gap);
      const g = el("g", {}, svg);
      let fill = "var(--cell-empty)", label = "нет", fg = "var(--ink2)";
      if (r && r.verdict === "pass") { fill = `var(${rampVar(r.score ?? 0)})`; label = (r.score ?? 0).toFixed(1); }
      else if (r && r.verdict) { label = `✕ ${r.passed}/${r.total}`; }
      const rect = el("rect", {x, y, width: cw, height: ch, rx: 4, fill}, g);
      if (r && r.verdict === "pass") fg = luminance(getComputedStyle(rect).fill) > 0.3 ? "#0b0b0b" : "#ffffff";
      const t = text(g, x + cw / 2, y + ch / 2 + 4, label + (r && r.attempts > 1 ? " ↻" : ""), {"text-anchor": "middle", "font-weight": 600});
      t.style.fill = fg;
      if (r && r.verdict && r.verdict !== "pass") { t.textContent = ""; const s = el("tspan", {}, t); s.textContent = "✕ ";
        s.style.fill = "var(--critical)"; const s2 = el("tspan", {}, t); s2.textContent = `${r.passed}/${r.total}` + (r.attempts > 1 ? " ↻" : ""); }
      if (r) hover(g, runHtml(r));
    });
  });
}
drawMatrix();
matchMedia("(prefers-color-scheme: dark)").addEventListener("change", drawMatrix);
new MutationObserver(drawMatrix).observe(document.documentElement, {attributes: true, attributeFilter: ["data-theme"]});

// Valid-test rate by model
(() => {
  const svg = document.getElementById("valid"), W = 460, H = 250, L = 40, B = 40, T = 22, bw = 24, gw = 130;
  svg.setAttribute("viewBox", `0 0 ${W} ${H}`); svg.style.width = "100%"; svg.style.height = "auto";
  const y = v => T + (H - T - B) * (1 - v);
  [0, .25, .5, .75, 1].forEach(v => { el("line", {x1: L, x2: W - 8, y1: y(v), y2: y(v), stroke: "var(--grid)"}, svg);
    text(svg, L - 6, y(v) + 4, Math.round(v * 100) + "%", {"text-anchor": "end", class: "muted"}); });
  DATA.models.forEach((m, i) => { const cx = L + 20 + i * gw + gw / 2;
    ["off", "on"].forEach((rag, k) => {
      const rs = DATA.runs.filter(r => r.model === m && r.rag === rag), ok = rs.filter(r => r.verdict === "pass").length;
      const v = ok / rs.length, x = cx - bw - 5 + k * (bw + 10);
      const g = el("g", {}, svg);
      el("path", {d: `M${x},${y(0)} V${y(v) + 4} a4,4 0 0 1 4,-4 h${bw - 8} a4,4 0 0 1 4,4 V${y(0)} Z`,
        fill: `var(--${rag})`}, g);
      el("rect", {x: x - 4, y: T, width: bw + 8, height: H - T - B, fill: "transparent"}, g);
      text(g, x + bw / 2, y(v) - 6, String(ok), {"text-anchor": "middle", class: "ink", "font-size": 12});
      hover(g, `<b>${SHORT[m]}</b> · RAG ${rag}<br>валидных тестов: ${ok} из ${rs.length} (${pct(ok, rs.length)})`);
    });
    text(svg, cx, H - B + 18, SHORT[m], {"text-anchor": "middle", class: "ink"}); });
  el("line", {x1: L, x2: W - 8, y1: y(0), y2: y(0), stroke: "var(--axis)"}, svg);
})();

// Mutation score strip
(() => {
  const svg = document.getElementById("score"), W = 460, H = 250, L = 40, B = 40, T = 16, gw = 130;
  svg.setAttribute("viewBox", `0 0 ${W} ${H}`); svg.style.width = "100%"; svg.style.height = "auto";
  const scores = DATA.runs.filter(r => r.score !== null).map(r => r.score);
  const lo = Math.min(0.4, Math.floor(Math.min(...scores) * 5) / 5);          // not a bar chart: the axis may start above 0
  const y = v => T + (H - T - B) * (1 - (v - lo) / (1 - lo));
  [lo, lo + (1 - lo) / 3, lo + 2 * (1 - lo) / 3, 1].forEach(v => { el("line", {x1: L, x2: W - 8, y1: y(v), y2: y(v), stroke: "var(--grid)"}, svg);
    text(svg, L - 6, y(v) + 4, v.toFixed(2).replace(/0$/, ""), {"text-anchor": "end", class: "muted"}); });
  DATA.models.forEach((m, i) => { const cx = L + 20 + i * gw + gw / 2;
    ["off", "on"].forEach((rag, k) => {
      const xs = cx + (k ? 22 : -22), rs = DATA.runs.filter(r => r.model === m && r.rag === rag && r.score !== null);
      const seen = {};
      rs.forEach(r => { const key = r.score.toFixed(2), n = seen[key] = (seen[key] || 0) + 1;
        const dx = ((n - 1) % 2 ? 1 : -1) * Math.ceil((n - 1) / 2) * 7;
        const c = el("circle", {cx: xs + dx, cy: y(r.score), r: 4, fill: `var(--${rag})`, stroke: "var(--surface)", "stroke-width": 2}, svg);
        hover(c, runHtml(r)); });
      if (rs.length) { const mean = rs.reduce((a, r) => a + r.score, 0) / rs.length;
        el("line", {x1: xs - 16, x2: xs + 16, y1: y(mean), y2: y(mean), stroke: "var(--ink)", "stroke-width": 2, "stroke-linecap": "round"}, svg);
        text(svg, xs, H - B + 32 - 14, mean.toFixed(2), {"text-anchor": "middle", class: "muted", "font-size": 11}); }
    });
    text(svg, cx, H - B + 32, SHORT[m], {"text-anchor": "middle", class: "ink"}); });
})();

// Kill rate by operator
(() => {
  const svg = document.getElementById("ops"), ops = [...DATA.operators].sort((a, b) => a.killed / a.total - b.killed / b.total);
  const L = 110, R = 150, rowH = 30, W = 900, H = ops.length * rowH + 30, x = v => L + (W - L - R) * v;
  svg.setAttribute("viewBox", `0 0 ${W} ${H}`); svg.style.width = "100%"; svg.style.height = "auto";
  [0, .25, .5, .75, 1].forEach(v => { el("line", {x1: x(v), x2: x(v), y1: 4, y2: H - 22, stroke: "var(--grid)"}, svg);
    text(svg, x(v), H - 6, Math.round(v * 100) + "%", {"text-anchor": "middle", class: "muted"}); });
  ops.forEach((o, i) => { const v = o.killed / o.total, yy = 10 + i * rowH, g = el("g", {}, svg);
    text(g, L - 10, yy + 15, o.op, {"text-anchor": "end", class: "ink"});
    const w = Math.max(x(v) - L, 4);
    el("path", {d: `M${L},${yy + 3} H${L + w - 4} a4,4 0 0 1 4,4 V${yy + 17} a4,4 0 0 1 -4,4 H${L} Z`, fill: "var(--on)"}, g);
    el("rect", {x: 0, y: yy, width: W, height: rowH - 4, fill: "transparent"}, g);
    text(g, L + w + 8, yy + 16, `${pct(o.killed, o.total)} · ${o.killed}/${o.total}`, {class: "ink"});
    hover(g, `<b>${o.op}</b><br>убито ${o.killed} из ${o.total} мутантов (${pct(o.killed, o.total)})`); });
  el("line", {x1: L, x2: L, y1: 4, y2: H - 22, stroke: "var(--axis)"}, svg);
})();

// Failed runs
(() => {
  const esc = t => t.replace(/&/g, "&amp;").replace(/</g, "&lt;");
  const rows = DATA.runs.filter(r => r.verdict && r.verdict !== "pass")
    .sort((a, b) => a.module.localeCompare(b.module) || a.model.localeCompare(b.model) || a.rag.localeCompare(b.rag));
  document.getElementById("fails").innerHTML = "<tr><th>Модуль</th><th>Модель</th><th>RAG</th><th>Прошло</th><th>Упавшие проверки</th></tr>" +
    rows.map(r => `<tr><td>${r.module}</td><td>${SHORT[r.model]}</td><td>${r.rag}</td><td>${r.passed}/${r.total}</td>` +
      `<td style="white-space:normal;min-width:420px">${r.failed.map(esc).join("<br>")}</td></tr>`).join("");
})();

// Table view
(() => {
  const rows = [...DATA.runs].sort((a, b) => a.module.localeCompare(b.module) || a.model.localeCompare(b.model) || a.rag.localeCompare(b.rag));
  document.getElementById("table").innerHTML = "<tr><th>Модуль</th><th>Модель</th><th>RAG</th><th>Попыток</th><th>Вердикт</th>" +
    "<th>Проверки</th><th>Мутанты</th><th>Score</th><th>$</th></tr>" + rows.map(r =>
    `<tr><td>${r.module}</td><td>${SHORT[r.model]}</td><td>${r.rag}</td><td>${r.attempts}</td><td>${r.verdict ?? "—"}</td>` +
    `<td>${r.passed}/${r.total}</td><td>${r.killed ?? "—"}/${r.mutants ?? "—"}</td><td>${r.score ?? "—"}</td><td>${r.cost.toFixed(4)}</td></tr>`).join("");
})();
</script>
"""


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--results", default=str(ROOT / "results" / "main_v3"))
    ap.add_argument("--out", default=str(ROOT / "analysis" / "matrix_overview.html"))
    ap.add_argument("--note", default="", help="HTML shown under the page subtitle (simulator version, caveats)")
    args = ap.parse_args()
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    page = PAGE.replace("__NOTE__", args.note).replace("__DATA__", json.dumps(collect(Path(args.results)), ensure_ascii=False))
    out.write_text(page, encoding="utf-8")
    print(out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
