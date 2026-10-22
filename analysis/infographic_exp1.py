"""Infographics of experiment 1: PNG figures (300 dpi) and a self-contained HTML page, in Russian and English.

    python analysis/infographic_exp1.py [--results results/main_v3] [--out analysis/infographic_exp1] [--lang ru|en|all]

Writes <out>/figures/ + infographic.html (Russian) and <out>/figures_en/ + infographic_en.html (English).
"""
from __future__ import annotations

import argparse
import base64
import html
import json
import sys
from pathlib import Path

import pandas as pd

sys.path[:0] = [str(Path(__file__).resolve().parent), str(Path(__file__).resolve().parents[1])]
from analyze import INK, MODELS, MUTED, ON, ROOT, SHORT, plt  # noqa: E402  (also sets the plot style)
from pilot_exp2 import (CONDS, FAIL_C, KILL_C, LETTER, LIVE_C, MODEL_C, NONE_C, PAGE_CSS, PASS_C,  # noqa: E402
                        cond_label)

TEXT = {
    "ru": {
        "no_compile": "не скомпил.", "tests": "тесты {p}/{t}", "mutants": "мутанты {k}/10", "size": "{k:.1f} тыс. симв.",
        "legend_matrix": {"pass": "прошёл на оригинале", "fail": "упал на оригинале (мутанты не запускались)",
                          "none": "не скомпилировался"},
        "stages": ["скомпилирован", "с первой попытки", "прошёл на оригинале"],
        "runs_of": "прогонов из {n}", "funnel_title": "Путь теста: из {n} прогонов на модель (15 модулей × RAG off/on)",
        "size_axis": "размер кода модуля, тыс. символов", "tests_axis": "тестов в ответе модели",
        "tests_title": "Сколько тестов написала модель (закрашено — все верны, контур — есть неверный)",
        "band": "промпт просил 3–8 тестов", "tests_note": "круг — RAG off, квадрат — RAG on; пунктир — среднее по модели",
        "mutant": "{op}, стр. {line}",
        "legend_mutants": ["убит", "выжил", "нет данных: тест упал на оригинале"],
        "mutants_note": "S — Sonnet 5, T — GPT-5.6 Terra, H — Haiku 4.5",
        "cost_axis": "стоимость прогона, $", "cost_title": "Стоимость LLM за прогон (всего ${total:.2f})",
        "lang": "ru", "title": "Эксперимент 1: простые модули",
        "sub": "Генерация тестов для прошивок Arduino Uno: 15 простых модулей (светодиоды, кнопки, моторы) × 3 модели × "
               "RAG off/on, 1 прогон на ячейку, офлайн-симулятор avr8js",
        "cards": ["прогонов: 15 модулей × 6 условий", "ошибок инфраструктуры",
                  "тестов скомпилировалось ({first} с первой попытки)", "тест прошёл на исходном модуле",
                  "средний mutation score (убито {killed} из {total} мутантов)", "LLM за весь эксперимент"],
        "note": "Статистика — в analysis/REPORT.md: ни одно сравнение RAG off / on не значимо после поправки Холма; "
                "GPT-5.6 Terra и Sonnet 5 дают валидный тест значимо чаще, чем Haiku 4.5; Terra убивает больше "
                "мутантов, чем Sonnet. Здесь — описательная картина тех же 90 прогонов.",
        "caps": {
            "p1": "Каждая клетка — один прогон. Синие тесты прошли на исходном (правильном) модуле и проверены на 10 "
                  "мутантах; жёлтые упали на исходном модуле, и мутанты для них не запускались.",
            "p2": "Скомпилировались {comp} тестов из 90, {first} — с первой попытки. Прошли на исходном модуле: "
                  "Terra {pt} из 30, Sonnet {ps}, Haiku {ph}.",
            "p3": "Промпт эксперимента 1 просил 3–8 тестов (полоса). Haiku писал {h}, Sonnet {s}, Terra {t}. Модули "
                  "маленькие: {smin:.1f}–{smax:.1f} тыс. символов кода.",
            "p4": "Какие мутанты (внедрённые баги) ловит каждый тест. Серые столбцы — тест упал на исходном модуле. "
                  "{never} из 150 мутантов не убил ни один тест.",
            "p5": "Весь эксперимент стоил ${total:.2f} по тарифам API; Sonnet 5 — {share:.0%} стоимости."},
        "head": ["модуль", "модель", "RAG", "попытки", "компиляция", "тесты", "вердикт", "мутанты", "$"],
        "yes": "да", "no": "нет",
    },
    "en": {
        "no_compile": "did not compile", "tests": "tests {p}/{t}", "mutants": "mutants {k}/10", "size": "{k:.1f}k chars",
        "legend_matrix": {"pass": "passed on the original", "fail": "failed on the original (no mutants run)",
                          "none": "did not compile"},
        "stages": ["compiled", "on the first attempt", "passed on the original"],
        "runs_of": "runs out of {n}", "funnel_title": "Test pipeline: {n} runs per model (15 modules × RAG off/on)",
        "size_axis": "module code size, thousand characters", "tests_axis": "tests in the model's answer",
        "tests_title": "How many tests the model wrote (filled: all correct, outline: at least one wrong)",
        "band": "prompt asked for 3–8 tests", "tests_note": "circle: RAG off, square: RAG on; dashed: mean per model",
        "mutant": "{op}, line {line}",
        "legend_mutants": ["killed", "survived", "no data: the test failed on the original"],
        "mutants_note": "S = Sonnet 5, T = GPT-5.6 Terra, H = Haiku 4.5",
        "cost_axis": "cost per run, $", "cost_title": "LLM cost per run (total ${total:.2f})",
        "lang": "en", "title": "Experiment 1: simple modules",
        "sub": "Test generation for Arduino Uno firmware: 15 simple modules (LEDs, buttons, motors) × 3 models × "
               "RAG off/on, 1 run per cell, offline avr8js simulator",
        "cards": ["runs: 15 modules × 6 conditions", "infrastructure errors",
                  "tests compiled ({first} on the first attempt)", "tests passed on the original module",
                  "mean mutation score ({killed} of {total} mutants killed)", "LLM cost of the whole experiment"],
        "note": "Statistics are in analysis/REPORT.md: no RAG off / on comparison is significant after the Holm "
                "correction; GPT-5.6 Terra and Sonnet 5 produce a valid test significantly more often than Haiku 4.5; "
                "Terra kills more mutants than Sonnet. This page is a descriptive view of the same 90 runs.",
        "caps": {
            "p1": "Each cell is one run. Blue tests passed on the original (correct) module and were run against 10 "
                  "mutants; yellow ones failed on the original module, so no mutants were run for them.",
            "p2": "{comp} of 90 tests compiled, {first} on the first attempt. Passed on the original module: Terra {pt} "
                  "of 30, Sonnet {ps}, Haiku {ph}.",
            "p3": "The experiment 1 prompt asked for 3–8 tests (band). Haiku wrote {h}, Sonnet {s}, Terra {t}. The "
                  "modules are small: {smin:.1f}–{smax:.1f} thousand characters of code.",
            "p4": "Which mutants (injected bugs) each test catches. Grey columns: the test failed on the original "
                  "module. {never} of 150 mutants were killed by no test.",
            "p5": "The whole experiment cost ${total:.2f} at API prices; Sonnet 5 accounts for {share:.0%}."},
        "head": ["module", "model", "RAG", "attempts", "compiled", "tests", "verdict", "mutants", "$"],
        "yes": "yes", "no": "no",
    },
}
OUT = {"ru": ("figures", "infographic.html"), "en": ("figures_en", "infographic_en.html")}


