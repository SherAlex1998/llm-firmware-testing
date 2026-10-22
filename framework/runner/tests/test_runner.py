import json
from types import SimpleNamespace as NS

import pandas as pd
import pytest

from framework.generator.llm_clients import LLMClient, LLMResponse
from framework.harness.tests.conftest import FIXTURE_TESTS, avr8js, requires_arduino  # noqa: F401 (pytest fixture)
from framework.rag.retrieve import RagResult
from framework.runner.aggregate import aggregate
from framework.runner.experiment import (Condition, RetryOnce, conditions, latest_records, mutation_only, plan_runs,
                                         run_matrix, run_one)

GOOD = "```cpp\n" + (FIXTURE_TESTS / "test_main.ino").read_text(encoding="utf-8") + "```"
BAD = '```cpp\n#include "logic.h"\nTEST(t) { led_onn(); }\nvoid setup() {}\nvoid loop() {}\n```'


class ScriptedLLM(LLMClient):
    provider = "fake"

    def __init__(self, replies, model_id="fake"):
        super().__init__({"id": model_id, "price_in_per_mtok": 1.0, "price_out_per_mtok": 2.0})
        self.replies, self.prompts = list(replies), []

    def _call(self, system, user):
        self.prompts.append(user)
        reply = self.replies.pop(0)
        if isinstance(reply, Exception):
            raise reply
        return LLMResponse(reply, 1000, 500, self.model_id, 0.1, "end_turn")


def fake_retrieve(module_id):
    return RagResult("q", "REFERENCE-TEXT", [{"id": "a", "title": "t", "source_path": "kb/guides/x.md",
                                              "kind": "guide", "similarity": 0.6, "text": "REFERENCE-TEXT"}])


def few_mutants(module_id):
    from framework.mutation.mutator import make_mutants
    return tuple(make_mutants(module_id, 3))


def test_conditions_read_yaml_booleans():
    cfg = {"experiment": {"conditions": {"rag": [False, True]}}, "models": [{"id": "a"}, {"id": "b"}]}
    assert conditions(cfg) == [Condition("off", "a"), Condition("on", "a"), Condition("off", "b"), Condition("on", "b")]


def test_plan_is_shuffled_deterministically():
    conds = [Condition("off", "a"), Condition("on", "a")]
    a, b = plan_runs(["m1", "m2", "m3"], conds, 42), plan_runs(["m1", "m2", "m3"], conds, 42)
    assert a == b and len(a) == 6 and a != [(m, c) for m in ["m1", "m2", "m3"] for c in conds]


def test_retry_once_on_error():
    calls = []

    class Flaky:
        name = "flaky"

        def run(self, fw, t):
            calls.append(1)
            return NS(verdict="error" if len(calls) == 1 else "pass")
    assert RetryOnce(Flaky()).run("x", 1).verdict == "pass" and len(calls) == 2


@requires_arduino
@pytest.mark.arduino
@pytest.mark.avr8js
def test_run_one_full_pipeline(avr8js, tmp_path):
    llm = ScriptedLLM([BAD, GOOD])
    rec = run_one("blink_fixture", Condition("on", "fake"), tmp_path, llm, RetryOnce(avr8js),
                  retrieve=fake_retrieve, mutants_for=few_mutants)
    assert rec["error"] is None and rec["compile_ok"] and rec["gen_attempts"] == 2
    assert (rec["exec_verdict"], rec["tests_passed"], rec["tests_total"]) == ("pass", 6, 6)
    assert rec["mutation"] == {"mutants_total": 3, "killed": 3, "score": 1.0}
    assert rec["rag"]["n_chunks"] == 1 and "REFERENCE-TEXT" in llm.prompts[0]
    assert (rec["tokens_in"], rec["cost_usd"], rec["cost_spent_usd"]) == (2000, 0.004, 0.004)
    run_dir = tmp_path / rec["run_id"]
    events = [json.loads(line)["event"] for line in (run_dir / "run.jsonl").read_text().splitlines()]
    assert events == ["start", "rag", "generation", "execution", "mutation", "run"]
    for name in ("test_main.ino", "serial.log", "rag_chunks.json", "gen.json", "mutants.json", "prompt_system.txt"):
        assert name in rec["artifacts"]
    assert not any(a.startswith("work/") for a in rec["artifacts"])


def test_run_one_isolates_exceptions(tmp_path):
    rec = run_one("blink_fixture", Condition("off", "fake"), tmp_path, ScriptedLLM([RuntimeError("no credits")]),
                  backend=None)
    assert rec["error"] == "RuntimeError: no credits" and rec["exec_verdict"] == "error"
    last = json.loads((tmp_path / rec["run_id"] / "run.jsonl").read_text().splitlines()[-1])
    assert last["event"] == "run" and last["error"] == rec["error"]


def test_matrix_stops_after_consecutive_errors(tmp_path):
    clients = {"fake": ScriptedLLM([RuntimeError("down")] * 10)}
    pairs = [("blink_fixture", Condition(r, "fake")) for r in ("off", "on")] * 3
    records = run_matrix(pairs, tmp_path, clients, backend=None, retrieve=fake_retrieve)
    assert len(records) == 3 and all(r["error"] for r in records)


@requires_arduino
@pytest.mark.arduino
@pytest.mark.avr8js
def test_resume_mutation_only_and_aggregate(avr8js, tmp_path):
    runs = tmp_path / "runs"
    pairs = [("blink_fixture", Condition("off", "fake")), ("blink_fixture", Condition("on", "fake"))]
    llm = ScriptedLLM([GOOD, GOOD])
    run_matrix(pairs, runs, {"fake": llm}, RetryOnce(avr8js), retrieve=fake_retrieve, skip_mutation=True)
    again = run_matrix(pairs, runs, {"fake": llm}, RetryOnce(avr8js), resume=True, retrieve=fake_retrieve)
    assert again == [] and llm.replies == []                      # nothing re-generated

    assert mutation_only(runs, RetryOnce(avr8js), mutants_for=few_mutants) == 2
    recs = latest_records(runs)
    assert all(r["mutation"]["killed"] == 3 for r in recs.values())

    df = aggregate(tmp_path)
    assert len(df) == 2 and set(df["rag"]) == {"off", "on"} and df["mutation_score"].tolist() == [1.0, 1.0]
    assert bool(df["compile_at_1"].all()) and df.loc[df["rag"] == "on", "rag_similarity_mean"].item() == 0.6
    mdf = pd.read_parquet(tmp_path / "mutants.parquet")
    assert len(mdf) == 6 and set(mdf["mutant_id"]) == {1, 2, 3} and bool(mdf["killed"].all())
    summary = (tmp_path / "SUMMARY.md").read_text(encoding="utf-8")
    assert "| fake | off | 1 |" in summary and "| fake | on | 1 |" in summary


def test_latest_record_prefers_success(tmp_path):
    def write(run_id, ts, error):
        d = tmp_path / run_id
        d.mkdir()
        (d / "run.jsonl").write_text(json.dumps({"event": "run", "run_id": run_id, "module_id": "m", "ts_start": ts,
                                                 "condition": {"rag": "off", "model": "x"}, "error": error}) + "\n")
    write("m__off__x__1", "2026-01-01T00:00:00", None)
    write("m__off__x__2", "2026-01-02T00:00:00", "boom")
    assert latest_records(tmp_path)[("m", "off", "x")]["run_id"] == "m__off__x__1"
    write("m__off__x__3", "2026-01-03T00:00:00", None)
    assert latest_records(tmp_path)[("m", "off", "x")]["run_id"] == "m__off__x__3"
