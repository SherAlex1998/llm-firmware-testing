"""Tables, figures and REPORT.md for the research questions of experiment 1.

    python analysis/analyze.py [--results results/main_v3] [--out analysis]

Idempotent: rebuilds analysis/tables/*.csv, analysis/figures/*.png (300 dpi) and analysis/REPORT.md from the
run tables. Paired tests use the within-module design: the same module under RAG off and on, the same mutant
under both conditions. Exact McNemar (binomial test on the discordant pairs) for binary outcomes, Wilcoxon
signed-rank for mutation score, Wilson intervals for proportions; Holm correction inside each family.
"""
from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import pandas as pd  # noqa: E402
from scipy import stats  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
ALPHA = 0.05
MODELS = ["claude-sonnet-5", "gpt-5.6-terra", "claude-haiku-4-5"]
SHORT = {"claude-sonnet-5": "Sonnet 5", "gpt-5.6-terra": "GPT-5.6 Terra", "claude-haiku-4-5": "Haiku 4.5"}
OFF, ON, INK, MUTED, GRID = "#898781", "#2a78d6", "#0b0b0b", "#52514e", "#e1e0d9"

# Mutants no test killed, reviewed by hand against the module sources (analysis/survivors_review.md).
# equivalent: no observable change, dropped from the *_adj tables; unobservable: differs only inside one blocking
# call; needs_signal: needs a timing or pin-signal measurement; test_gap: an ordinary pin or API check kills it.
SURVIVOR_REVIEW = {
    ("dc_motor_pot", 6): ("test_gap", "после init IN2 = HIGH, а motor_stop() его не гасит; IN2 не проверен"),
    ("dc_motor_pot", 9): ("equivalent", "raw = 1024: map() даёт те же 255, что и raw = 1023"),
    ("dual_hbridge", 4): ("test_gap", "переход назад -> вперёд не проверен, IN2 остаётся HIGH"),
    ("dual_hbridge", 8): ("equivalent", "hb_init() обнуляет g_dir через hb_stop_all()"),
    ("dual_hbridge", 9): ("equivalent", "hb_init() обнуляет g_speed через hb_stop_all()"),
    ("joystick_motor", 8): ("test_gap", "jm_update() проверен только на согласованность, не на значение (в avr8js analogRead = 0)"),
    ("led_breathing", 2): ("test_gap", "скорость 1 не проверена, по умолчанию она 2"),
    ("led_chaser", 1): ("test_gap", "chaser_count_on() читает тот же массив пинов, что и мутант"),
    ("railway_gate", 6): ("needs_signal", "цикл пропущен: угол тот же, но поворот мгновенный (millis())"),
    ("railway_gate", 8): ("needs_signal", "то же для возрастающего цикла"),
    ("railway_gate", 9): ("unobservable", "нет промежуточных углов внутри блокирующего gate_sweep()"),
    ("traffic_light", 3): ("unobservable", "зелёный не мигает внутри блокирующего tl_blink_green()"),
    ("traffic_servo_gate", 3): ("needs_signal", "другая ширина импульса, servo.read() её компенсирует (pulseIn())"),
    ("two_color_crossfade", 4): ("equivalent", "value = 0: ветка присваивает тот же 0"),
    ("two_color_crossfade", 6): ("needs_signal", "геттер верный, а ШИМ на пине 10 не выставлен (pulseIn())"),
}
EQUIVALENT = {key for key, (kind, _) in SURVIVOR_REVIEW.items() if kind == "equivalent"}

plt.rcParams.update({"figure.dpi": 300, "savefig.dpi": 300, "font.family": "DejaVu Sans", "font.size": 9,
                     "axes.edgecolor": "#c3c2b7", "axes.labelcolor": INK, "text.color": INK,
                     "xtick.color": MUTED, "ytick.color": MUTED, "axes.spines.top": False,
                     "axes.spines.right": False, "figure.facecolor": "#fcfcfb", "axes.facecolor": "#fcfcfb",
                     "grid.color": GRID, "grid.linewidth": 0.8})


def wilson(k: int, n: int) -> tuple[float, float]:
    if not n:
        return (float("nan"), float("nan"))
    lo, hi = stats.binomtest(k, n).proportion_ci(confidence_level=0.95, method="wilson")
    return round(lo, 3), round(hi, 3)


def mcnemar(pairs: list[tuple[bool, bool]]) -> dict:
    """Exact McNemar: only the discordant pairs carry information."""
    b = sum(1 for x, y in pairs if x and not y)          # off only
    c = sum(1 for x, y in pairs if y and not x)          # on only
    p = stats.binomtest(c, b + c).pvalue if b + c else 1.0
    return {"n_pairs": len(pairs), "off_only": b, "on_only": c, "p": round(p, 4)}


