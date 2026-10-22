"""Collect run records into tables for the analysis.

    python -m framework.runner.aggregate            # results/runs -> results/results.parquet, mutants.parquet, SUMMARY.md
    python -m framework.runner.aggregate --pilot    # results/pilot/...
    python -m framework.runner.aggregate --exp2 [--pilot]   # results/exp2[/pilot]: one row per (module, rag, model, rep)

results.parquet: one row per (module, rag, model) - the newest run, preferring runs without error.
mutants.parquet: one row per (run, mutant), the input of the per-mutant McNemar test.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import pandas as pd

from framework.common.config import load_config, resolve_path
from framework.runner.experiment import benchmark_modules, conditions, latest_records

CONTRACT = ["run_id", "module_id", "condition", "ts_start", "ts_end", "gen_attempts", "tokens_in", "tokens_out",
            "cost_usd", "compile_ok", "exec_backend", "exec_verdict", "tests_total", "tests_passed", "mutation",
            "artifacts"]


def runs_frame(records: list[dict]) -> pd.DataFrame:
    rows = []
    for r in records:
        m, rag = r.get("mutation") or {}, r.get("rag") or {}
        rows.append({**{k: ";".join(map(str, v)) if isinstance(v, list) else v for k, v in r.items()
                        if k not in ("condition", "mutation", "mutation_pruned", "rag", "artifacts", "event", "ts")},
                     "rag": r["condition"]["rag"], "model": r["condition"]["model"],
                     "compile_at_1": bool(r["compile_ok"] and r["gen_attempts"] == 1),
                     "mutants_total": m.get("mutants_total"), "mutants_killed": m.get("killed"),
                     "mutation_score": m.get("score"), "rag_similarity_mean": rag.get("similarity_mean"),
                     "rag_similarity_max": rag.get("similarity_max"),
                     "rag_sources": ";".join(rag.get("sources", [])) or None,
                     "artifacts": ";".join(r.get("artifacts", []))})
        if r.get("experiment") == 2:                          # secondary metrics of experiment 2
            pm = r.get("mutation_pruned") or {}
            rows[-1].update(tests_pass_share=r["tests_passed"] / r["tests_total"] if r["tests_total"] else None,
                            pruned_status=pm.get("status"), pruned_disabled=len(pm.get("disabled") or []) if pm else None,
                            pruned_tests_kept=pm.get("tests_kept"), pruned_mutants_total=pm.get("mutants_total"),
                            pruned_killed=pm.get("killed"), pruned_score=pm.get("score"))
    return pd.DataFrame(rows)


def pruned_mutants_frame(runs_dir: Path, records: list[dict], catalog: Path) -> pd.DataFrame:
    """Experiment 2: per-mutant verdicts of the pruned tests (mutants.json when nothing had to be disabled)."""
    keep = []
    for r in records:
        pm = r.get("mutation_pruned") or {}
        if pm.get("status") == "pass" and pm.get("mutants_total"):
            name = "mutants_pruned.json" if pm.get("disabled") else "mutants.json"
            keep.append(({**r, "mutation": pm}, name))
    frames = [mutants_frame(runs_dir, [r], catalog, name) for r, name in keep]
    return pd.concat(frames, ignore_index=True) if frames else pd.DataFrame()


def mutants_frame(runs_dir: Path, records: list[dict], catalog: Path | None = None,
                  file_name: str = "mutants.json") -> pd.DataFrame:
    """catalog (experiment 2): adds rep, file and the catalog's trivial / equivalent marks."""
    rows, marks = [], {}
    for r in records:
        path = Path(runs_dir) / r["run_id"] / file_name
        if r.get("mutation") and path.exists():
            for pm in json.loads(path.read_text(encoding="utf-8"))["per_mutant"]:
                row = {"run_id": r["run_id"], "module_id": r["module_id"], "rag": r["condition"]["rag"],
                       "model": r["condition"]["model"], "mutant_id": pm["id"], "operator": pm["operator"],
                       "line": pm["line"], "before": pm["before"], "after": pm["after"],
                       "verdict": pm["verdict"], "killed": pm["killed"]}
                if catalog is not None:
                    if r["module_id"] not in marks:
                        cat = json.loads((Path(catalog) / r["module_id"] / "catalog.json").read_text(encoding="utf-8"))
                        marks[r["module_id"]] = {m["id"]: m for m in cat["mutants"]}
                    mark = marks[r["module_id"]].get(pm["id"], {})
                    row.update(rep=r.get("rep"), complexity=r.get("complexity"), file=pm.get("file", "logic.cpp"),
                               trivial=mark.get("trivial"), equivalent=mark.get("equivalent"))
                rows.append(row)
    return pd.DataFrame(rows)


def summary(df: pd.DataFrame) -> str:
    lines = ["# Experiment summary", "", f"Runs: {len(df)}; with errors: {int(df['error'].notna().sum())}; "
             f"total cost ${df['cost_usd'].sum():.2f} (spent now ${df['cost_spent_usd'].sum():.2f}); "
             f"tokens in/out {int(df['tokens_in'].sum())}/{int(df['tokens_out'].sum())}.", ""] + condition_table(df)
    lines += ["", "compile@1: compiled without repair; compile@k: compiled after at most "
              f"{load_config()['generation']['max_repair_attempts']} repairs; pass: every test passed on the original "
              "module; mutation score: mean over runs that passed.", ""]
    return "\n".join(lines)


