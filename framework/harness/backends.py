"""Execution result and backend selection: avr8js (default), wokwi or a real board."""
from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path

from framework.common.config import load_config
from framework.common.protocol import TestResult, parse_serial_log
from framework.harness.sketch import load_module_yaml, module_diagram, resolve_module_dir


@dataclass
class ExecResult:
    verdict: str                      # pass | fail | timeout | error
    tests: list[TestResult] = field(default_factory=list)
    tests_total: int = 0
    tests_passed: int = 0
    serial_log: str = ""
    backend: str = ""
    duration_s: float = 0.0
    exit_code: int | None = None
    error: str = ""
    retries: int = 0                  # cloud-failure retries (wokwi)

    @classmethod
    def from_log(cls, serial_log: str, **kw) -> ExecResult:
        p = parse_serial_log(serial_log)
        passed = p.passed if p.passed is not None else sum(t.passed for t in p.tests)
        return cls(p.verdict, p.tests, p.total or len(p.tests), passed, serial_log, **kw)


def backend_name(cfg: dict, name: str | None = None, module_meta: dict | None = None) -> str:
    """backends.force > explicit name (CLI) > module.yaml `backend` > backends.default."""
    b = cfg["backends"]
    return b.get("force") or name or (module_meta or {}).get("backend") or b["default"]


def get_backend(name: str | None = None, cfg: dict | None = None, module: str | Path | None = None):
    """module: benchmark id or folder; picks its backend and, for wokwi, its diagram.json."""
    cfg = cfg or load_config()
    module_dir = resolve_module_dir(module) if module is not None else None
    name = backend_name(cfg, name, load_module_yaml(module_dir) if module_dir else None)
    if name == "wokwi":
        from framework.harness.run_wokwi import WokwiBackend
        return WokwiBackend(cfg, diagram=module_diagram(module_dir) if module_dir else None)
    if name == "avr8js":
        from framework.harness.run_avr8js import Avr8jsBackend
        return Avr8jsBackend(cfg)
    if name == "board":
        from framework.harness.run_board import BoardBackend
        return BoardBackend(cfg)
    raise ValueError(f"unknown backend {name!r}")


def run_firmware(firmware: Path, timeout_s: int | None = None, backend: str | None = None) -> ExecResult:
    cfg = load_config()
    return get_backend(backend, cfg).run(Path(firmware), timeout_s or cfg["serial"]["timeout_s"])