def wilcoxon(x: list[float], y: list[float]) -> dict:
    diff = [b - a for a, b in zip(x, y)]
    nonzero = [d for d in diff if d]
    p = stats.wilcoxon(x, y).pvalue if nonzero else 1.0
    return {"n_pairs": len(diff), "n_changed": len(nonzero), "mean_diff": round(float(pd.Series(diff).mean()), 4),
            "median_diff": round(float(pd.Series(diff).median()), 4), "p": round(float(p), 4)}


def holm(rows: list[dict], key: str = "p", out: str = "p_holm") -> list[dict]:
    """Holm-Bonferroni inside one family of tests; adds <out> and <out>_significant."""
    order = sorted(range(len(rows)), key=lambda i: rows[i][key])
    running = 0.0
    for rank, i in enumerate(order):
        adjusted = min(1.0, (len(rows) - rank) * rows[i][key])
        running = max(running, adjusted)
        rows[i] = {**rows[i], out: round(running, 4), f"{out}_significant": running < ALPHA}
    return rows


def overview(runs: pd.DataFrame) -> pd.DataFrame:
    rows = []
    for (model, rag), g in runs.groupby(["model", "rag"]):
        valid, n = int(g.valid.sum()), len(g)
        score = g.mutation_score.dropna()
        lo, hi = wilson(valid, n)
        rows.append({"model": model, "rag": rag, "runs": n,
                     "compile_at_1": round(g.compile_at_1.mean(), 3), "compile_at_k": round(g.compile_ok.mean(), 3),
                     "valid_tests": valid, "valid_rate": round(valid / n, 3), "ci_low": lo, "ci_high": hi,
                     "mean_tests": round(g.tests_total.mean(), 1), "mutation_score_mean": round(score.mean(), 3),
                     "mutation_score_n": len(score), "cost_usd": round(g.cost_usd.sum(), 3),
                     "cost_per_valid_test": round(g.cost_usd.sum() / valid, 4) if valid else None})
    return pd.DataFrame(rows).sort_values(["model", "rag"])


def rag_effect(runs: pd.DataFrame, mutants: pd.DataFrame) -> tuple[pd.DataFrame, pd.DataFrame, pd.DataFrame]:
    """RQ2: RAG off vs on, paired within module (valid tests, mutation score) and within mutant (killed)."""
    valid_rows, score_rows, mutant_rows = [], [], []
    for model in MODELS:
        g = runs[runs.model == model].pivot(index="module_id", columns="rag", values="valid")
        valid_rows.append({"model": model, "outcome": "valid test", **mcnemar(list(zip(g["off"], g["on"])))})
        s = runs[runs.model == model].pivot(index="module_id", columns="rag", values="mutation_score").dropna()
        score_rows.append({"model": model, "outcome": "mutation score", "mean_off": round(s["off"].mean(), 3),
                           "mean_on": round(s["on"].mean(), 3), **wilcoxon(list(s["off"]), list(s["on"]))})
        m = (mutants[mutants.model == model]
             .pivot_table(index=["module_id", "mutant_id"], columns="rag", values="killed").dropna())
        mutant_rows.append({"model": model, "outcome": "mutant killed",
                            "killed_off": int(m["off"].sum()), "killed_on": int(m["on"].sum()),
                            **mcnemar([(bool(a), bool(b)) for a, b in zip(m["off"], m["on"])])})
    return (pd.DataFrame(holm(valid_rows)), pd.DataFrame(holm(score_rows)), pd.DataFrame(holm(mutant_rows)))


def model_comparison(runs: pd.DataFrame, mutants: pd.DataFrame) -> pd.DataFrame:
    """RQ3: models against each other, paired by (module, rag)."""
    rows = []
    for i, a in enumerate(MODELS):
        for b in MODELS[i + 1:]:
            pa = runs[runs.model == a].set_index(["module_id", "rag"]).valid
            pb = runs[runs.model == b].set_index(["module_id", "rag"]).valid
            test = mcnemar([(bool(x), bool(y)) for x, y in zip(pa, pb.reindex(pa.index))])
            sa = mutants[mutants.model == a].set_index(["module_id", "rag", "mutant_id"]).killed
            sb = mutants[mutants.model == b].set_index(["module_id", "rag", "mutant_id"]).killed
            common = sa.index.intersection(sb.index)
            mut = mcnemar([(bool(sa[i_]), bool(sb[i_])) for i_ in common])
            rows.append({"model_a": a, "model_b": b, "valid_a": int(pa.sum()), "valid_b": int(pb.sum()),
                         "valid_a_only": test["off_only"], "valid_b_only": test["on_only"], "p_valid": test["p"],
                         "mutants_paired": mut["n_pairs"], "killed_a_only": mut["off_only"],
                         "killed_b_only": mut["on_only"], "p_mutants": mut["p"]})
    return pd.DataFrame(holm(holm(rows, "p_valid", "p_valid_holm"), "p_mutants", "p_mutants_holm"))