def load(results: Path):
    runs = pd.read_parquet(results / "results.parquet")
    muts = pd.read_parquet(results / "mutants.parquet")
    from framework.harness.sketch import module_sources
    sizes = {m: sum(len(t) for t in module_sources(ROOT / "benchmark" / m).values()) for m in runs.module_id.unique()}
    return runs, muts, sorted(sizes, key=sizes.get), sizes


def cell(runs, mod, m, r):
    return runs[(runs.module_id == mod) & (runs.model == m) & (runs.rag == r)].iloc[0]


def fig_matrix(runs, order, sizes, path, L):
    fig, ax = plt.subplots(figsize=(7.6, 8.4))
    seen = set()
    for i, mod in enumerate(order):
        for j, (m, r) in enumerate(CONDS):
            row = cell(runs, mod, m, r)
            if not row.compile_ok:
                kind, lines = "none", [L["no_compile"]]
            elif row.exec_verdict == "pass":
                kind = "pass"
                lines = [L["tests"].format(p=row.tests_passed, t=row.tests_total),
                         L["mutants"].format(k=int(row.mutants_killed))]
            else:
                kind, lines = "fail", [L["tests"].format(p=row.tests_passed, t=row.tests_total), "—"]
            seen.add(kind)
            color = {"pass": PASS_C, "fail": FAIL_C, "none": NONE_C}[kind]
            ax.add_patch(plt.Rectangle((j, i), 0.96, 0.92, color=color, zorder=2))
            ax.text(j + 0.48, i + 0.46, "\n".join(lines), ha="center", va="center", fontsize=6.6,
                    color="white" if kind == "pass" else INK, zorder=3)
    ax.set_xlim(0, len(CONDS)); ax.set_ylim(len(order), 0)
    ax.set_xticks([j + 0.48 for j in range(len(CONDS))], [cond_label(m, r) for m, r in CONDS], fontsize=7.5)
    ax.set_yticks([i + 0.46 for i in range(len(order))],
                  [f"{m}\n" + L["size"].format(k=sizes[m] / 1000) for m in order], fontsize=7)
    ax.tick_params(length=0); ax.xaxis.tick_top()
    for s in ax.spines.values():
        s.set_visible(False)
    kinds = [k for k in ("pass", "fail", "none") if k in seen]
    ax.legend([plt.Rectangle((0, 0), 1, 1, color={"pass": PASS_C, "fail": FAIL_C, "none": NONE_C}[k]) for k in kinds],
              [L["legend_matrix"][k] for k in kinds], frameon=False, fontsize=7, loc="upper center",
              bbox_to_anchor=(0.5, -0.005), ncols=len(kinds))
    fig.tight_layout(); fig.savefig(path); plt.close(fig)