def condition_table(df: pd.DataFrame) -> list[str]:
    lines = ["| model | rag | runs | compile@1 | compile@k | pass | mean tests | mutation score | cost $ |",
             "|---|---|---|---|---|---|---|---|---|"]
    for (model, rag), g in df.groupby(["model", "rag"], sort=True):
        score = g["mutation_score"].dropna()
        score_text = f"{score.mean():.2f} (n={len(score)})" if len(score) else "-"
        lines.append(f"| {model} | {rag} | {len(g)} | {g['compile_at_1'].mean():.2f} | {g['compile_ok'].mean():.2f} "
                     f"| {(g['exec_verdict'] == 'pass').mean():.2f} | {g['tests_total'].mean():.1f} | {score_text} "
                     f"| {g['cost_usd'].sum():.3f} |")
    return lines


def summary_exp2(df: pd.DataFrame, results_root: Path) -> str:
    """Experiment 2: the experiment 1 table per tier, plus repetitions, mutation skips and Wokwi usage."""
    from framework.harness.run_wokwi import wokwi_usage_summary
    lines = ["# Experiment 2 summary", "", f"Runs: {len(df)} ({df['module_id'].nunique()} modules, reps "
             f"{sorted(df['rep'].dropna().astype(int).unique().tolist())}); with errors: {int(df['error'].notna().sum())}; "
             f"total cost ${df['cost_usd'].sum():.2f} (spent now ${df['cost_spent_usd'].sum():.2f}); "
             f"mutation skipped (rep outside mutation.reps): {int(df['mutation_skipped'].fillna(False).sum())}.", ""]
    for tier, g in df.groupby("complexity", sort=False):
        lines += [f"## {tier} ({g['module_id'].nunique()} modules)", ""] + condition_table(g) + [""]
    lines += ["## Secondary metrics", "", "| tier | model | rag | share of correct tests | pruned mutation score |",
              "|---|---|---|---|---|"]
    for (tier, model, rag), g in df.groupby(["complexity", "model", "rag"], sort=False):
        share, ps = g["tests_pass_share"].dropna(), g["pruned_score"].dropna()
        lines.append(f"| {tier} | {model} | {rag} | {f'{share.mean():.2f}' if len(share) else '-'} | "
                     f"{f'{ps.mean():.2f} (n={len(ps)})' if len(ps) else '-'} |")
    lines += ["", "Share of correct tests: tests_passed / tests_total of compiled runs. Pruned mutation score: the "
              "tests that failed on the original are disabled, the rest must pass, then the same 10 mutants run "
              "(equals the mutation score when the test passed).", ""]
    w = wokwi_usage_summary(Path(results_root) / "wokwi_usage.csv")
    kinds = ", ".join(f"{k} {v['runs']} ({v['wall_s']:.0f} s)" for k, v in sorted(w["per_kind"].items()))
    lines += ["## Wokwi", "", f"Logged runs {w['runs']}, wall {w['wall_s']:.0f} s, retries {w['retries']}, "
              f"cloud errors {w['errors']}; by kind: {kinds or '-'}.", "",
              "compile@1 / compile@k / pass / mutation score as in experiment 1; one row of results.parquet per "
              "(module, rag, model, rep).", ""]
    return "\n".join(lines)


def aggregate(results_root: Path, expected_modules: list[str] | None = None, exp2: bool = False) -> pd.DataFrame:
    results_root = Path(results_root)
    records = list(latest_records(results_root / "runs").values())
    if not records:
        raise SystemExit(f"no run records under {results_root / 'runs'}")
    for r in records:
        missing = [k for k in CONTRACT if k not in r]
        if missing:
            print(f"warning: {r['run_id']} lacks {missing}")
    if expected_modules is not None:
        cfg = load_config()
        reps = range(1, int(cfg["experiment2"]["reps"]) + 1) if exp2 else [None]
        have = {(r["module_id"], r["condition"]["rag"], r["condition"]["model"], r.get("rep")) for r in records}
        want = {(m, c.rag, c.model, k) for m in expected_modules for c in conditions(cfg) for k in reps}
        for key in sorted(want - have, key=str):
            print(f"warning: no run for {key}")
    df = runs_frame(records).sort_values(["module_id", "model", "rag"] + (["rep"] if exp2 else [])).reset_index(drop=True)
    df.to_parquet(results_root / "results.parquet", index=False)
    df.to_csv(results_root / "results.csv", index=False)
    catalog = None
    if exp2:
        from framework.mutation.catalog import catalog_root
        catalog = catalog_root()
    mdf = mutants_frame(results_root / "runs", records, catalog)
    if not mdf.empty:
        mdf.to_parquet(results_root / "mutants.parquet", index=False)
        mdf.to_csv(results_root / "mutants.csv", index=False)
    if exp2:
        pdf = pruned_mutants_frame(results_root / "runs", records, catalog)
        if not pdf.empty:
            pdf.to_parquet(results_root / "mutants_pruned.parquet", index=False)
            pdf.to_csv(results_root / "mutants_pruned.csv", index=False)
    (results_root / "SUMMARY.md").write_text(summary_exp2(df, results_root) if exp2 else summary(df), encoding="utf-8")
    return df


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--pilot", action="store_true")
    ap.add_argument("--results-dir")
    ap.add_argument("--exp2", action="store_true", help="experiment 2 (experiment2.results_dir, reps, tiers)")
    args = ap.parse_args()
    cfg = load_config()
    root = resolve_path(args.results_dir or (cfg["experiment2"] if args.exp2 else cfg["experiment"])["results_dir"])
    root = root / "pilot" if args.pilot and not args.results_dir else root
    expected = None if (args.pilot or args.results_dir) else cfg["experiment2"]["modules"] if args.exp2 else benchmark_modules()
    df = aggregate(root, expected, exp2=args.exp2)
    print(f"{len(df)} runs -> {root / 'results.parquet'}")
    print((root / "SUMMARY.md").read_text(encoding="utf-8"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
