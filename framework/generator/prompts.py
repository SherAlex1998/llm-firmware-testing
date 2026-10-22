from __future__ import annotations

import re
from pathlib import Path

from framework.harness.sketch import TEST_RUNNER_H, TEST_RUNNER_V2_H, is_exp2, load_module_yaml, module_sources

STDERR_LIMIT = 6000

SYSTEM_TEMPLATE = """\
You are an expert in embedded software testing. You write test sketches for Arduino Uno modules.

Target: Arduino Uno (ATmega328P, 16 MHz, 2 KB SRAM, 32 KB flash). The test sketch is compiled with
arduino-cli for arduino:avr:uno and executed in a simulator or on a real board; the verdict is read
from the Serial protocol printed by the test framework.

Rules for the test sketch:
1. One file. It starts with `#include "logic.h"` and `#include "test_runner.h"`. Do not add other
   includes unless logic.h itself requires them (e.g. <Servo.h>).
2. Define 3 to 8 tests with TEST(name). Names are unique C identifiers.
3. Test only functions declared in logic.h. Make every test independent: if the module has an init
   function, call it at the start of each test.
4. Check outputs with ASSERT_PIN(pin, HIGH|LOW) and values with ASSERT_EQ / ASSERT_NE / ASSERT_NEAR /
   ASSERT_TRUE / ASSERT_FALSE. Use ASSERT_* only directly inside a TEST body (they `return` on
   failure). ASSERT_PIN reads the pin back with digitalRead: on the Uno an OUTPUT pin reads back
   the level last written to it; after analogWrite only 0 (LOW) and 255 (HIGH) are readable.
5. Keep the whole run under 15 s: no delay() above 300 ms in one test, no infinite loops, no waiting
   for external input. No dynamic memory (new, malloc, String). No Serial.print (Serial carries the
   protocol).
6. Finish with exactly:
   void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }
   void loop() {}
7. Reply with ONLY the complete sketch in a single ```cpp code block. No explanations.

The test framework (test_runner.h) that the sketch includes:
```cpp
{test_runner}
```
"""


RULE_TEST_COUNT = "2. Define 3 to 8 tests with TEST(name). Names are unique C identifiers."
# experiment 2: the model chooses how many tests to write
RULE_TEST_COUNT_EXP2 = ("2. Decide yourself how many tests the module needs and define them with TEST(name). "
                        "Names are unique C identifiers.")


def build_system_prompt(module_dir: Path | None = None) -> str:
    """Experiment 1 text by default; for an experiment 2 module the free test count and test_runner_v2.h."""
    if module_dir is not None and is_exp2(module_dir):
        template = SYSTEM_TEMPLATE.replace(RULE_TEST_COUNT, RULE_TEST_COUNT_EXP2)
        return template.replace("{test_runner}", TEST_RUNNER_V2_H.read_text(encoding="utf-8").rstrip())
    return SYSTEM_TEMPLATE.replace("{test_runner}", TEST_RUNNER_H.read_text(encoding="utf-8").rstrip())


def module_context(module_dir: Path) -> str:
    meta, src = load_module_yaml(module_dir), module_sources(module_dir)
    lines = [f"Module: {meta['name']} (id: {meta['id']}, category: {meta['category']}, "
             f"complexity: {meta['complexity']})", "Pins:"]
    lines += [f"- {p['pin']} {p['mode']}: {p.get('role', '')}".rstrip() for p in meta["pins"]]
    if meta.get("devices"):
        lines += ["External devices (simulated; their readings are fixed for the whole run):"]
        lines += [f"- {d['id']} ({d['part']}), pins {' '.join(f'{k}={v}' for k, v in d.get('pins', {}).items())}: "
                  f"{d['test_visible']}" for d in meta["devices"]]
    if meta.get("libraries"):
        lines += ["Libraries available: " + ", ".join(f"{lib['name']} {lib['version']}" for lib in meta["libraries"])]
    lines += ["Functions to test:"] + [f"- {fn}" for fn in meta["entry_functions"]]
    for name, text in src.items():
        lines += ["", f"### {name}", "```cpp", text.rstrip(), "```"]
    return "\n".join(lines)


def build_user_prompt(module_dir: Path, rag_context: str | None = None) -> str:
    parts = [module_context(module_dir)]
    if rag_context:
        parts += ["", "<reference>", rag_context.strip(), "</reference>",
                  "The <reference> block contains documentation excerpts; use them only where they apply."]
    return "\n".join(parts + ["", "Write the test sketch for this module."])


def build_repair_prompt(prev_code: str, compiler_stderr: str, context: str = "") -> str:
    stderr = compiler_stderr.strip()
    if len(stderr) > STDERR_LIMIT:
        stderr = stderr[:STDERR_LIMIT] + "\n... (truncated)"
    return "\n".join(([context, ""] if context else []) + [
        "The test sketch below failed to compile. Fix it and reply with ONLY the complete corrected "
        "sketch in a single ```cpp code block.",
        "", "### Sketch", "```cpp", prev_code.rstrip(), "```",
        "", "### Compiler output (arduino-cli, arduino:avr:uno)", "```", stderr, "```"])


_FENCE = re.compile(r"```[ \t]*(?:cpp|c\+\+|cxx|c|arduino|ino)?[ \t]*\r?\n(.*?)```", re.S | re.I)


def extract_code(text: str) -> str:
    """The ```cpp block containing RUN_ALL_TESTS, else the longest block, else the whole reply."""
    blocks = [b.strip("\r\n") for b in _FENCE.findall(text)]
    if not blocks:
        return text.strip()
    return next((b for b in blocks if "RUN_ALL_TESTS" in b), max(blocks, key=len))