def fig_funnel(runs, path, L):
    counts = [lambda g: g.compile_ok.sum(), lambda g: (g.compile_ok & (g.gen_attempts == 1)).sum(),
              lambda g: (g.exec_verdict == "pass").sum()]
    n = int(runs.groupby("model").size().max())
    fig, ax = plt.subplots(figsize=(6.6, 3.0))
    width = 0.26
    for k, (name, f) in enumerate(zip(L["stages"], counts)):
        vals = [f(runs[runs.model == m]) for m in MODELS]
        pos = [x + (k - 1) * width for x in range(len(MODELS))]
        ax.bar(pos, vals, width * 0.92, color=[ON, "#6aa0e0", "#1f5fa8"][k], label=name, zorder=3)
        for x, v in zip(pos, vals):
            ax.text(x, v + n * 0.015, str(int(v)), ha="center", fontsize=7.5)
    ax.set_xticks(range(len(MODELS)), [SHORT[m] for m in MODELS])
    ax.set_ylim(0, n * 1.2); ax.set_yticks(range(0, n + 1, 5)); ax.set_ylabel(L["runs_of"].format(n=n))
    ax.yaxis.grid(True, zorder=0)
    ax.set_title(L["funnel_title"].format(n=n), loc="left", fontsize=10)
    ax.legend(frameon=False, fontsize=7, ncols=3, loc="upper center", bbox_to_anchor=(0.5, -0.12))
    fig.tight_layout(); fig.savefig(path); plt.close(fig)


def fig_tests(runs, sizes, path, L):
    fig, ax = plt.subplots(figsize=(6.4, 3.2))
    ax.axhspan(3, 8, color="#eef1f5", zorder=0)
    ax.text(min(sizes.values()) / 1000 - 0.05, 3.15, L["band"], fontsize=6.5, color=MUTED, va="bottom")
    for mi, m in enumerate(MODELS):
        for r, dx in (("off", -0.02 + 0.012 * (mi - 1)), ("on", 0.02 + 0.012 * (mi - 1))):
            g = runs[(runs.model == m) & (runs.rag == r) & runs.compile_ok]
            xs = [sizes[x] / 1000 + dx * 2 for x in g.module_id]
            ok = (g.exec_verdict == "pass").tolist()
            ax.scatter([x for x, o in zip(xs, ok) if o], g.tests_total[g.exec_verdict == "pass"], s=22, color=MODEL_C[m],
                       zorder=3, marker="o" if r == "off" else "s")
            ax.scatter([x for x, o in zip(xs, ok) if not o], g.tests_total[g.exec_verdict != "pass"], s=22,
                       facecolors="white", edgecolors=MODEL_C[m], linewidths=1.2, zorder=3,
                       marker="o" if r == "off" else "s")
        ax.axhline(runs[(runs.model == m) & runs.compile_ok].tests_total.mean(), color=MODEL_C[m], lw=1, ls="--",
                   alpha=0.7, label=SHORT[m], zorder=2)
    ax.set_xlabel(L["size_axis"]); ax.set_ylabel(L["tests_axis"])
    ax.set_ylim(2, runs.tests_total.max() + 1.5)
    ax.yaxis.grid(True, zorder=0)
    ax.set_title(L["tests_title"], loc="left", fontsize=8.5)
    ax.legend(frameon=False, fontsize=7.5, loc="upper left")
    fig.text(0.01, 0.01, L["tests_note"], fontsize=6.5, color=MUTED)
    fig.tight_layout(rect=(0, 0.03, 1, 1)); fig.savefig(path); plt.close(fig)