def operators(mutants: pd.DataFrame) -> pd.DataFrame:
    g = mutants.groupby("operator").killed.agg(["sum", "count"]).rename(columns={"sum": "killed", "count": "runs"})
    g["survived"] = g["runs"] - g["killed"]
    g["kill_rate"] = (g.killed / g.runs).round(3)
    by_rag = mutants.pivot_table(index="operator", columns="rag", values="killed", aggfunc="mean").round(3)
    return g.join(by_rag.rename(columns={"off": "kill_rate_rag_off", "on": "kill_rate_rag_on"})).reset_index()


def survivors(mutants: pd.DataFrame) -> pd.DataFrame:
    """Mutants no test killed in any condition: candidates for equivalent mutants."""
    g = mutants.groupby(["module_id", "mutant_id", "operator", "line", "before", "after"]).killed.agg(["sum", "count"])
    out = g[g["sum"] == 0].reset_index().drop(columns=["sum"]).rename(columns={"count": "tests_that_missed_it"})
    review = [SURVIVOR_REVIEW.get(key, ("", "")) for key in zip(out.module_id, out.mutant_id)]
    out["review"], out["reason"] = [kind for kind, _ in review], [reason for _, reason in review]
    return out.sort_values(["module_id", "mutant_id"])


def without_equivalent(runs: pd.DataFrame, mutants: pd.DataFrame) -> tuple[pd.DataFrame, pd.DataFrame]:
    """Drop the equivalent mutants and recompute each run's mutation score from the remaining ones."""
    mutants = mutants[[key not in EQUIVALENT for key in zip(mutants.module_id, mutants.mutant_id)]]
    return runs.assign(mutation_score=runs.run_id.map(mutants.groupby("run_id").killed.mean())), mutants


def failures(runs: pd.DataFrame, results_dir: Path) -> pd.DataFrame:
    import json
    rows = []
    for r in runs[runs.exec_verdict == "fail"].itertuples():
        events = [json.loads(line) for line in
                  (results_dir / "runs" / r.run_id / "run.jsonl").read_text(encoding="utf-8").splitlines()]
        for check in next(e for e in events if e["event"] == "execution")["failed"]:
            name, _, message = check.partition(": ")
            rows.append({"module_id": r.module_id, "model": r.model, "rag": r.rag, "test": name, "message": message})
    return pd.DataFrame(rows)


def fig_rates(runs: pd.DataFrame, path: Path) -> None:
    fig, ax = plt.subplots(figsize=(6.2, 3.2))
    width, xs = 0.36, range(len(MODELS))
    for k, (rag, color) in enumerate([("off", OFF), ("on", ON)]):
        vals = [runs[(runs.model == m) & (runs.rag == rag)].valid.mean() for m in MODELS]
        pos = [x + (k - 0.5) * width for x in xs]
        ax.bar(pos, vals, width * 0.92, color=color, label=f"RAG {rag}", zorder=3)
        for x, v in zip(pos, vals):
            ax.text(x, v + 0.02, f"{v:.0%}", ha="center", fontsize=8, color=INK)
    ax.set_xticks(list(xs), [SHORT[m] for m in MODELS])
    ax.set_ylim(0, 1.12)
    ax.set_yticks([0, .25, .5, .75, 1], ["0%", "25%", "50%", "75%", "100%"])
    ax.set_ylabel("Доля валидных тестов")
    ax.set_title("Тесты, прошедшие на исходном коде (15 модулей на условие)", loc="left", fontsize=10)
    ax.yaxis.grid(True, zorder=0)
    ax.legend(frameon=False, loc="upper right", ncols=2)
    fig.tight_layout(); fig.savefig(path); plt.close(fig)


def fig_pass_slope(runs: pd.DataFrame, path: Path) -> None:
    fig, axes = plt.subplots(1, 3, figsize=(7.6, 3.0), sharey=True)
    for ax, model in zip(axes, MODELS):
        g = runs[runs.model == model].pivot(index="module_id", columns="rag", values="valid")
        counts = g.groupby(["off", "on"]).size()
        for (off, on), n in counts.items():
            color = ON if on and not off else OFF if off and not on else MUTED
            ax.plot([0, 1], [int(off), int(on)], color=color, lw=1 + n * 0.5, alpha=.85, zorder=3)
            x = 0.5 if off == on else 0.78                      # off the crossing point, where labels collide
            ax.text(x, int(off) + (int(on) - int(off)) * x + 0.05, f"{n}", ha="center", fontsize=8, color=INK)
        ax.set_xticks([0, 1], ["RAG off", "RAG on"])
        ax.set_yticks([0, 1], ["упал", "прошёл"])
        ax.set_title(SHORT[model], fontsize=9, loc="left")
        ax.set_xlim(-0.25, 1.25); ax.set_ylim(-0.2, 1.25)
    fig.suptitle("Что менялось по модулям при включении RAG (подпись — число модулей)", fontsize=10, x=0.01, ha="left")
    fig.tight_layout(); fig.savefig(path); plt.close(fig)


