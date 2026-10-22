"""avr8js vs Wokwi verdict agreement on modules that run on avr8js.

python -m framework.harness.crosscheck                          # all avr8js modules of experiment 2 (smoke tests)
python -m framework.harness.crosscheck --v1                     # + experiment 1: one main_v3 test per module and verdict
python -m framework.harness.crosscheck --modules counter_fixture --out results/exp2/crosscheck.csv

Every row costs one Wokwi run (logged to <out dir>/wokwi_usage.csv, kind=crosscheck).
"""
from __future__ import annotations

import argparse
import csv
import sys
import tempfile
from pathlib import Path

from framework.common.config import load_config
from framework.harness.backends import get_backend
from framework.harness.compile import compile_sketch
from framework.harness.run_wokwi import WokwiBackend
from framework.harness.sketch import REPO_ROOT, load_module_yaml, resolve_module_dir, stage_test_sketch

FIELDS = ["module_id", "test_source", "avr8js_verdict", "wokwi_verdict", "avr8js_passed", "wokwi_passed", "total", "match"]


def exp2_modules(cfg: dict) -> list[str]:
    return [m for m in (cfg.get("experiment2") or {}).get("modules") or []
            if load_module_yaml(resolve_module_dir(m)).get("backend", "avr8js") == "avr8js"]


def v1_tests(results: Path) -> list[tuple[str, Path]]:
    """One test per (module, verdict) from experiment 1, so both pass and fail outcomes get compared."""
    import pandas as pd
    df = pd.read_parquet(results / "results.parquet").sort_values("run_id")
    picked = df[df.compile_ok].drop_duplicates(["module_id", "exec_verdict"])
    return [(r.module_id, results / "runs" / r.run_id / "test_main.ino") for r in picked.itertuples()]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--modules", nargs="*", help="module ids (default: experiment2.modules with backend avr8js)")
    ap.add_argument("--v1", action="store_true", help="also experiment 1 tests from results/main_v3")
    ap.add_argument("--out", default="results/exp2/crosscheck.csv")
    args = ap.parse_args()

    cfg = load_config()
    out = REPO_ROOT / args.out
    jobs = [(m, resolve_module_dir(m) / "tests" / "smoke_test.ino") for m in (args.modules or exp2_modules(cfg))]
    if args.v1:
        jobs += v1_tests(REPO_ROOT / "results" / "main_v3")
    avr = get_backend("avr8js", cfg)
    wokwi = WokwiBackend(cfg, usage_log=out.parent / "wokwi_usage.csv")
    for backend in (avr, wokwi):
        ok, reason = backend.check()
        if not ok:
            print(f"{backend.name} unavailable: {reason}")
            return 3
    timeout = cfg["serial"]["timeout_s"]
    rows, work = [], Path(tempfile.mkdtemp(prefix="crosscheck_"))
    for i, (module, test) in enumerate(jobs):
        if not test.exists():
            print(f"skip {module}: no {test.name}")
            continue
        sketch = stage_test_sketch(resolve_module_dir(module), test.read_text(encoding="utf-8"), work / str(i))
        comp = compile_sketch(sketch, work / str(i) / "build")
        if not comp.ok:
            print(f"skip {module}: {test} does not compile")
            continue
        a = avr.run(comp.hex_path, timeout)
        wokwi.context = {"run_id": f"crosscheck_{i}", "kind": "crosscheck", "module_id": module}
        w = wokwi.run(comp.hex_path, timeout)
        match = a.verdict == w.verdict and a.tests_passed == w.tests_passed
        rows.append({"module_id": module, "test_source": str(test.relative_to(REPO_ROOT)).replace("\\", "/"),
                     "avr8js_verdict": a.verdict, "wokwi_verdict": w.verdict, "avr8js_passed": a.tests_passed,
                     "wokwi_passed": w.tests_passed, "total": a.tests_total, "match": match})
        print(f"{module:28s} avr8js={a.verdict}:{a.tests_passed}/{a.tests_total} "
              f"wokwi={w.verdict}:{w.tests_passed}/{w.tests_total} {'ok' if match else 'MISMATCH'}")
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(fh, fieldnames=FIELDS)
        writer.writeheader()
        writer.writerows(rows)
    n = sum(r["match"] for r in rows)
    print(f"agreement: {n}/{len(rows)} -> {out}")
    return 0 if n == len(rows) else 1


if __name__ == "__main__":
    sys.exit(main())
