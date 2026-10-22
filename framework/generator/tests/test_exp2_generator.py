"""Experiment 2 generator changes: multi-file / device prompts, repetition cache keys, experiment 1 prompts unchanged."""
from pathlib import Path

import pytest

from framework.generator.prompts import build_user_prompt
from framework.generator.tests.test_llm_clients import FakeClient
from framework.harness.sketch import REPO_ROOT, resolve_module_dir

MAIN_V3 = REPO_ROOT / "results" / "main_v3" / "runs"
FIXTURES = REPO_ROOT / "framework" / "harness" / "fixtures"


def v1_prompt_files() -> list[Path]:
    """One RAG-off attempt-1 prompt per experiment 1 module."""
    seen, out = set(), []
    for run in sorted(MAIN_V3.glob("*__off__*")) if MAIN_V3.exists() else []:
        module = run.name.split("__")[0]
        if module not in seen and (run / "attempt_1_prompt.txt").exists():
            seen.add(module)
            out.append(run / "attempt_1_prompt.txt")
    return out


@pytest.mark.skipif(not v1_prompt_files(), reason="results/main_v3 not present")
@pytest.mark.parametrize("prompt_file", v1_prompt_files(), ids=lambda p: p.parent.name.split("__")[0])
def test_experiment1_prompts_unchanged(prompt_file):
    module = prompt_file.parent.name.split("__")[0]
    assert build_user_prompt(resolve_module_dir(module)) == prompt_file.read_text(encoding="utf-8")


def test_multifile_prompt_lists_all_sources():
    prompt = build_user_prompt(FIXTURES / "counter_fixture")
    assert prompt.index("### logic.h") < prompt.index("### logic.cpp") < prompt.index("### display.cpp")
    assert "uint8_t cnt_digit(uint8_t pos)" in prompt


def test_device_section():
    prompt = build_user_prompt(FIXTURES / "sonar_fixture")
    assert "External devices (simulated; their readings are fixed for the whole run):" in prompt
    assert "- sonar (wokwi-hc-sr04), pins TRIG=9 ECHO=10: echo distance is fixed at 100 cm" in prompt
    assert "attrs" not in prompt and "diagram" not in prompt


def test_repetition_has_own_cache_entry(tmp_path):
    c = FakeClient(["rep1", "rep2"], tmp_path)
    assert c.generate("s", "u").text == "rep1"
    c.rep = 2
    r = c.generate("s", "u")
    assert (r.text, r.cached) == ("rep2", False)
    assert c.generate("s", "u").cached               # rep 2 is cached under its own key
    c.rep = 1
    assert c.generate("s", "u").text == "rep1"       # rep 1 key unchanged (experiment 1 cache stays valid)


def test_experiment1_system_prompt_unchanged():
    from framework.generator.prompts import build_system_prompt
    saved = sorted(MAIN_V3.glob("*/prompt_system.txt")) if MAIN_V3.exists() else []
    if not saved:
        pytest.skip("results/main_v3 not present")
    assert build_system_prompt() == saved[0].read_text(encoding="utf-8")
    assert build_system_prompt(resolve_module_dir("traffic_light")) == saved[0].read_text(encoding="utf-8")


def test_experiment2_system_prompt_lets_model_choose_test_count():
    from framework.generator.prompts import RULE_TEST_COUNT, RULE_TEST_COUNT_EXP2, build_system_prompt
    prompt = build_system_prompt(FIXTURES / "counter_fixture")
    assert RULE_TEST_COUNT not in prompt and RULE_TEST_COUNT_EXP2 in prompt
    assert "TEST_MAX_TESTS (40)" in prompt and "PROGMEM" in prompt
