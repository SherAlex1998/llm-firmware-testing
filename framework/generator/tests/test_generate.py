import json
from pathlib import Path

import pytest

import framework.generator.generate as gen_mod
from framework.generator.generate import generate_test, resolve_module_dir, save_artifacts
from framework.generator.llm_clients import LLMClient, LLMResponse
from framework.harness.compile import CompileResult
from framework.harness.tests.conftest import FIXTURE_DIR, FIXTURE_TESTS, requires_arduino

GOOD = "Here is the sketch:\n```cpp\n" + (FIXTURE_TESTS / "test_main.ino").read_text(encoding="utf-8") + "```\n"
BAD = ('```cpp\n#include "logic.h"\n#include "test_runner.h"\n'
       "TEST(t) { led_onn(); }\nvoid setup() { Serial.begin(9600); RUN_ALL_TESTS(); }\nvoid loop() {}\n```")


class ScriptedLLM(LLMClient):
    provider = "fake"

    def __init__(self, replies):
        super().__init__({"id": "fake", "price_in_per_mtok": 1.0, "price_out_per_mtok": 2.0})
        self.replies, self.prompts = list(replies), []

    def _call(self, system, user):
        self.prompts.append(user)
        return LLMResponse(self.replies.pop(0), 1000, 500, "fake", 0.5, "end_turn")


def fake_compile(sketch_dir, out_dir, fqbn=None):
    if "led_onn" in (Path(sketch_dir) / "test_main.ino").read_text(encoding="utf-8"):
        return CompileResult(False, None, None, "test_main.ino:4:11: error: 'led_onn' was not declared")
    out = Path(out_dir)
    out.mkdir(parents=True, exist_ok=True)
    hexp = out / "test_main.ino.hex"
    hexp.write_text(":00000001FF\n")
    return CompileResult(True, hexp, out / "test_main.ino.elf", "", 1000, 100)


@pytest.fixture
def no_arduino(monkeypatch):
    monkeypatch.setattr(gen_mod, "compile_sketch", fake_compile)


def test_first_try(no_arduino, tmp_path):
    r = generate_test("blink_fixture", ScriptedLLM([GOOD]), work_dir=tmp_path, max_repair_attempts=2)
    assert r.compile_ok and r.gen_attempts == 1 and r.attempts[0].kind == "generate" and not r.rag
    assert (r.tokens_in, r.tokens_out) == (1000, 500) and r.cost_usd == pytest.approx(0.002)
    assert r.hex_path.exists() and r.code.startswith("// Hand-written test") and r.code.endswith("\n")
    assert (r.module_id, r.model_id) == ("blink_fixture", "fake")


def test_repair_loop(no_arduino, tmp_path):
    llm = ScriptedLLM([BAD, GOOD])
    r = generate_test(FIXTURE_DIR, llm, work_dir=tmp_path)
    assert r.compile_ok and [a.kind for a in r.attempts] == ["generate", "repair"]
    assert not r.attempts[0].compile_ok and "led_onn" in r.attempts[0].compile_stderr
    assert "led_onn" in llm.prompts[1] and "Compiler output" in llm.prompts[1] and "### logic.h" in llm.prompts[1]
    assert (r.tokens_in, r.tokens_out) == (2000, 1000) and r.hex_path.parent.parent.name == "attempt_2"


def test_gives_up_after_max_repairs(no_arduino, tmp_path):
    llm = ScriptedLLM([BAD, BAD, BAD, GOOD])
    r = generate_test("blink_fixture", llm, work_dir=tmp_path, max_repair_attempts=2)
    assert not r.compile_ok and r.gen_attempts == 3 and r.hex_path is None and llm.replies == [GOOD]


def test_rag_context_in_prompt(no_arduino, tmp_path):
    llm = ScriptedLLM([GOOD])
    r = generate_test("blink_fixture", llm, rag_context="REF-42", work_dir=tmp_path)
    assert r.rag and "<reference>" in llm.prompts[0] and "REF-42" in llm.prompts[0]


def test_save_artifacts(no_arduino, tmp_path):
    r = generate_test("blink_fixture", ScriptedLLM([BAD, GOOD]), work_dir=tmp_path / "w")
    names = save_artifacts(r, tmp_path / "out")
    assert sorted(names) == sorted(p.name for p in (tmp_path / "out").iterdir())
    for n in ("test_main.ino", "prompt_system.txt", "gen.json", "attempt_1_prompt.txt", "attempt_1_response.txt",
              "attempt_1_test_main.ino", "attempt_1_compile_stderr.txt", "attempt_2_prompt.txt", "attempt_2_response.txt"):
        assert n in names
    assert "attempt_2_compile_stderr.txt" not in names
    assert (tmp_path / "out" / "test_main.ino").read_text(encoding="utf-8") == r.code
    meta = json.loads((tmp_path / "out" / "gen.json").read_text(encoding="utf-8"))
    assert meta["gen_attempts"] == 2 and meta["compile_ok"] and meta["tokens_in"] == 2000
    assert [a["kind"] for a in meta["attempts"]] == ["generate", "repair"] and "prompt" not in meta["attempts"][0]


def test_resolve_module_dir():
    assert resolve_module_dir("blink_fixture") == FIXTURE_DIR.resolve()
    assert resolve_module_dir(FIXTURE_DIR) == FIXTURE_DIR.resolve()
    assert resolve_module_dir("led_chaser").name == "led_chaser"
    with pytest.raises(FileNotFoundError):
        resolve_module_dir("no_such_module")


@requires_arduino
@pytest.mark.arduino
def test_real_compile_with_scripted_llm(tmp_path):
    r = generate_test("blink_fixture", ScriptedLLM([BAD, GOOD]), work_dir=tmp_path)
    assert r.gen_attempts == 2 and r.compile_ok and r.hex_path.exists()
    assert "led_onn" in r.attempts[0].compile_stderr


@requires_arduino
@pytest.mark.arduino
def test_repair_prompt_is_the_same_in_any_work_dir(tmp_path):
    """The compiler output in the repair prompt must not contain the run's work dir (LLM cache, no local paths)."""
    a = ScriptedLLM([BAD, GOOD])
    b = ScriptedLLM([BAD, GOOD])
    generate_test("blink_fixture", a, work_dir=tmp_path / "run_a")
    generate_test("blink_fixture", b, work_dir=tmp_path / "other_run_b")
    assert a.prompts[1] == b.prompts[1] and "test_main.ino:" in a.prompts[1]
    assert str(tmp_path) not in a.prompts[1] and tmp_path.as_posix() not in a.prompts[1]
