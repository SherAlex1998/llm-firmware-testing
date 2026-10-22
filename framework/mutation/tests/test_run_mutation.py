import json
from pathlib import Path
from types import SimpleNamespace as NS

import pytest

from framework.mutation.mutator import make_mutants
from framework.mutation.run_mutation import mutation_score, save_mutation
from framework.harness.tests.conftest import FIXTURE_TESTS, avr8js, requires_arduino, wokwi  # noqa: F401 (pytest fixtures)

TEST_MAIN = (FIXTURE_TESTS / "test_main.ino").read_text(encoding="utf-8")


class ScriptedBackend:
    name = "fake"

    def __init__(self, verdicts):
        self.verdicts = list(verdicts)

    def run(self, firmware, timeout_s):
        return NS(verdict=self.verdicts.pop(0))


@pytest.fixture
def mutants(tmp_path):
    return make_mutants("blink_fixture", 5, seed=3, work_dir=tmp_path / "c", check=lambda *a: True)


@requires_arduino
@pytest.mark.arduino
def test_score_counts_fail_and_timeout_as_killed_and_drops_errors(mutants, tmp_path):
    res = mutation_score("blink_fixture", TEST_MAIN, mutants, ScriptedBackend(["fail", "pass", "timeout", "error", "pass"]),
                         work_dir=tmp_path)
    assert [r.killed for r in res.per_mutant] == [True, False, True, None, False]
    assert (res.mutants_total, res.killed, res.score) == (4, 2, 0.5) and len(res.warnings) == 1
    save_mutation(res, mutants, tmp_path / "out")
    data = json.loads((tmp_path / "out" / "mutants.json").read_text(encoding="utf-8"))
    assert data["score"] == 0.5 and data["per_mutant"][0]["before"] and data["per_mutant"][0]["after"]
    assert (tmp_path / "out" / "mutants" / "5" / "logic.cpp").read_text(encoding="utf-8") == mutants[4].source


def test_score_undefined_when_nothing_ran(mutants, tmp_path, monkeypatch):
    import framework.mutation.run_mutation as rm
    monkeypatch.setattr(rm, "compile_sketch", lambda *a, **k: NS(ok=False, hex_path=None))
    res = mutation_score("blink_fixture", TEST_MAIN, mutants, ScriptedBackend([]), work_dir=tmp_path)
    assert (res.mutants_total, res.killed, res.score) == (0, 0, None) and len(res.warnings) == 5


@requires_arduino
@pytest.mark.arduino
@pytest.mark.wokwi
def test_fixture_test_kills_at_least_three_of_five(wokwi, tmp_path):
    ms = make_mutants("blink_fixture", 5, work_dir=tmp_path / "c")
    res = mutation_score("blink_fixture", TEST_MAIN, ms, wokwi, work_dir=tmp_path)
    assert res.mutants_total == 5 and res.killed >= 3, [(r.operator, r.before, r.after, r.verdict) for r in res.per_mutant]


@requires_arduino
@pytest.mark.arduino
@pytest.mark.avr8js
def test_fixture_test_kills_all_ten_offline(avr8js, tmp_path):
    ms = make_mutants("blink_fixture", work_dir=tmp_path / "c")
    res = mutation_score("blink_fixture", TEST_MAIN, ms, avr8js, work_dir=tmp_path)
    assert (res.mutants_total, res.killed) == (10, 10), [(r.operator, r.before, r.after, r.verdict) for r in res.per_mutant]