def fig_score_box(runs: pd.DataFrame, path: Path) -> None:
    fig, ax = plt.subplots(figsize=(6.2, 3.2))
    data, labels, colors = [], [], []
    for model in MODELS:
        for rag in ("off", "on"):
            data.append(runs[(runs.model == model) & (runs.rag == rag)].mutation_score.dropna().tolist())
            labels.append(f"{SHORT[model]}\nRAG {rag}")
            colors.append(OFF if rag == "off" else ON)
    bp = ax.boxplot(data, tick_labels=labels, patch_artist=True, widths=0.55, medianprops={"color": INK})
    for patch, color in zip(bp["boxes"], colors):
        patch.set(facecolor=color, alpha=.35, edgecolor=color)
    for i, (values, color) in enumerate(zip(data, colors), 1):
        ax.plot([i + (j % 5 - 2) * 0.05 for j in range(len(values))], values, "o", ms=3.5, color=color, alpha=.9, zorder=3)
    ax.set_ylabel("Mutation score"); ax.set_ylim(0, 1.05)
    ax.set_title("Доля пойманных мутантов, по прогонам с валидным тестом", loc="left", fontsize=10)
    ax.yaxis.grid(True, zorder=0)
    fig.tight_layout(); fig.savefig(path); plt.close(fig)


def fig_operators(ops: pd.DataFrame, ops_adj: pd.DataFrame, path: Path) -> None:
    """Survival per operator with all mutants (gray) and without the equivalent ones (blue); most missed on top."""
    adj = ops_adj.set_index("operator").sort_values("kill_rate", ascending=False)
    raw = ops.set_index("operator").reindex(adj.index)
    fig, ax = plt.subplots(figsize=(6.2, 3.9))
    height = 0.4
    for k, (df, other, color, label) in enumerate([(raw, adj, OFF, "все мутанты"),
                                                   (adj, raw, ON, "без эквивалентных")]):
        ys = [y + (0.5 - k) * height for y in range(len(df))]
        ax.barh(ys, df.survived / df.runs, height * 0.88, color=color, label=label, zorder=3)
        for y, row, other_rate in zip(ys, df.itertuples(), other.survived / other.runs):
            rate = row.survived / row.runs
            if k == 1 or abs(rate - other_rate) > 1e-9:             # gray bars are labelled only where they differ
                ax.text(rate + 0.01, y, f"{rate:.0%} · {row.survived}/{row.runs}", va="center", fontsize=7.5,
                        color=INK)
    ax.set_yticks(range(len(adj)), adj.index)
    ax.set_xlim(0, 0.75)
    ax.set_xticks([0, .2, .4, .6], ["0%", "20%", "40%", "60%"])
    ax.set_xlabel("Доля выживших мутантов")
    ax.set_title("Какие внесённые баги тесты пропускают чаще всего", loc="left", fontsize=10)
    ax.xaxis.grid(True, zorder=0)
    ax.legend(frameon=False, loc="lower right")
    fig.tight_layout(); fig.savefig(path); plt.close(fig)


def fig_cost(table: pd.DataFrame, path: Path) -> None:
    fig, ax = plt.subplots(figsize=(6.2, 3.2))
    for row in table.itertuples():
        color = OFF if row.rag == "off" else ON
        ax.scatter(row.cost_usd, row.valid_rate, s=70, color=color, zorder=3,
                   edgecolor="#fcfcfb", linewidth=1.5)
        ax.annotate(f"{SHORT[row.model]}, RAG {row.rag}", (row.cost_usd, row.valid_rate),
                    textcoords="offset points", xytext=(8, 5 if row.rag == "on" else -12), fontsize=8, color=MUTED)
    ax.set_xlabel("Стоимость 15 прогонов, $"); ax.set_ylabel("Доля валидных тестов")
    ax.set_xlim(0, table.cost_usd.max() * 1.45); ax.set_ylim(0, 1.1)
    ax.set_yticks([0, .25, .5, .75, 1], ["0%", "25%", "50%", "75%", "100%"])
    ax.set_title("Цена условия против доли валидных тестов", loc="left", fontsize=10)
    ax.grid(True, zorder=0)
    fig.tight_layout(); fig.savefig(path); plt.close(fig)


def fig_similarity(runs: pd.DataFrame, path: Path) -> None:
    sims = runs.rag_similarity_mean.dropna()
    fig, ax = plt.subplots(figsize=(6.2, 3.0))
    ax.hist(sims, bins=12, color=ON, zorder=3)
    ax.axvline(sims.mean(), color=INK, lw=1.5, zorder=4)
    ax.text(sims.mean(), ax.get_ylim()[1] * 0.92, f" среднее {sims.mean():.2f}", fontsize=8, color=INK)
    ax.set_xlabel("Средняя близость найденных кусков к запросу"); ax.set_ylabel("Прогонов")
    ax.set_title("Насколько релевантен найденный контекст (условия с RAG)", loc="left", fontsize=10)
    ax.yaxis.grid(True, zorder=0)
    fig.tight_layout(); fig.savefig(path); plt.close(fig)


