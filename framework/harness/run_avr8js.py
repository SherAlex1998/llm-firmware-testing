"""Offline simulator backend: avr8js (open-source core of Wokwi) in Node.js. No token, no network, no quota.

Setup once: `npm ci` in framework/harness/avr8js (version pinned in package.json / package-lock.json).
"""
from __future__ import annotations

import shutil
import subprocess
import time
from pathlib import Path

from framework.common.config import load_config, resolve_path
from framework.common.protocol import ERROR
from framework.harness.backends import ExecResult

RUNNER = Path(__file__).resolve().parent / "avr8js" / "run.js"
TIMEOUT_EXIT_CODE = 42


class Avr8jsBackend:
    name = "avr8js"

    def __init__(self, cfg: dict | None = None, node: str | None = None):
        configured = ((cfg or load_config()).get("avr8js") or {}).get("node")
        self.node = node or (str(resolve_path(configured)) if configured else shutil.which("node"))

    def check(self) -> tuple[bool, str]:
        if not self.node or not Path(self.node).exists():
            return False, "node not found (install Node.js or set avr8js.node in configs/experiment.yaml)"
        if not (RUNNER.parent / "node_modules" / "avr8js").exists():
            return False, f"avr8js is not installed: run `npm ci` in {RUNNER.parent}"
        return True, "ok"

    def run(self, firmware: Path, timeout_s: int) -> ExecResult:
        ok, reason = self.check()
        if not ok:
            return ExecResult(ERROR, backend=self.name, error=reason)
        if not Path(firmware).exists():
            return ExecResult(ERROR, backend=self.name, error=f"firmware not found: {firmware}")
        t0 = time.time()
        try:
            proc = subprocess.run([self.node, str(RUNNER), str(firmware), str(timeout_s * 1000)], capture_output=True,
                                  text=True, encoding="utf-8", errors="replace", timeout=timeout_s * 10 + 30)
        except subprocess.TimeoutExpired:
            return ExecResult(ERROR, backend=self.name, duration_s=time.time() - t0, error="node did not exit")
        result = ExecResult.from_log(proc.stdout, backend=self.name, duration_s=time.time() - t0,
                                     exit_code=proc.returncode)
        if proc.returncode not in (0, TIMEOUT_EXIT_CODE):
            result.verdict, result.error = ERROR, f"avr8js exit code {proc.returncode}\n{proc.stderr}".strip()
        return result
