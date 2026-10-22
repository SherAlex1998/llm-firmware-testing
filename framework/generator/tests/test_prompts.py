from framework.generator.prompts import (STDERR_LIMIT, build_repair_prompt, build_system_prompt,
                                         build_user_prompt, extract_code)
from framework.harness.tests.conftest import FIXTURE_DIR


def test_system_prompt_embeds_test_runner():
    s = build_system_prompt()
    assert "RUN_ALL_TESTS" in s and "#define ASSERT_PIN" in s and "{test_runner}" not in s
    assert "```cpp" in s and "Serial.begin(9600)" in s


def test_user_prompt_has_sources_and_meta():
    u = build_user_prompt(FIXTURE_DIR)
    assert "void led_on()" in u and "led_on_count" in u and "- 13 OUTPUT: status LED" in u
    assert "### logic.h" in u and "### logic.cpp" in u and "id: blink_fixture" in u
    assert "<reference>" not in u


def test_user_prompt_rag_block():
    u = build_user_prompt(FIXTURE_DIR, "digitalRead returns the written level")
    assert "<reference>" in u and "digitalRead returns the written level" in u and "</reference>" in u


def test_repair_prompt_contains_code_errors_and_context():
    p = build_repair_prompt("int x;", "error: foo", "CTX")
    assert p.startswith("CTX") and "int x;" in p and "error: foo" in p and "```cpp" in p


def test_repair_prompt_truncates_stderr():
    p = build_repair_prompt("c", "e" * (STDERR_LIMIT + 100))
    assert "(truncated)" in p and p.count("e" * 100) < STDERR_LIMIT // 100 + 2


def test_extract_cpp_fence():
    assert extract_code("text\n```cpp\nint a;\n```\nmore") == "int a;"


def test_extract_plain_fence_and_crlf():
    assert extract_code("```\nint b;\n```") == "int b;"
    assert extract_code("```cpp\r\nint c;\r\n```") == "int c;"


def test_extract_no_fence_returns_whole_text():
    assert extract_code("  no fences at all \n") == "no fences at all"


def test_extract_prefers_block_with_run_all_tests():
    text = "```cpp\nint helper;\n```\n\n```cpp\nvoid setup() { RUN_ALL_TESTS(); }\n```"
    assert extract_code(text) == "void setup() { RUN_ALL_TESTS(); }"


def test_extract_falls_back_to_longest_block():
    assert extract_code("```\nshort\n```\n```arduino\nmuch longer block\n```") == "much longer block"
