"""English copies of the analysis figures f1, f3 and f4.

    python analysis/figures_en.py          # -> analysis/figures_en/*.png

Draws with the functions of analyze.py and translates every text of a figure just before it is saved, so the charts
match analysis/figures/ in everything but the language. analysis/figures/ and the tables are not touched.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

import matplotlib.figure
import pandas as pd
from matplotlib.text import Text

sys.path.insert(0, str(Path(__file__).parent))
import analyze  # noqa: E402

OUT = analyze.ROOT / "analysis" / "figures_en"
CYRILLIC = re.compile("[а-яёА-ЯЁ]")
EN = {
    "Доля валидных тестов": "Share of valid tests",
    "Тесты, прошедшие на исходном коде (15 модулей на условие)": "Tests that passed on the original code "
                                                                 "(15 modules per condition)",
    "Доля пойманных мутантов, по прогонам с валидным тестом": "Share of killed mutants, runs with a valid test",
    "Доля выживших мутантов": "Share of surviving mutants",
    "Какие внесённые баги тесты пропускают чаще всего": "Which injected bugs the tests miss most often",
    "все мутанты": "all mutants",
    "без эквивалентных": "without equivalent ones",
}

_savefig = matplotlib.figure.Figure.savefig


def savefig_en(fig, *args, **kwargs):
    for text in fig.findobj(Text):
        source = text.get_text()
        text.set_text(EN.get(source, source))
        if CYRILLIC.search(text.get_text()):
            raise ValueError(f"untranslated text in {args[0]}: {text.get_text()!r}")
    _savefig(fig, *args, **kwargs)


def main() -> int:
    results_dir = analyze.ROOT / "results" / "main_v3"
    runs = pd.read_parquet(results_dir / "results.parquet")
    runs = runs[runs.error.isna()].assign(valid=lambda d: d.exec_verdict.eq("pass"))
    mutants = pd.read_parquet(results_dir / "mutants.parquet")
    mutants = mutants[mutants.killed.notna()].assign(killed=lambda d: d.killed.astype(bool))
    _, mutants_adj = analyze.without_equivalent(runs, mutants)

    OUT.mkdir(parents=True, exist_ok=True)
    matplotlib.figure.Figure.savefig = savefig_en
    try:
        analyze.fig_rates(runs, OUT / "f1_rates.png")
        analyze.fig_score_box(runs, OUT / "f3_score_box.png")
        analyze.fig_operators(analyze.operators(mutants), analyze.operators(mutants_adj), OUT / "f4_operators.png")
    finally:
        matplotlib.figure.Figure.savefig = _savefig
    print(f"figures: 3 -> {OUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
