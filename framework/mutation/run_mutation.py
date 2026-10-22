"""Mutation score of one test sketch.

    python -m framework.mutation.run_mutation --module led_chaser --test-file my_test.ino [--out dir]
"""
from __future__ import annotations

import argparse
import json
import re
import sys
import tempfile
from dataclasses import asdict, dataclass, field
from pathlib import Path

from framework.common.config import load_config
from framework.common.protocol import ERROR, FAIL, PASS, TIMEOUT
from framework.harness.backends import get_backend
from framework.harness.compile import compile_sketch
from framework.harness.sketch import resolve_module_dir, stage_test_sketch
from framework.mutation.mutator import Mutant, make_mutants


@dataclass
class MutantResult:
    id: int
    operator: str
    line: int
    before: str
    after: str
    verdict: str
    killed: bool | None       # None: excluded (infrastructure error)
    file: str = "logic.cpp"   # mutated source file


@dataclass
class MutationResult:
    mutants_total: int        # mutants with a verdict (errors excluded)
    killed: int
    score: float | None       # killed / mutants_total; None when nothing could be run
    per_mutant: list[MutantResult] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)


def mutation_score(module: str | Path, test_source: str, mutants: list[Mutant], backend=None,
                   timeout_s: int | None = None, work_dir: Path | None = None) -> MutationResult:
    cfg = load_config()
    module_dir = resolve_module_dir(module)
    backend = backend or get_backend(cfg=cfg, module=module_dir)
    timeout_s = timeout_s or cfg["serial"]["timeout_s"]
    work = Path(work_dir or tempfile.mkdtemp(prefix="mutrun_"))
    per, warnings = [], []
    for m in mutants:
        sketch = stage_test_sketch(module_dir, test_source, work / f"mutant_{m.id}", overrides={m.file: m.source})
        comp = compile_sketch(sketch, work / f"mutant_{m.id}" / "build")
        verdict = backend.run(comp.hex_path, timeout_s).verdict if comp.ok else ERROR
        killed = {FAIL: True, TIMEOUT: True, PASS: False}.get(verdict)
        if killed is None:
            warnings.append(f"mutant {m.id} ({m.site.operator}, line {m.site.line}): {verdict}, excluded")
        per.append(MutantResult(m.id, m.site.operator, m.site.line, m.before, m.after, verdict, killed, m.file))
    total = sum(r.killed is not None for r in per)
    killed = sum(bool(r.killed) for r in per)
    return MutationResult(total, killed, killed / total if total else None, per, warnings)


_TEST_DEF = r"\bTEST\s*\(\s*{name}\s*\)"
DISABLED_MACRO = "#define TEST_DISABLED_(name) static void __attribute__((unused)) test_off_##name()\n"


def test_names(test_source: str) -> list[str]:
    return re.findall(_TEST_DEF.format(name=r"(\w+)"), test_source)


def disable_tests(test_source: str, names: set[str]) -> str:
    """The test sketch with these TEST(name) blocks kept as plain unused functions (not registered, not run)."""
    for name in names:
        test_source = re.sub(_TEST_DEF.format(name=re.escape(name)), f"TEST_DISABLED_({name})", test_source)
    return DISABLED_MACRO + test_source


@dataclass
class PrunedResult:
    """Mutation score of a test after disabling the checks that failed on the original module."""
    status: str               # pass: pruned test passes, mutants run | no_tests_left | <verdict> of the last check
    disabled: list[str]
    tests_kept: int
    mutation: MutationResult | None = None
    source: str = ""          # the pruned test sketch


def pruned_mutation_score(module: str | Path, test_source: str, failed: list[str], mutants: list[Mutant], backend,
                          timeout_s: int | None = None, work_dir: Path | None = None,
                          max_rounds: int = 2) -> PrunedResult:
    """Disable the failed tests and re-run the rest on the original, since a remaining test may depend on a
    disabled one. Mutants are run once the rest passes."""
    timeout_s = timeout_s or load_config()["serial"]["timeout_s"]
    module_dir = resolve_module_dir(module)
    work = Path(work_dir or tempfile.mkdtemp(prefix="pruned_"))
    disabled, names = set(failed), test_names(test_source)
    status, source = "fail", test_source
    for k in range(max_rounds):
        kept = [n for n in names if n not in disabled]
        source = disable_tests(test_source, disabled)
        if not kept:
            return PrunedResult("no_tests_left", sorted(disabled), 0, None, source)
        comp = compile_sketch(stage_test_sketch(module_dir, source, work / f"check_{k}"), work / f"check_{k}" / "build")
        if not comp.ok:
            return PrunedResult(ERROR, sorted(disabled), len(kept), None, source)
        ex = backend.run(comp.hex_path, timeout_s)
        status = ex.verdict
        if status == PASS:
            res = mutation_score(module_dir, source, mutants, backend, timeout_s, work / "mutants")
            return PrunedResult(PASS, sorted(disabled), len(kept), res, source)
        if status != FAIL:
            break
        disabled |= {t.name for t in ex.tests if not t.passed}
    return PrunedResult(status, sorted(disabled), len([n for n in names if n not in disabled]), None, source)


def save_mutation(result: MutationResult, mutants: list[Mutant], out_dir: Path) -> None:
    """Write mutants/<id>/<file> and mutants.json."""
    out = Path(out_dir)
    for m in mutants:
        (out / "mutants" / str(m.id)).mkdir(parents=True, exist_ok=True)
        (out / "mutants" / str(m.id) / m.file).write_text(m.source, encoding="utf-8")
    (out / "mutants.json").write_text(json.dumps(asdict(result), ensure_ascii=False, indent=1), encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--module", required=True, help="benchmark id, fixture id or module folder")
    ap.add_argument("--test-file", help="test sketch (default: <module>/tests/test_main.ino)")
    ap.add_argument("--out", help="write mutants/ and mutants.json here")
    ap.add_argument("--backend", help="wokwi | avr8js | board (default from configs/experiment.yaml)")
    args = ap.parse_args()

    module_dir = resolve_module_dir(args.module)
    test_source = Path(args.test_file or module_dir / "tests" / "test_main.ino").read_text(encoding="utf-8")
    work = Path(tempfile.mkdtemp(prefix="mutation_"))
    backend = get_backend(args.backend)
    ok, reason = backend.check()
    if not ok:
        print(f"backend {backend.name} unavailable: {reason}")
        return 2

    comp = compile_sketch(stage_test_sketch(module_dir, test_source, work / "original"), work / "original" / "build")
    base = backend.run(comp.hex_path, load_config()["serial"]["timeout_s"]) if comp.ok else None
    if base is None or base.verdict != PASS:
        print(f"test does not pass on the original module ({base.verdict if base else 'compile error'}); "
              f"mutation score is undefined")
        return 1

    mutants = make_mutants(module_dir, work_dir=work / "candidates")
    res = mutation_score(module_dir, test_source, mutants, backend, work_dir=work)
    for r in res.per_mutant:
        mark = {True: "KILLED  ", False: "survived", None: "error   "}[r.killed]
        print(f"#{r.id} {mark} {r.operator:12s} line {r.line:3d}: {r.before}  ->  {r.after}")
    for w in res.warnings:
        print("warning:", w)
    score = "n/a" if res.score is None else f"{res.score:.2f}"
    print(f"mutation score: {res.killed}/{res.mutants_total} = {score}")
    if args.out:
        save_mutation(res, mutants, Path(args.out))
        print(f"saved -> {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
