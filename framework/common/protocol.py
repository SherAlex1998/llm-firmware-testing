"""Serial protocol printed by test_runner.h:

    @RUN_BEGIN <total>
    @TEST <name> PASS
    @TEST <name> FAIL <message>
    @RUN_END <passed>/<total>

No @RUN_END -> timeout; passed < total or no tests -> fail; other lines are ignored.
"""
from __future__ import annotations

import re
from dataclasses import dataclass, field

PASS, FAIL, TIMEOUT, ERROR = "pass", "fail", "timeout", "error"


@dataclass
class TestResult:
    name: str
    passed: bool
    msg: str = ""


@dataclass
class ParsedRun:
    verdict: str = TIMEOUT
    tests: list[TestResult] = field(default_factory=list)
    total: int | None = None
    passed: int | None = None
    run_end: bool = False
    warnings: list[str] = field(default_factory=list)


_MARKER = re.compile(r"@(RUN_BEGIN|TEST|RUN_END|WARN)\b\s*(.*)$")
_COUNTS = re.compile(r"(\d+)\s*/\s*(\d+)")


def parse_serial_log(text: str) -> ParsedRun:
    run = ParsedRun()
    for line in text.splitlines():
        m = _MARKER.search(line)
        if not m:
            continue
        kind, rest = m.groups()
        if kind == "RUN_BEGIN" and rest.strip().isdigit():
            run.total = int(rest)
        elif kind == "TEST":
            name, status, msg = (rest.split(None, 2) + ["", ""])[:3]
            if status in ("PASS", "FAIL"):
                run.tests.append(TestResult(name, status == "PASS", msg))
        elif kind == "RUN_END":
            run.run_end = True
            if c := _COUNTS.match(rest):
                run.passed, run.total = int(c[1]), int(c[2])
        elif kind == "WARN":
            run.warnings.append(rest)
    if not run.run_end:
        return run
    if run.total is None:
        run.total = len(run.tests)
    if run.passed is None:
        run.passed = sum(t.passed for t in run.tests)
    run.verdict = PASS if 0 < run.total == run.passed else FAIL
    return run
