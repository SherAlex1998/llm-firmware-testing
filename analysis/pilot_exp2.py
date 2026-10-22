"""Infographics of the experiment 2 pilot: PNG figures (300 dpi) and one self-contained HTML page, in Russian and English.

    python analysis/pilot_exp2.py [--results results/exp2/pilot] [--lang ru|en|all]

Needs `python -m framework.runner.aggregate --exp2 --pilot` first. Writes <results>/figures/ + infographic.html
(Russian) and figures_en/ + infographic_en.html (English).
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

PASS_C, FAIL_C, NONE_C, KILL_C, LIVE_C = "#2a78d6", "#e3a33b", "#d9d8d2", "#1f3f68", "#eef1f5"
MODEL_C = {"claude-sonnet-5": "#c2603a", "gpt-5.6-terra": "#2f8f6b", "claude-haiku-4-5": "#7a5bc0"}
CONDS = [(m, r) for m in MODELS for r in ("off", "on")]
LETTER = {"claude-sonnet-5": "S", "gpt-5.6-terra": "T", "claude-haiku-4-5": "H"}

TEXT = {
    "ru": {
        "no_compile": "не скомпил.", "tests": "тесты {p}/{t}", "mutants": "мутанты {k}/10", "pruned": "очищ. {k}/10",
        "size": "{tier}, {k:.0f} тыс. симв.",
        "legend_matrix": ["прошёл на оригинале", "упал на оригинале (очищ. — после отключения упавших)",
                          "не скомпилировался"],
        "stages": ["скомпилирован", "с первой попытки", "прошёл на оригинале",
                   "есть mutation score\n(с очищенным тестом)"],
        "runs_of_6": "прогонов из 6", "funnel_title": "Путь теста: из 6 прогонов на модель",
        "size_axis": "размер кода модуля, тыс. символов", "tests_axis": "тестов в ответе модели",
        "tests_title": "Сколько тестов модель решила написать (закрашено — все верны, контур — есть неверный)",
        "tests_note": "круг — RAG off, квадрат — RAG on; линия — среднее по модели",
        "not_compiled": "не скомпилирован",
        "legend_mutants": ["убит", "выжил", "очищенный тест (исходный тест упал на оригинале)"],
        "mutants_note": "* тривиальный мутант (его убивает ручной smoke-тест);  S — Sonnet 5, T — GPT-5.6 Terra, "
                        "H — Haiku 4.5",
        "cut": "обр.\n×{n}", "cost_axis": "стоимость прогона, $",
        "cost_title": "Стоимость LLM за прогон (всего ${total:.2f}; «обр.» — ответ оборван лимитом токенов)",
        "lang": "ru", "title": "Пилот эксперимента 2",
        "sub": "Генерация тестов для прошивок Arduino Uno: 3 модуля (medium и complex) × 3 модели × RAG off/on, "
               "1 прогон на ячейку · 2026-10-12",
        "cards": ["прогонов: 3 модуля × 6 условий", "ошибок инфраструктуры", "тест прошёл на исходном модуле",
                  "скомпилированных тестов получили mutation score (с очищенным)", "LLM за весь пилот",
                  "запусков Wokwi, 0 ошибок облака"],
        "note": "Пилот проверяет инфраструктуру, стоимость и расход Wokwi. На каждую пару «модель × RAG» здесь 3 прогона, "
                "поэтому выводов об эффекте RAG или о сравнении моделей эти данные не дают — только описание.",
        "caps": {
            "p1": "Каждая клетка — один прогон. Синие тесты прошли на исходном (правильном) модуле и проверены на 10 "
                  "мутантах; у жёлтых упавшие тесты отключены, остаток («очищенный тест») тоже проверен на мутантах.",
            "p2": "Terra прошла на оригинале во всех 6 прогонах, Sonnet — в 4, Haiku — ни в одном; но с очищенным тестом "
                  "mutation score есть почти у всех скомпилированных тестов.",
            "p3": "Число тестов в эксперименте 2 решает модель. Haiku пишет больше всех (19–32), и почти все верны, но "
                  "одного неверного достаточно для «упал».",
            "p4": "Какие мутанты (внедрённые баги) ловит каждый тест. pomodoro_rtc трудный: 8 из 10 мутантов не убил никто.",
            "p5": "Sonnet — 75% стоимости пилота; самый дорогой прогон оборвался на лимите 16 000 токенов дважды. "
                  "Для основного прогона лимит поднят до 32 000."},
        "head": ["модуль", "модель", "RAG", "попытки", "компиляция", "тесты", "вердикт", "мутанты", "очищ. тест", "$"],
        "yes": "да", "no": "нет",
    },
    "en": {
        "no_compile": "did not compile", "tests": "tests {p}/{t}", "mutants": "mutants {k}/10", "pruned": "pruned {k}/10",
        "size": "{tier}, {k:.0f}k chars",
        "legend_matrix": ["passed on the original", "failed on the original (pruned: failing tests disabled)",
                          "did not compile"],
        "stages": ["compiled", "on the first attempt", "passed on the original",
                   "has a mutation score\n(with the pruned test)"],
        "runs_of_6": "runs out of 6", "funnel_title": "Test pipeline: 6 runs per model",
        "size_axis": "module code size, thousand characters", "tests_axis": "tests in the model's answer",
        "tests_title": "How many tests the model chose to write (filled: all correct, outline: at least one wrong)",
        "tests_note": "circle: RAG off, square: RAG on; line: mean per model",
        "not_compiled": "did not compile",
        "legend_mutants": ["killed", "survived", "pruned test (the original test failed on the correct module)"],
        "mutants_note": "* trivial mutant (killed by the hand-written smoke test);  S = Sonnet 5, T = GPT-5.6 Terra, "
                        "H = Haiku 4.5",
        "cut": "cut\n×{n}", "cost_axis": "cost per run, $",
        "cost_title": "LLM cost per run (total ${total:.2f}; \"cut\": answer cut off by the token limit)",
        "lang": "en", "title": "Experiment 2 pilot",
        "sub": "Test generation for Arduino Uno firmware: 3 modules (medium and complex) × 3 models × RAG off/on, "
               "1 run per cell · 2026-10-12",
        "cards": ["runs: 3 modules × 6 conditions", "infrastructure errors", "tests passed on the original module",
                  "compiled tests got a mutation score (with the pruned test)", "LLM cost of the whole pilot",
                  "Wokwi runs, 0 cloud errors"],
        "note": "The pilot checks the infrastructure, cost and Wokwi usage. Each model × RAG pair has only 3 runs here, "
                "so these data support no conclusions about the effect of RAG or about model differences — they are "
                "descriptive only.",
        "caps": {
            "p1": "Each cell is one run. Blue tests passed on the original (correct) module and were run against 10 "
                  "mutants; for yellow ones the failing tests were disabled and the rest (the \"pruned test\") was run "
                  "against the mutants as well.",
            "p2": "Terra passed on the original in all 6 runs, Sonnet in 4, Haiku in none; with the pruned test, almost "
                  "every compiled test has a mutation score.",
            "p3": "In experiment 2 the model decides how many tests to write. Haiku writes the most (19–32) and almost "
                  "all of them are correct, but one wrong test is enough for a fail.",
            "p4": "Which mutants (injected bugs) each test catches. pomodoro_rtc is hard: 8 of 10 mutants were killed "
                  "by no test.",
            "p5": "Sonnet accounts for 75% of the pilot cost; the most expensive run was cut off by the 16,000-token "
                  "limit twice. The limit is raised to 32,000 for the main run."},
        "head": ["module", "model", "RAG", "attempts", "compiled", "tests", "verdict", "mutants", "pruned test", "$"],
        "yes": "yes", "no": "no",
    },
}
OUT = {"ru": ("figures", "infographic.html"), "en": ("figures_en", "infographic_en.html")}
PAGE_CSS = """body{margin:0;background:#fcfcfb;color:#0b0b0b;font:15px/1.5 "Segoe UI",system-ui,sans-serif}
main{max-width:980px;margin:0 auto;padding:24px 16px 48px} h1{font-size:26px;margin:0 0 4px}
.sub{color:#52514e;margin:0 0 20px} .cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px;margin-bottom:18px}
.card{background:#fff;border:1px solid #e1e0d9;border-radius:10px;padding:12px} .card b{display:block;font-size:24px;color:#2a78d6}
.card span{font-size:13px;color:#52514e} .note{background:#fff6e6;border:1px solid #f0d8a8;border-radius:10px;padding:10px 14px;font-size:14px}
section{background:#fff;border:1px solid #e1e0d9;border-radius:10px;padding:14px;margin:16px 0} section img{width:100%;height:auto}
section p{margin:8px 2px 0;color:#33322f;font-size:14px} table{border-collapse:collapse;width:100%;font-size:13px;background:#fff}
th,td{border-bottom:1px solid #e1e0d9;padding:5px 6px;text-align:left} th{background:#f3f2ee} .wrap{overflow-x:auto}
"""


def load(results: Path):
    runs = pd.read_parquet(results / "results.parquet")
    muts = pd.read_parquet(results / "mutants_pruned.parquet")
    from framework.harness.sketch import module_sources
    sizes = {m: sum(len(t) for t in module_sources(ROOT / "benchmark" / m).values()) for m in runs.module_id.unique()}
    order = sorted(sizes, key=sizes.get)                     # modules from small to large
    trunc = {}
    for rid in runs.run_id:
        for line in (results / "runs" / rid / "run.jsonl").read_text(encoding="utf-8").splitlines():
            e = json.loads(line)
            if e["event"] == "generation":
                trunc[rid] = e["stop_reasons"].count("max_tokens")
    runs["truncated"] = runs.run_id.map(trunc).fillna(0).astype(int)
    return runs, muts, order, sizes


def cond_label(m: str, r: str) -> str:
    return f"{SHORT[m]}\nRAG {r}"


def fig_matrix(runs, order, sizes, path, L):
    fig, ax = plt.subplots(figsize=(7.6, 2.9))
    for i, mod in enumerate(order):
        for j, (m, r) in enumerate(CONDS):
            row = runs[(runs.module_id == mod) & (runs.model == m) & (runs.rag == r)].iloc[0]
            if not row.compile_ok:
                color, lines = NONE_C, [L["no_compile"]]
            elif row.exec_verdict == "pass":
                color = PASS_C
                lines = [L["tests"].format(p=row.tests_passed, t=row.tests_total),
                         L["mutants"].format(k=int(row.mutants_killed))]
            else:
                color = FAIL_C
                lines = [L["tests"].format(p=row.tests_passed, t=row.tests_total),
                         L["pruned"].format(k=int(row.pruned_killed)) if pd.notna(row.pruned_killed) else "—"]
            ax.add_patch(plt.Rectangle((j, i), 0.96, 0.92, color=color, zorder=2))
            txt = "white" if color == PASS_C else INK
            ax.text(j + 0.48, i + 0.46, "\n".join(lines), ha="center", va="center", fontsize=7.2, color=txt, zorder=3)
    ax.set_xlim(0, len(CONDS)); ax.set_ylim(len(order), 0)
    ax.set_xticks([j + 0.48 for j in range(len(CONDS))], [cond_label(m, r) for m, r in CONDS], fontsize=7.5)
    tiers = runs.drop_duplicates("module_id").set_index("module_id").complexity
    ax.set_yticks([i + 0.46 for i in range(len(order))],
                  [f"{m}\n" + L["size"].format(tier=tiers[m], k=sizes[m] / 1000) for m in order], fontsize=7.5)
    ax.tick_params(length=0); ax.xaxis.tick_top()
    for s in ax.spines.values():
        s.set_visible(False)
    handles = [plt.Rectangle((0, 0), 1, 1, color=c) for c in (PASS_C, FAIL_C, NONE_C)]
    ax.legend(handles, L["legend_matrix"], frameon=False, fontsize=7, loc="upper center", bbox_to_anchor=(0.5, -0.02),
              ncols=3)
    fig.tight_layout(); fig.savefig(path); plt.close(fig)


def fig_funnel(runs, path, L):
    counts = [lambda g: g.compile_ok.sum(), lambda g: (g.compile_ok & (g.gen_attempts == 1)).sum(),
              lambda g: (g.exec_verdict == "pass").sum(), lambda g: g.pruned_score.notna().sum()]
    fig, ax = plt.subplots(figsize=(6.6, 3.0))
    width = 0.2
    for k, (name, f) in enumerate(zip(L["stages"], counts)):
        vals = [f(runs[runs.model == m]) for m in MODELS]
        pos = [x + (k - 1.5) * width for x in range(len(MODELS))]
        shade = [ON, "#6aa0e0", "#1f5fa8", "#9aa6b4"][k]
        ax.bar(pos, vals, width * 0.92, color=shade, label=name, zorder=3)
        for x, v in zip(pos, vals):
            ax.text(x, v + 0.1, str(int(v)), ha="center", fontsize=7.5)
    ax.set_xticks(range(len(MODELS)), [SHORT[m] for m in MODELS])
    ax.set_ylim(0, 7.2); ax.set_yticks(range(0, 7)); ax.set_ylabel(L["runs_of_6"])
    ax.yaxis.grid(True, zorder=0)
    ax.set_title(L["funnel_title"], loc="left", fontsize=10)
    ax.legend(frameon=False, fontsize=7, ncols=2, loc="upper center", bbox_to_anchor=(0.5, -0.12))
    fig.tight_layout(); fig.savefig(path); plt.close(fig)


def fig_tests(runs, sizes, path, L):
    fig, ax = plt.subplots(figsize=(6.4, 3.2))
    for mi, m in enumerate(MODELS):
        for r, dx in (("off", -0.05 + 0.03 * (mi - 1)), ("on", 0.05 + 0.03 * (mi - 1))):
            g = runs[(runs.model == m) & (runs.rag == r) & runs.compile_ok]
            xs = [sizes[x] / 1000 * (1 + dx) for x in g.module_id]
            ok = g.exec_verdict == "pass"
            ax.scatter([x for x, o in zip(xs, ok) if o], g.tests_total[ok], s=34, color=MODEL_C[m], zorder=3,
                       marker="o" if r == "off" else "s")
            ax.scatter([x for x, o in zip(xs, ok) if not o], g.tests_total[~ok], s=34, facecolors="white",
                       edgecolors=MODEL_C[m], linewidths=1.4, zorder=3, marker="o" if r == "off" else "s")
        mean = runs[(runs.model == m) & runs.compile_ok].groupby("module_id").tests_total.mean()
        ax.plot([sizes[x] / 1000 for x in sorted(mean.index, key=sizes.get)],
                [mean[x] for x in sorted(mean.index, key=sizes.get)], color=MODEL_C[m], lw=1, alpha=0.6, label=SHORT[m])
    from matplotlib.ticker import NullFormatter, NullLocator
    ax.set_xscale("log"); ax.set_xticks([7, 26, 56], ["7", "26", "56"])
    ax.xaxis.set_minor_locator(NullLocator()); ax.xaxis.set_minor_formatter(NullFormatter())
    ax.set_xlim(5.5, 72)
    ax.set_xlabel(L["size_axis"]); ax.set_ylabel(L["tests_axis"])
    ax.yaxis.grid(True, zorder=0)
    ax.set_title(L["tests_title"], loc="left", fontsize=8.5)
    ax.legend(frameon=False, fontsize=7.5, loc="upper right")
    fig.text(0.01, 0.01, L["tests_note"], fontsize=6.5, color=MUTED)
    fig.tight_layout(rect=(0, 0.03, 1, 1)); fig.savefig(path); plt.close(fig)


def fig_mutants(runs, muts, order, path, L):
    fig, axes = plt.subplots(1, len(order), figsize=(9.6, 3.6))
    for ax, mod in zip(axes, order):
        cat = json.loads((ROOT / "results" / "exp2" / "mutants_catalog" / mod / "catalog.json").read_text(encoding="utf-8"))
        for j, (m, r) in enumerate(CONDS):
            row = runs[(runs.module_id == mod) & (runs.model == m) & (runs.rag == r)].iloc[0]
            g = muts[muts.run_id == row.run_id].set_index("mutant_id")
            if not row.compile_ok:
                ax.text(j + 0.47, 5, L["not_compiled"], rotation=90, ha="center", va="center", fontsize=6, color=MUTED)
            for i in range(1, 11):
                if i in g.index:
                    color = KILL_C if g.loc[i, "killed"] else LIVE_C
                    hatch = "////" if row.exec_verdict != "pass" else None
                else:
                    color, hatch = "white", None
                ax.add_patch(plt.Rectangle((j, i - 1), 0.94, 0.9, facecolor=color, hatch=hatch,
                                           edgecolor="#9aa6b4" if hatch else color, lw=0, zorder=2))
        ax.set_xlim(0, 6); ax.set_ylim(10, 0)
        ax.set_xticks([j + 0.47 for j in range(6)], [f"{LETTER[m]}\n{r}" for m, r in CONDS], fontsize=6.5)
        ax.set_yticks([i + 0.45 for i in range(10)],
                      [("* " if e["trivial"] else "") + f"{e['file']}: {e['operator']}" for e in cat["mutants"]], fontsize=5.6)
        ax.tick_params(length=0); ax.xaxis.tick_top()
        for s in ax.spines.values():
            s.set_visible(False)
        ax.set_title(mod, fontsize=8, pad=22)
    handles = [plt.Rectangle((0, 0), 1, 1, color=KILL_C), plt.Rectangle((0, 0), 1, 1, color=LIVE_C),
               plt.Rectangle((0, 0), 1, 1, facecolor=LIVE_C, hatch="////", edgecolor="#9aa6b4")]
    fig.legend(handles, L["legend_mutants"], frameon=False, fontsize=7, loc="lower center", ncols=3,
               bbox_to_anchor=(0.5, 0.035))
    fig.text(0.01, 0.01, L["mutants_note"], fontsize=6.5, color=MUTED)
    fig.tight_layout(rect=(0, 0.1, 1, 1)); fig.savefig(path); plt.close(fig)


def fig_cost(runs, order, path, L):
    fig, ax = plt.subplots(figsize=(7.2, 3.1))
    width = 0.13
    for k, (m, r) in enumerate(CONDS):
        vals, pos = [], []
        for i, mod in enumerate(order):
            row = runs[(runs.module_id == mod) & (runs.model == m) & (runs.rag == r)].iloc[0]
            vals.append(row.cost_usd); pos.append(i + (k - 2.5) * width)
            if row.truncated:
                ax.text(pos[-1], row.cost_usd + 0.02, L["cut"].format(n=row.truncated), ha="center", fontsize=6,
                        color="#a33")
        ax.bar(pos, vals, width * 0.9, color=MODEL_C[m], alpha=0.55 if r == "off" else 1.0,
               label=f"{SHORT[m]} RAG {r}", zorder=3)
    ax.set_xticks(range(len(order)), order)
    ax.set_ylabel(L["cost_axis"]); ax.yaxis.grid(True, zorder=0)
    ax.set_ylim(0, runs.cost_usd.max() * 1.25)
    ax.set_title(L["cost_title"].format(total=runs.cost_usd.sum()), loc="left", fontsize=8.5)
    ax.legend(frameon=False, fontsize=6.5, ncols=3, loc="upper left")
    fig.tight_layout(); fig.savefig(path); plt.close(fig)


def page(runs, figs: dict[str, Path], results: Path, L) -> str:
    from framework.harness.run_wokwi import wokwi_usage_summary
    w = wokwi_usage_summary(results / "wokwi_usage.csv")
    comp = runs[runs.compile_ok]
    values = ["18", "0", f"{(runs.exec_verdict == 'pass').sum()} / 18", f"{comp.pruned_score.notna().sum()} / {len(comp)}",
              f"${runs.cost_usd.sum():.2f}", str(w["runs"])]
    cards = "".join(f'<div class="card"><b>{v}</b><span>{html.escape(t)}</span></div>' for v, t in zip(values, L["cards"]))
    rows = "".join(
        f"<tr><td>{html.escape(r.module_id)}</td><td>{SHORT[r.model]}</td><td>{r.rag}</td><td>{r.gen_attempts}</td>"
        f"<td>{L['yes'] if r.compile_ok else L['no']}</td>"
        f"<td>{f'{r.tests_passed}/{r.tests_total}' if r.compile_ok else '—'}</td>"
        f"<td>{r.exec_verdict if r.compile_ok else '—'}</td>"
        f"<td>{'' if pd.isna(r.mutants_killed) else f'{int(r.mutants_killed)}/10'}</td>"
        f"<td>{'' if pd.isna(r.pruned_killed) else f'{int(r.pruned_killed)}/10'}</td><td>{r.cost_usd:.3f}</td></tr>"
        for r in runs.sort_values(["module_id", "model", "rag"]).itertuples())
    body = "".join(
        f'<section><img alt="{k}" src="data:image/png;base64,{base64.b64encode(p.read_bytes()).decode()}">'
        f"<p>{html.escape(L['caps'][k])}</p></section>" for k, p in figs.items())
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


def build(results: Path, lang: str) -> Path:
    L = TEXT[lang]
    runs, muts, order, sizes = load(results)
    fig_dir, page_name = OUT[lang]
    out = results / fig_dir
    out.mkdir(parents=True, exist_ok=True)
    figs = {"p1": out / "p1_matrix.png", "p2": out / "p2_funnel.png", "p3": out / "p3_tests_vs_size.png",
            "p4": out / "p4_mutants.png", "p5": out / "p5_cost.png"}
    fig_matrix(runs, order, sizes, figs["p1"], L)
    fig_funnel(runs, figs["p2"], L)
    fig_tests(runs, sizes, figs["p3"], L)
    fig_mutants(runs, muts, order, figs["p4"], L)
    fig_cost(runs, order, figs["p5"], L)
    (results / page_name).write_text(page(runs, figs, results, L), encoding="utf-8")
    print(f"{lang}: {len(figs)} figures -> {out}; page -> {results / page_name}")
    return results / page_name


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--results", default=str(ROOT / "results" / "exp2" / "pilot"))
    ap.add_argument("--lang", choices=["ru", "en", "all"], default="all")
    args = ap.parse_args()
    for lang in (["ru", "en"] if args.lang == "all" else [args.lang]):
        build(Path(args.results), lang)
    return 0


if __name__ == "__main__":
    sys.exit(main())