def fig_mutants(runs, muts, order, path, L):
    cols = 5
    rows = -(-len(order) // cols)
    fig, axes = plt.subplots(rows, cols, figsize=(11.5, 3.0 * rows))
    for ax, mod in zip(axes.flat, order):
        info = muts[muts.module_id == mod].drop_duplicates("mutant_id").set_index("mutant_id")
        for j, (m, r) in enumerate(CONDS):
            row = cell(runs, mod, m, r)
            g = muts[muts.run_id == row.run_id].set_index("mutant_id")
            for i in range(1, 11):
                color = (KILL_C if g.loc[i, "killed"] else LIVE_C) if i in g.index else NONE_C
                ax.add_patch(plt.Rectangle((j, i - 1), 0.92, 0.88, color=color, lw=0, zorder=2))
        ax.set_xlim(0, 6); ax.set_ylim(10, 0)
        ax.set_xticks([j + 0.46 for j in range(6)], [f"{LETTER[m]}\n{r}" for m, r in CONDS], fontsize=5.5)
        ax.set_yticks([i + 0.44 for i in range(10)],
                      [L["mutant"].format(op=info.loc[i, "operator"], line=info.loc[i, "line"]) if i in info.index else ""
                       for i in range(1, 11)], fontsize=5)
        ax.tick_params(length=0); ax.xaxis.tick_top()
        for s in ax.spines.values():
            s.set_visible(False)
        ax.set_title(mod, fontsize=7.5, pad=18)
    for ax in list(axes.flat)[len(order):]:
        ax.axis("off")
    handles = [plt.Rectangle((0, 0), 1, 1, color=c) for c in (KILL_C, LIVE_C, NONE_C)]
    fig.legend(handles, L["legend_mutants"], frameon=False, fontsize=7.5, loc="lower center", ncols=3,
               bbox_to_anchor=(0.5, 0.012))
    fig.text(0.01, 0.005, L["mutants_note"], fontsize=6.5, color=MUTED)
    fig.tight_layout(rect=(0, 0.035, 1, 1)); fig.savefig(path); plt.close(fig)


def fig_cost(runs, order, path, L):
    fig, ax = plt.subplots(figsize=(10, 3.4))
    width = 0.13
    for k, (m, r) in enumerate(CONDS):
        pos = [i + (k - 2.5) * width for i in range(len(order))]
        vals = [cell(runs, mod, m, r).cost_usd for mod in order]
        ax.bar(pos, vals, width * 0.9, color=MODEL_C[m], alpha=0.55 if r == "off" else 1.0,
               label=f"{SHORT[m]} RAG {r}", zorder=3)
    ax.set_xticks(range(len(order)), order, rotation=30, ha="right", fontsize=7)
    ax.set_ylabel(L["cost_axis"]); ax.yaxis.grid(True, zorder=0)
    ax.set_ylim(0, runs.cost_usd.max() * 1.3)
    ax.set_title(L["cost_title"].format(total=runs.cost_usd.sum()), loc="left", fontsize=9)
    ax.legend(frameon=False, fontsize=6.5, ncols=6, loc="upper left")
    fig.tight_layout(); fig.savefig(path); plt.close(fig)


def facts(runs, muts, sizes) -> dict:
    by = {m: runs[runs.model == m] for m in MODELS}
    rng = {m: f"{by[m].tests_total.min()}–{by[m].tests_total.max()}" for m in MODELS}
    return {"comp": int(runs.compile_ok.sum()), "first": int((runs.compile_ok & (runs.gen_attempts == 1)).sum()),
            "pass": int((runs.exec_verdict == "pass").sum()),
            "pt": int((by["gpt-5.6-terra"].exec_verdict == "pass").sum()),
            "ps": int((by["claude-sonnet-5"].exec_verdict == "pass").sum()),
            "ph": int((by["claude-haiku-4-5"].exec_verdict == "pass").sum()),
            "h": rng["claude-haiku-4-5"], "s": rng["claude-sonnet-5"], "t": rng["gpt-5.6-terra"],
            "smin": min(sizes.values()) / 1000, "smax": max(sizes.values()) / 1000,
            "never": int((~muts.groupby(["module_id", "mutant_id"]).killed.any()).sum()),
            "killed": int(muts.killed.sum()), "mtotal": len(muts), "score": runs.mutation_score.mean(),
            "total": runs.cost_usd.sum(), "share": by["claude-sonnet-5"].cost_usd.sum() / runs.cost_usd.sum()}


def page(runs, figs: dict[str, Path], f: dict, L) -> str:
    values = [str(len(runs)), str(int(runs.error.notna().sum())), f"{f['comp']} / {len(runs)}", f"{f['pass']} / {len(runs)}",
              f"{f['score']:.2f}", f"${f['total']:.2f}"]
    labels = [t.format(first=f["first"], killed=f["killed"], total=f["mtotal"]) for t in L["cards"]]
    cards = "".join(f'<div class="card"><b>{v}</b><span>{html.escape(t)}</span></div>' for v, t in zip(values, labels))
    rows = "".join(
        f"<tr><td>{html.escape(r.module_id)}</td><td>{SHORT[r.model]}</td><td>{r.rag}</td><td>{r.gen_attempts}</td>"
        f"<td>{L['yes'] if r.compile_ok else L['no']}</td>"
        f"<td>{f'{r.tests_passed}/{r.tests_total}' if r.compile_ok else '—'}</td>"
        f"<td>{r.exec_verdict if r.compile_ok else '—'}</td>"
        f"<td>{'' if pd.isna(r.mutants_killed) else f'{int(r.mutants_killed)}/10'}</td><td>{r.cost_usd:.3f}</td></tr>"
        for r in runs.sort_values(["module_id", "model", "rag"]).itertuples())
    body = "".join(
        f'<section><img alt="{k}" src="data:image/png;base64,{base64.b64encode(p.read_bytes()).decode()}">'
        f"<p>{html.escape(L['caps'][k].format(**f))}</p></section>" for k, p in figs.items())
    head = "".join(f"<th>{h}</th>" for h in L["head"])
    return f"""<!doctype html><html lang="{L['lang']}"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1"><title>{L['title']}</title>
<style>
{PAGE_CSS}</style></head><body><main>
<h1>{L['title']}</h1>
<p class="sub">{html.escape(L['sub'])}</p>
<div class="cards">{cards}</div>
<p class="note">{html.escape(L['note'])}</p>
{body}
<section><div class="wrap"><table><tr>{head}</tr>{rows}</table></div></section>
</main></body></html>"""


def build(results: Path, out_root: Path, lang: str) -> Path:
    L = TEXT[lang]
    runs, muts, order, sizes = load(results)
    fig_dir, page_name = OUT[lang]
    out = out_root / fig_dir
    out.mkdir(parents=True, exist_ok=True)
    figs = {"p1": out / "p1_matrix.png", "p2": out / "p2_funnel.png", "p3": out / "p3_tests_vs_size.png",
            "p4": out / "p4_mutants.png", "p5": out / "p5_cost.png"}
    fig_matrix(runs, order, sizes, figs["p1"], L)
    fig_funnel(runs, figs["p2"], L)
    fig_tests(runs, sizes, figs["p3"], L)
    fig_mutants(runs, muts, order, figs["p4"], L)
    fig_cost(runs, order, figs["p5"], L)
    (out_root / page_name).write_text(page(runs, figs, facts(runs, muts, sizes), L), encoding="utf-8")
    print(f"{lang}: {len(figs)} figures -> {out}; page -> {out_root / page_name}")
    return out_root / page_name


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--results", default=str(ROOT / "results" / "main_v3"))
    ap.add_argument("--out", default=str(ROOT / "analysis" / "infographic_exp1"))
    ap.add_argument("--lang", choices=["ru", "en", "all"], default="all")
    args = ap.parse_args()
    for lang in (["ru", "en"] if args.lang == "all" else [args.lang]):
        build(Path(args.results), Path(args.out), lang)
    return 0


if __name__ == "__main__":
    sys.exit(main())
