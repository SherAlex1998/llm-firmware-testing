import json
import shutil
from types import SimpleNamespace as NS

import pytest

from framework.common.config import REPO_ROOT, load_config
from framework.harness.sketch import resolve_module_dir
from framework.harness.tests.conftest import requires_arduino
from framework.mutation.catalog import build_catalog, load_catalog, smoke_mark
from framework.mutation.mutator import make_mutants


def always(*_):
    return True


def test_catalog_roundtrip_two_files(tmp_path):
    cat = build_catalog("counter_fixture", tmp_path, n=6, check=always)
    assert {m["file"] for m in cat["mutants"]} == {"logic.cpp", "display.cpp"}
    assert all(m["trivial"] is None and m["equivalent"] is None for m in cat["mutants"])
    loaded = load_catalog("counter_fixture", tmp_path)
    assert loaded == tuple(make_mutants("counter_fixture", 6, work_dir=tmp_path / "w", check=always))


def test_catalog_is_not_rebuilt_without_force(tmp_path):
    build_catalog("counter_fixture", tmp_path, n=4, check=always)
    path = tmp_path / "counter_fixture" / "catalog.json"
    first = path.read_text(encoding="utf-8")
    assert build_catalog("counter_fixture", tmp_path, n=2, check=always)["mutants"] == json.loads(first)["mutants"]
    assert path.read_text(encoding="utf-8") == first
    build_catalog("counter_fixture", tmp_path / "again", n=4, check=always)
    assert (tmp_path / "again" / "counter_fixture" / "catalog.json").read_text(encoding="utf-8") == first


def test_catalog_refuses_changed_module(tmp_path):
    module = tmp_path / "counter_fixture"
    shutil.copytree(resolve_module_dir("counter_fixture"), module)
    build_catalog(str(module), tmp_path / "cat", n=2, check=always)
    logic = module / "src" / "logic.cpp"
    logic.write_text(logic.read_text(encoding="utf-8") + "\n// edited\n", encoding="utf-8")
    with pytest.raises(ValueError, match="changed"):
        load_catalog(str(module), tmp_path / "cat")
    with pytest.raises(FileNotFoundError):
        load_catalog("blink_fixture", tmp_path / "cat")


@requires_arduino
@pytest.mark.arduino
def test_smoke_mark_fills_only_unchecked(tmp_path):
    module = tmp_path / "counter_fixture"
    shutil.copytree(resolve_module_dir("counter_fixture"), module)
    (module / "tests" / "smoke_test.ino").write_text((module / "tests" / "test_main.ino").read_text(encoding="utf-8"),
                                                     encoding="utf-8")
    build_catalog(str(module), tmp_path / "cat", n=3)
    verdicts = iter(["pass", "fail", "pass", "timeout"])     # original, then the 3 mutants

    class Scripted:
        name = "scripted"

        def run(self, fw, t):
            return NS(verdict=next(verdicts))
    cat = smoke_mark(str(module), tmp_path / "cat", Scripted(), timeout_s=1)
    assert cat["smoke"] == {"backend": "scripted", "original_verdict": "pass"}
    assert [m["trivial"] for m in cat["mutants"]] == [True, False, True]
    assert [m["smoke_verdict"] for m in cat["mutants"]] == ["fail", "pass", "timeout"]
    assert smoke_mark(str(module), tmp_path / "cat", None)["mutants"] == cat["mutants"]    # nothing left to run


def test_exp2_catalog_complete():
    """Every experiment 2 module has a catalog of 10 mutants built from its current sources."""
    cfg = load_config()
    for module in cfg["experiment2"]["modules"]:
        assert len(load_catalog(module)) == cfg["mutation"]["mutants_per_module"], module


MAIN_V3 = REPO_ROOT / "results" / "main_v3" / "runs"


@pytest.mark.skipif(not MAIN_V3.exists(), reason="results/main_v3 not present")
def test_v1_mutant_sets_unchanged(tmp_path):
    """Experiment 1 stays reproducible: make_mutants() gives the mutants saved in results/main_v3."""
    seen = {}
    for run in sorted(MAIN_V3.iterdir()):
        if (run / "mutants").is_dir():
            seen.setdefault(run.name.split("__")[0], run)
    assert len(seen) == 15
    for module, run in seen.items():
        mutants = make_mutants(module, work_dir=tmp_path / module, check=always)   # v1: compiler rejected none
        assert [m.source for m in mutants] == [(run / "mutants" / str(k) / "logic.cpp").read_text(encoding="utf-8")
                                               for k in range(1, 11)], module


def test_disable_tests_keeps_code_but_not_registration():
    from framework.mutation.run_mutation import disable_tests, test_names
    src = '#include "logic.h"\nTEST(a) { ASSERT_TRUE(1); }\nTEST( b ) { ASSERT_EQ(1, 2); }\nTEST(ab) {}\n'
    assert test_names(src) == ["a", "b", "ab"]
    out = disable_tests(src, {"b", "a"})
    assert test_names(out) == ["ab"] and "TEST_DISABLED_(b)" in out and "TEST_DISABLED_(a)" in out


@requires_arduino
@pytest.mark.arduino
def test_pruned_score_drops_failed_tests_and_rechecks(tmp_path):
    from framework.harness.tests.conftest import FIXTURE_TESTS
    from framework.mutation.run_mutation import pruned_mutation_score
    src = (FIXTURE_TESTS / "test_main.ino").read_text(encoding="utf-8")
    first, second = __import__("framework.mutation.run_mutation", fromlist=["x"]).test_names(src)[:2]
    mutants = make_mutants("blink_fixture", 2, work_dir=tmp_path / "m", check=always)
    script = iter([NS(verdict="fail", tests=[NS(name=second, passed=False)]),   # check 1: another test depended on `first`
                   NS(verdict="pass", tests=[]),                                 # check 2: the rest passes
                   NS(verdict="fail", tests=[]), NS(verdict="pass", tests=[])])  # the two mutants

    class Scripted:
        name = "scripted"

        def run(self, fw, t):
            return next(script)
    res = pruned_mutation_score("blink_fixture", src, [first], mutants, Scripted(), timeout_s=1, work_dir=tmp_path / "w")
    assert res.status == "pass" and res.disabled == sorted([first, second])
    assert (res.mutation.killed, res.mutation.mutants_total) == (1, 2) and "TEST_DISABLED_" in res.source
