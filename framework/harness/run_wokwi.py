"""Wokwi simulator backend (wokwi-cli). Needs WOKWI_CLI_TOKEN from https://wokwi.com/dashboard/ci."""
from __future__ import annotations

import csv
import json
import os
import shutil
import subprocess
import tempfile
import time
from datetime import datetime, timezone
from pathlib import Path

from framework.common.config import get_secret, load_config, tool_path
from framework.common.protocol import ERROR
from framework.harness.backends import ExecResult

TIMEOUT_EXIT_CODE = 42
DIAGRAM = json.dumps({"version": 1, "author": "harness", "editor": "wokwi", "connections": [],
                      "parts": [{"type": "wokwi-arduino-uno", "id": "uno", "top": 0, "left": 0, "attrs": {}}]})
TOML = '[wokwi]\nversion = 1\nfirmware = "firmware.hex"\nelf = "firmware.elf"\n'
USAGE_FIELDS = ["ts", "run_id", "kind", "module_id", "wall_s", "exit_code", "retries", "verdict"]


class WokwiBackend:
    """diagram: the module's diagram.json text (devices); None = a bare Uno.
    usage_log + context ({run_id, kind, module_id}): every run appends a row to the quota log."""
    name = "wokwi"

    def __init__(self, cfg: dict | None = None, cli: Path | None = None, token: str | None = None,
                 diagram: str | None = None, usage_log: Path | None = None):
        cfg = cfg or load_config()
        self.cli = cli or tool_path(cfg, "wokwi_cli", "wokwi-cli")
        self.token = token if token is not None else get_secret("WOKWI_CLI_TOKEN")
        self.diagram = diagram or DIAGRAM
        self.retries = int((cfg.get("wokwi") or {}).get("retries", 2))
        self.retry_pause_s = float((cfg.get("wokwi") or {}).get("retry_pause_s", 20))
        self.usage_log = Path(usage_log) if usage_log else None
        self.context: dict = {}
        self.wall_total = 0.0         # wall seconds of all runs of this backend object (per-run delta in the runner)

    def check(self) -> tuple[bool, str]:
        if not self.cli or not Path(self.cli).exists():
            return False, "wokwi-cli not found (tools/wokwi-cli/wokwi-cli.exe or PATH)"
        if not self.token:
            return False, "WOKWI_CLI_TOKEN is not set"
        return True, "ok"

    def run(self, firmware: Path, timeout_s: int) -> ExecResult:
        ok, reason = self.check()
        if not ok:
            return ExecResult(ERROR, backend=self.name, error=reason)
        firmware = Path(firmware)
        elf = firmware.with_suffix(".elf")
        if not (firmware.exists() and elf.exists()):
            return ExecResult(ERROR, backend=self.name, error=f"firmware not found: {firmware}")
        t0 = time.time()
        for attempt in range(self.retries + 1):
            result, cloud_failure = self._run_once(firmware, elf, timeout_s)
            if not cloud_failure or attempt == self.retries:
                break
            time.sleep(self.retry_pause_s)
        result.retries, result.duration_s = attempt, time.time() - t0
        self.wall_total += result.duration_s
        self._log_usage(result)
        return result

    def _run_once(self, firmware: Path, elf: Path, timeout_s: int) -> tuple[ExecResult, bool]:
        """One wokwi-cli call -> (result, cloud_failure). A firmware timeout is a verdict, not a cloud failure."""
        work = Path(tempfile.mkdtemp(prefix="wokwi_"))
        try:
            shutil.copyfile(firmware, work / "firmware.hex")
            shutil.copyfile(elf, work / "firmware.elf")
            (work / "diagram.json").write_text(self.diagram, encoding="utf-8")
            (work / "wokwi.toml").write_text(TOML)
            log = work / "serial.log"
            cmd = [str(self.cli), "--timeout", str(timeout_s * 1000), "--timeout-exit-code", str(TIMEOUT_EXIT_CODE),
                   "--serial-log-file", str(log), "--expect-text", "@RUN_END", "--quiet", str(work)]
            t0 = time.time()
            try:
                proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace",
                                      env={**os.environ, "WOKWI_CLI_TOKEN": self.token}, timeout=timeout_s + 90)
                code, out = proc.returncode, proc.stdout + proc.stderr
            except subprocess.TimeoutExpired:
                code, out = None, "wokwi-cli did not exit"
            serial = log.read_text(encoding="utf-8", errors="replace") if log.exists() else ""
            result = ExecResult.from_log(serial, backend=self.name, duration_s=time.time() - t0, exit_code=code)
            cloud_failure = code not in (0, TIMEOUT_EXIT_CODE) and "@RUN_BEGIN" not in serial
            if cloud_failure:
                result.verdict, result.error = ERROR, f"wokwi-cli exit code {code}\n{out}".strip()
            return result, cloud_failure
        finally:
            shutil.rmtree(work, ignore_errors=True)

    def _log_usage(self, result: ExecResult) -> None:
        if not self.usage_log:
            return
        self.usage_log.parent.mkdir(parents=True, exist_ok=True)
        new = not self.usage_log.exists()
        with open(self.usage_log, "a", newline="", encoding="utf-8") as fh:
            w = csv.DictWriter(fh, fieldnames=USAGE_FIELDS)
            if new:
                w.writeheader()
            w.writerow({"ts": datetime.now(timezone.utc).isoformat(timespec="seconds"),
                        "run_id": self.context.get("run_id", ""), "kind": self.context.get("kind", ""),
                        "module_id": self.context.get("module_id", ""), "wall_s": round(result.duration_s, 2),
                        "exit_code": result.exit_code, "retries": result.retries, "verdict": result.verdict})


def wokwi_usage_summary(usage_log: Path) -> dict:
    """Totals for the pilot / quota report: runs, wall seconds, retries, cloud errors, per kind."""
    rows = list(csv.DictReader(open(usage_log, encoding="utf-8"))) if Path(usage_log).exists() else []
    per_kind: dict[str, dict] = {}
    for r in rows:
        k = per_kind.setdefault(r["kind"] or "-", {"runs": 0, "wall_s": 0.0})
        k["runs"] += 1
        k["wall_s"] += float(r["wall_s"] or 0)
    return {"runs": len(rows), "wall_s": round(sum(float(r["wall_s"] or 0) for r in rows), 1),
            "retries": sum(int(r["retries"] or 0) for r in rows),
            "errors": sum(r["verdict"] == ERROR for r in rows), "per_kind": per_kind}