def md_table(df: pd.DataFrame) -> str:
    head = "| " + " | ".join(df.columns) + " |\n|" + "---|" * len(df.columns)
    rows = ["| " + " | ".join("" if pd.isna(v) else str(v) for v in row) + " |" for row in df.itertuples(index=False)]
    return "\n".join([head, *rows])


def report(runs, mutants, runs_adj, mutants_adj, tables: dict[str, pd.DataFrame], results_dir: Path) -> str:
    t = tables
    valid, n = int(runs.valid.sum()), len(runs)
    scored, scored_adj = runs.mutation_score.dropna(), runs_adj.mutation_score.dropna()
    killed, total = int(mutants.killed.sum()), len(mutants)
    killed_adj, total_adj = int(mutants_adj.killed.sum()), len(mutants_adj)
    kinds = t["t8_survivors"].review.value_counts()
    valid_by_model = runs.groupby("model").valid.sum()
    cost_by_model = runs.groupby("model").cost_usd.sum()
    failed_by_model = runs[~runs.valid].model.value_counts()
    pair = lambda a, b: t["t5_models"].set_index(["model_a", "model_b"]).loc[(a, b)]
    ops_adj = t["t7_operators_adj"].set_index("operator").kill_rate
    ops_raw = t["t7_operators"].set_index("operator").kill_rate
    sig = lambda df: ", ".join(f"{SHORT[r.model]} p={r.p_holm}" for r in df.itertuples())
    mutant_pairs = int(t["t4_rag_mutants"].n_pairs.sum())
    arrow = lambda v: "выше с RAG" if v > 0 else "ниже с RAG" if v < 0 else "без изменений"
    score_direction = ", ".join(f"{SHORT[r.model]} {r.mean_diff:+.3f} ({arrow(r.mean_diff)})"
                                for r in t["t3_rag_score"].itertuples())
    valid_direction = ", ".join(f"{SHORT[r.model]} {r.on_only - r.off_only:+d} модуля"
                                for r in t["t2_rag_valid"].itertuples())
    return f"""# Результаты эксперимента 1

Данные: `{results_dir.relative_to(ROOT).as_posix() if results_dir.is_relative_to(ROOT) else results_dir.as_posix()}` — {n} прогонов, {len(mutants)} проверок мутантов. Таблицы в `analysis/tables/`,
графики в `analysis/figures/` (300 dpi). Пересобирается командой `python analysis/analyze.py`.

## 1. Что за эксперимент

15 модулей Arduino Uno × 6 условий: три модели (Sonnet 5, GPT-5.6 Terra, Haiku 4.5) × база знаний выключена
или включена. Для каждого прогона: агент генерирует тест-скетч (до 2 починок по выводу компилятора), тест
компилируется `arduino-cli` 1.5.1 и исполняется в офлайн-симуляторе `avr8js` 0.21.1; если он прошёл на исходном
модуле, его прогоняют против 10 детерминированных мутантов (8 операторов, seed 42).
Порядок прогонов перемешан, α = {ALPHA}. Поправка Холма применяется внутри каждого семейства из трёх
сравнений (по одному на модель); семейства — «валидные тесты», «mutation score», «отдельные мутанты»,
и отдельно два семейства в сравнении моделей.

| Показатель | Значение |
|---|---|
| Скомпилировалось | {int(runs.compile_ok.sum())}/{n} ({runs.compile_ok.mean():.0%}), с первой попытки {runs.compile_at_1.mean():.0%} |
| Тест прошёл на исходном коде | {valid}/{n} ({valid / n:.0%}) |
| Mutation score (прогоны с валидным тестом) | среднее {scored.mean():.2f}, медиана {scored.median():.2f}, от {scored.min():.1f} до {scored.max():.1f} |
| Мутанты | убито {killed} из {total} ({killed / total:.0%}) |
| То же без {len(EQUIVALENT)} эквивалентных мутантов | убито {killed_adj} из {total_adj} ({killed_adj / total_adj:.1%}), mutation score: среднее {scored_adj.mean():.2f}, медиана {scored_adj.median():.2f} |
| Стоимость | ${runs.cost_usd.sum():.2f} за {n} прогонов |

{md_table(t["t1_overview"])}

## 2. RQ1. Генерирует ли агент валидные тесты

Компиляция упирается в потолок: {int(runs.compile_ok.sum())} из {n} тестов собрались, починка понадобилась
{int((runs.gen_attempts > 1).sum())} раз. Различий между моделями и условиями по компиляции нет.

Настоящий барьер дальше: {n - valid} из {n} тестов упали на исходном, правильном коде, то есть сами содержали
неверные ожидания. Доля валидных тестов по условиям — в таблице выше (с 95% интервалом Уилсона),
разбор упавших проверок — `analysis/tables/t6_failures.csv`, всего их {len(t["t6_failures"])}.

![Валидные тесты](figures/f1_rates.png)

## 3. RQ2. Помогает ли RAG

Парные сравнения внутри модуля: один и тот же модуль с выключенной и включённой базой знаний.

**Валидные тесты (McNemar по 15 модулям на модель).**

{md_table(t["t2_rag_valid"])}

**Mutation score (Wilcoxon по модулям, где тест прошёл в обоих условиях).**

{md_table(t["t3_rag_score"])}

**По отдельным мутантам (McNemar; пара — один и тот же мутант в обоих условиях, {mutant_pairs} пар).**

{md_table(t["t4_rag_mutants"])}

![Изменения по модулям](figures/f2_pass_slope.png)

Направление эффекта по mutation score: {score_direction}. По валидным тестам: {valid_direction}.
Значимость после поправки Холма (валидные тесты): {sig(t["t2_rag_valid"])} — ни одно сравнение не значимо.
Гипотеза H1, записанная до эксперимента (RAG не влияет на компиляцию), не отвергается: компиляция на потолке.

![Mutation score](figures/f3_score_box.png)

## 4. RQ3. Какая модель лучше

Парные сравнения моделей по (модуль, условие RAG); стоимость — за 15 прогонов условия.

{md_table(t["t5_models"])}

![Цена против качества](figures/f5_cost_vs_pass.png)

## 5. RQ4. Ловят ли тесты внесённые баги

{md_table(t["t7_operators"])}

![Операторы мутаций](figures/f4_operators.png)

Серые полосы — все мутанты, синие — без эквивалентных; серые подписаны только там, где они отличаются.

**Выжившие мутанты.** {len(t["t8_survivors"])} из 150 мутантов не убил ни один тест ни в одном условии
(`analysis/tables/t8_survivors.csv`). Каждый разобран вручную по исходнику модуля, разбор —
`analysis/survivors_review.md`, класс и причина — в столбцах `review` и `reason`:

| Класс | Мутантов | Что это значит |
|---|---|---|
| `equivalent` | {kinds.get("equivalent", 0)} | наблюдаемое поведение не меняется; исключены из пересчёта ниже |
| `unobservable` | {kinds.get("unobservable", 0)} | отличие есть только внутри одного блокирующего вызова, итог и время те же |
| `needs_signal` | {kinds.get("needs_signal", 0)} | ловится только замером времени (`millis()`) или сигнала на пине (`pulseIn()`) |
| `test_gap` | {kinds.get("test_gap", 0)} | пропуск в тестах: убивается обычной проверкой пина или вызовом API |

**Пересчёт без эквивалентных мутантов.** Mutation score по определению считается без эквивалентных мутантов;
сырые таблицы выше оставлены для сопоставимости с первой сборкой. Убито {killed_adj} из {total_adj}
({killed_adj / total_adj:.1%} против {killed / total:.1%}), средний score {scored_adj.mean():.3f} против {scored.mean():.3f}.

{md_table(t["t7_operators_adj"])}

Mutation score по RAG (Wilcoxon, `t3_rag_score_adj.csv`):

{md_table(t["t3_rag_score_adj"])}

McNemar по отдельным мутантам (`t4_rag_mutants_adj.csv`) и сравнение моделей по мутантам не меняются:
эквивалентный мутант не убит ни в одном условии, то есть всегда даёт совпадающую пару, а p считается только
по несовпадающим.

## 6. Контекст RAG

![Близость найденных кусков](figures/f6_rag_similarity.png)

Средняя близость найденных кусков к запросу — {runs.rag_similarity_mean.dropna().mean():.2f}
(от {runs.rag_similarity_mean.dropna().min():.2f} до {runs.rag_similarity_mean.dropna().max():.2f}).
Какие именно куски попали в промпт, записано в `rag_chunks.json` каждого прогона.

## 7. Угрозы валидности

- **Малое n.** 15 модулей, один прогон на условие: случайность генерации не усреднена. Размеры эффекта важнее p.
- **Симулятор вместо платы.** `avr8js` моделирует голую плату: внешние детали не подключены, аналоговые входы
  читают 0, висящий вход детерминированно держит последний уровень (на плате он не определён). Подтяжка
  эмулируется отдельно: библиотека её не моделирует, и без этого правильные тесты падали.
- **Потолок mutation score.** Распределение прижато к 1, различия между условиями малы по величине.
- **Упрощения против протокола диплома.** 15 модулей вместо 50, 6 условий вместо 6 исходных, 8 операторов
  мутаций вместо 12, α = 0.05 с поправкой Холма вместо 0.01 с Бонферрони, RAG с одной стратегией чанкинга.
- **Мутаторы-артефакты.** Удаление `delay()` и `pinMode()` исключено: их исход решает не тест, а модель
  симулятора.
- **Ручная разметка выживших мутантов.** Классы в `t8_survivors.csv` проставил один исследователь, второй
  независимой разметки нет. Эквивалентность обоснована по коду (`analysis/survivors_review.md`), но не доказана
  формально; при спорном решении менялся бы только пересчёт `*_adj`, сырые таблицы от разметки не зависят.

## 8. Выводы

Числа здесь подставляются из таблиц выше при каждой сборке; примеры подобраны вручную.

**RQ1. Валидные тесты.** Синтаксис для агента не проблема: компилируются {int(runs.compile_ok.sum())} из {n}
тестов, {runs.compile_at_1.mean():.0%} с первой попытки. Проблема в оракуле, то есть в ожиданиях самого теста:
{n - valid} из {n} тестов падают на правильном коде, и {failed_by_model.get(MODELS[2], 0)} из них написал Haiku 4.5.
Три типичные ошибки из `t6_failures.csv`:

1. *Целочисленная арифметика* (`dc_motor_pot`, Haiku 4.5, RAG on). Тест ждёт
   `motor_power_from_reading(512) == 128`, а `map()` отбрасывает дробную часть 127.6 и даёт 127.
2. *Неверная модель состояния* (`led_chaser`, Sonnet 5, RAG on). Тест ждёт, что первый `chaser_step()` после
   `chaser_init()` зажжёт позицию 1, а модуль стартует с −1 и встаёт на 0.
3. *Артефакт симулятора* (`dc_motor_pot`, GPT-5.6 Terra, RAG off). Тест переводит вход потенциометра в
   OUTPUT HIGH и ждёт мощность 255 после `motor_update()`. На ATmega328P АЦП меряет напряжение на пине, и
   проверка, скорее всего, прошла бы, а в `avr8js` `analogRead` всегда возвращает 0. Это случай, обратный RQ2
   протокола диплома: тест падает в симуляторе, хотя на плате, вероятно, прошёл бы.

**RQ2. RAG.** Влияния не найдено ни по одной метрике. Валидные тесты, p после поправки Холма:
{sig(t["t2_rag_valid"])}. Mutation score:
{score_direction}; ни одно сравнение не значимо, и пересчёт без эквивалентных мутантов этого не меняет
(наименьшее p_holm = {t["t3_rag_score_adj"].p_holm.min()}). Направления к тому же расходятся: по валидным
тестам {valid_direction}. Это совпадает с ожиданием из H1–H3: исходник модуля целиком помещается в промпт, и база
знаний почти ничего не добавляет.

**RQ3. Модели.** GPT-5.6 Terra и Sonnet 5 дают валидный тест заметно чаще Haiku 4.5:
{int(valid_by_model[MODELS[1]])}/30 и {int(valid_by_model[MODELS[0]])}/30 против {int(valid_by_model[MODELS[2]])}/30
(p_holm = {pair(MODELS[1], MODELS[2]).p_valid_holm} и {pair(MODELS[0], MODELS[2]).p_valid_holm}). Между собой GPT
и Sonnet по валидности не различаются (p_holm = {pair(MODELS[0], MODELS[1]).p_valid_holm}), но тесты GPT убивают
больше мутантов: в несовпадающих парах {int(pair(MODELS[0], MODELS[1]).killed_b_only)} в пользу GPT против
{int(pair(MODELS[0], MODELS[1]).killed_a_only)} (p_holm = {pair(MODELS[0], MODELS[1]).p_mutants_holm}). При этом GPT
в {cost_by_model[MODELS[0]] / cost_by_model[MODELS[1]]:.1f} раза дешевле: ${cost_by_model[MODELS[1]]:.2f} за 30
прогонов против ${cost_by_model[MODELS[0]]:.2f} у Sonnet. Haiku дешевле всех (${cost_by_model[MODELS[2]]:.2f}), но
{30 - int(valid_by_model[MODELS[2]])} из 30 его тестов невалидны. Лучшее соотношение цены и качества у GPT-5.6 Terra.

**RQ4. Ловят ли тесты баги.** Валидные тесты убивают {killed_adj / total_adj:.0%} неэквивалентных мутантов (сырой
подсчёт {killed / total:.0%}). Легко ловятся перевёрнутые условия и арифметика: negate_cond {ops_adj["negate_cond"]:.0%},
swap_arith {ops_adj["swap_arith"]:.0%}, swap_hilo {ops_adj["swap_hilo"]:.0%}. Хуже всего ловятся изменённые
константы, сдвиги границ и удалённые вызовы: const_change {ops_adj["const_change"]:.0%}, off_by_one
{ops_adj["off_by_one"]:.0%}, drop_call {ops_adj["drop_call"]:.0%}; у swap_logic столько же, {ops_adj["swap_logic"]:.0%}, но
всего на {int(t["t7_operators_adj"].set_index("operator").runs["swap_logic"])} проверках. До разметки off_by_one выглядел самым трудным
оператором ({ops_raw["off_by_one"]:.0%}), но эта оценка в основном держалась на двух эквивалентных мутантах.

Из {len(t["t8_survivors"]) - kinds.get("equivalent", 0)} неэквивалентных выживших {kinds.get("test_gap", 0)} — пропуски
в самих тестах: не проверены пины после смены состояния (`dc_motor_pot #6`, `dual_hbridge #4`), функция с
аппаратным входом (`joystick_motor #8`), граничное значение (`led_breathing #2`), а одна проверка идёт через
собственный API модуля (`led_chaser #1`). Остальные {kinds.get("unobservable", 0) + kinds.get("needs_signal", 0)}
видны только при наблюдении за временем или сигналом. Три примера из `t8_survivors.csv`:

1. `led_chaser #1`: в массиве пинов 7 заменён на 14. Тесты считают горящие светодиоды через `chaser_count_on()`,
   а она читает тот же изменённый массив, так что проверка согласна сама с собой. Мутанта убил бы один
   `ASSERT_PIN(7, HIGH)` после `chaser_light_only(5)`.
2. `traffic_light #3`: из цикла мигания удалён `digitalWrite(GREEN, LOW)`, и зелёный горит постоянно вместо
   мигания. Итоговое состояние и длительность вызова те же, поэтому последовательный тест через serial-протокол
   этого не видит. Нужен наблюдатель сигнала во времени (логический анализатор или HIL-стенд). Это довод в пользу
   аппаратного уровня из протокола диплома.
3. `dc_motor_pot #9`: `raw > 1023` заменено на `raw > 1024`. Мутант эквивалентен: при raw = 1024 `map()` даёт
   те же 255, поэтому он исключён из пересчёта.

**Итог.** На простых модулях агент с недорогой сильной моделью (GPT-5.6 Terra) пишет валидный тест в
{valid_by_model[MODELS[1]] / 30:.0%} случаев, и эти тесты убивают в среднем
{runs_adj[runs_adj.model == MODELS[1]].mutation_score.mean():.0%} неэквивалентных мутантов. RAG при исходнике в промпте
ничего не даёт. Около половины оставшихся пропусков лежит за пределами того, что видит последовательный тест:
время, ширина импульса, поведение внутри блокирующего вызова.
"""


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--results", default=str(ROOT / "results" / "main_v3"))
    ap.add_argument("--out", default=str(ROOT / "analysis"))
    args = ap.parse_args()
    results_dir, out = Path(args.results), Path(args.out)
    (out / "tables").mkdir(parents=True, exist_ok=True)
    (out / "figures").mkdir(parents=True, exist_ok=True)

    runs = pd.read_parquet(results_dir / "results.parquet")
    runs = runs[runs.error.isna()].assign(valid=lambda d: d.exec_verdict.eq("pass"))
    mutants = pd.read_parquet(results_dir / "mutants.parquet")
    mutants = mutants[mutants.killed.notna()].assign(killed=lambda d: d.killed.astype(bool))
    if len(runs) != 90:
        print(f"warning: {len(runs)} runs without errors (expected 90)")

    valid_t, score_t, mutant_t = rag_effect(runs, mutants)
    runs_adj, mutants_adj = without_equivalent(runs, mutants)
    _, score_adj, mutant_adj = rag_effect(runs_adj, mutants_adj)
    tables = {"t1_overview": overview(runs), "t2_rag_valid": valid_t, "t3_rag_score": score_t,
              "t4_rag_mutants": mutant_t, "t5_models": model_comparison(runs, mutants),
              "t6_failures": failures(runs, results_dir), "t7_operators": operators(mutants),
              "t8_survivors": survivors(mutants), "t3_rag_score_adj": score_adj,
              "t4_rag_mutants_adj": mutant_adj, "t7_operators_adj": operators(mutants_adj)}
    unreviewed = set(zip(tables["t8_survivors"].module_id, tables["t8_survivors"].mutant_id)) ^ set(SURVIVOR_REVIEW)
    if unreviewed:
        print(f"warning: SURVIVOR_REVIEW does not match t8_survivors: {sorted(unreviewed)}")
    for name, df in tables.items():
        df.to_csv(out / "tables" / f"{name}.csv", index=False, encoding="utf-8")

    fig_rates(runs, out / "figures" / "f1_rates.png")
    fig_pass_slope(runs, out / "figures" / "f2_pass_slope.png")
    fig_score_box(runs, out / "figures" / "f3_score_box.png")
    fig_operators(tables["t7_operators"], tables["t7_operators_adj"], out / "figures" / "f4_operators.png")
    fig_cost(tables["t1_overview"], out / "figures" / "f5_cost_vs_pass.png")
    fig_similarity(runs, out / "figures" / "f6_rag_similarity.png")

    (out / "REPORT.md").write_text(report(runs, mutants, runs_adj, mutants_adj, tables, results_dir), encoding="utf-8")
    print(f"tables: {len(tables)} -> {out / 'tables'}\nfigures: 6 -> {out / 'figures'}\nreport: {out / 'REPORT.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
