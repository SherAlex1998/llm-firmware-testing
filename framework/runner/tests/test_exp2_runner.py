import json

import pandas as pd
import pytest

from framework.harness.tests.conftest import avr8js, requires_arduino  # noqa: F401 (pytest fixture)
from framework.runner.experiment import Condition, RetryOnce, module_facts, plan_exp2, run_one
from framework.runner.tests.test_runner import GOOD, ScriptedLLM


def test_plan_exp2_blocks_by_rep():
    conds = [Condition("off", "a"), Condition("on", "a")]
    jobs = plan_exp2(["m1", "m2", "m3"], conds, 3, 42)
    assert jobs == plan_exp2(["m1", "m2", "m3"], conds, 3, 42) and len(jobs) == 18
    assert [r for _, _, r in jobs] == [1] * 6 + [2] * 6 + [3] * 6
    assert [(m, c) for m, c, r in jobs if r == 1] != [(m, c) for m, c, r in jobs if r == 2]   # own order per rep
    assert sorted({r for _, _, r in plan_exp2(["m1"], conds, 3, 42, block_by_rep=False)}) == [1, 2, 3]


def test_module_facts_h5_groups():
    assert module_facts("ultrasonic_parking")["uses_libs"] is True          # NewPing, NeoPixel, Button
    assert module_facts("lcd_kitchen_timer")["uses_libs"] is False          # only the official LiquidCrystal
    f = module_facts("ir_wakeup_light")
    assert f["complexity"] == "complex" and f["has_devices"] and "IRremote@2.8.0" in f["libraries"]


@requires_arduino
@pytest.mark.arduino
@pytest.mark.avr8js
def test_mutation_skipped_outside_mutation_reps(avr8js, tmp_path):
    def no_mutants(module_id):
        pytest.fail("mutation must not run for a rep outside mutation.reps")
    rec = run_one("blink_fixture", Condition("off", "fake"), tmp_path, ScriptedLLM([GOOD]), RetryOnce(avr8js),
                  mutants_for=no_mutants, rep=2, exp2={"kb_version": "k", "mutation_reps": [1]})
    assert rec["exec_verdict"] == "pass" and rec["mutation"] is None
    assert rec["mutation_skipped"] is True and (rec["experiment"], rec["rep"], rec["kb_version"]) == (2, 2, "k")
    assert "__r2__" in rec["run_id"]


def _record(tmp, module, rag, rep, complexity, score):
    from framework.mutation.catalog import load_catalog
    run_id = f"{module}__{rag}__fake__r{rep}__20261012-000000"
    run = tmp / "runs" / run_id
    run.mkdir(parents=True)
    rec = {"event": "run", "run_id": run_id, "module_id": module, "condition": {"rag": rag, "model": "fake"},
           "ts_start": "2026-10-12T00:00:00+00:00", "ts_end": None, "gen_attempts": 1, "tokens_in": 10, "tokens_out": 5,
           "cost_usd": 0.01, "cost_spent_usd": 0.01, "compile_ok": True, "exec_backend": "avr8js", "exec_verdict": "pass",
           "tests_total": 4, "tests_passed": 4, "error": None, "artifacts": [], "experiment": 2, "rep": rep,
           "complexity": complexity, "uses_libs": False, "libraries": ["LiquidCrystal@1.0.7"], "mutation_skipped": False,
           "mutation": {"mutants_total": 2, "killed": int(score * 2), "score": score}}
    m = load_catalog(module)[0]
    (run / "mutants.json").write_text(json.dumps({"per_mutant": [
        {"id": m.id, "operator": m.site.operator, "line": m.site.line, "before": m.before, "after": m.after,
         "verdict": "fail", "killed": True, "file": m.file}]}), encoding="utf-8")
    (run / "run.jsonl").write_text(json.dumps(rec) + "\n", encoding="utf-8")


def test_aggregate_exp2(tmp_path):
    from framework.runner.aggregate import aggregate
    _record(tmp_path, "lcd_kitchen_timer", "off", 1, "medium", 0.5)
    _record(tmp_path, "lcd_kitchen_timer", "off", 2, "medium", 1.0)
    _record(tmp_path, "lcd_pomoduino", "on", 1, "complex", 0.0)
    df = aggregate(tmp_path, exp2=True)
    assert len(df) == 3 and df["rep"].tolist() == [1, 2, 1] and df["libraries"].iloc[0] == "LiquidCrystal@1.0.7"
    mdf = pd.read_parquet(tmp_path / "mutants.parquet")
    assert {"rep", "complexity", "file", "trivial", "equivalent"} <= set(mdf.columns)
    assert mdf.loc[mdf["module_id"] == "lcd_pomoduino", "trivial"].iloc[0] is not None     # avr8js: smoke-marked
    text = (tmp_path / "SUMMARY.md").read_text(encoding="utf-8")
    assert "## medium (1 modules)" in text and "## complex (1 modules)" in text and "reps [1, 2]" in text


@requires_arduino
@pytest.mark.arduino
@pytest.mark.avr8js
def test_pruned_mutation_for_failed_test(avr8js, tmp_path):
    from framework.runner.tests.test_runner import few_mutants
    wrong = GOOD.replace("void setup()", "TEST(always_wrong) { ASSERT_EQ(1, 2); }\nvoid setup()", 1)
    rec = run_one("blink_fixture", Condition("off", "fake"), tmp_path, ScriptedLLM([wrong]), RetryOnce(avr8js),
                  mutants_for=few_mutants, rep=1, exp2={"mutation_reps": [1], "pruned": True, "max_tokens": 32000})
    assert rec["exec_verdict"] == "fail" and rec["mutation"] is None and rec["max_tokens"] == 32000
    pm = rec["mutation_pruned"]
    assert pm["status"] == "pass" and pm["disabled"] == ["always_wrong"] and pm["tests_kept"] == 6
    assert (pm["killed"], pm["mutants_total"]) == (3, 3)
    assert {"test_pruned.ino", "mutants_pruned.json"} <= set(rec["artifacts"])


@requires_arduino
@pytest.mark.arduino
@pytest.mark.avr8js
def test_pruned_equals_strict_for_passing_test(avr8js, tmp_path):
    from framework.runner.tests.test_runner import few_mutants
    rec = run_one("blink_fixture", Condition("off", "fake"), tmp_path, ScriptedLLM([GOOD]), RetryOnce(avr8js),
                  mutants_for=few_mutants, rep=1, exp2={"mutation_reps": [1], "pruned": True})
    assert rec["mutation_pruned"] == {**rec["mutation"], "status": "pass", "disabled": [], "tests_kept": 6}
