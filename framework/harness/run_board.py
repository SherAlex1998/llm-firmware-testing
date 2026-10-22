from __future__ import annotations

import subprocess
import time
from pathlib import Path

from framework.common.config import load_config, tool_path
from framework.common.protocol import ERROR
from framework.harness.backends import ExecResult


class BoardBackend:
    name = "board"

    def __init__(self, cfg: dict | None = None, port: str | None = None, baud: int | None = None):
        cfg = cfg or load_config()
        self.port = port or cfg["board"]["port"]
        self.baud = baud or cfg["serial"]["baud"]
        self.cli = tool_path(cfg, "arduino_cli", "arduino-cli")
        self.fqbn = cfg["arduino_cli"]["fqbn"]

    def check(self) -> tuple[bool, str]:
        if not self.cli:
            return False, "arduino-cli not found"
        try:
            import serial  # noqa: F401
        except ImportError:
            return False, "pyserial is not installed"
        return True, "ok"

    def run(self, firmware: Path, timeout_s: int) -> ExecResult:
        ok, reason = self.check()
        if not ok:
            return ExecResult(ERROR, backend=self.name, error=reason)
        t0 = time.time()
        cmd = [str(self.cli), "upload", "-p", self.port, "--fqbn", self.fqbn, "--input-file", str(firmware)]
        proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
        if proc.returncode != 0:
            return ExecResult(ERROR, backend=self.name, error="upload failed:\n" + proc.stdout + proc.stderr,
                              duration_s=time.time() - t0)
        try:
            serial_log = self._capture(timeout_s)
        except Exception as exc:
            return ExecResult(ERROR, backend=self.name, error=f"serial capture failed: {exc}",
                              duration_s=time.time() - t0)
        return ExecResult.from_log(serial_log, backend=self.name, duration_s=time.time() - t0, exit_code=0)

    def _capture(self, timeout_s: int) -> str:
        import serial
        chunks: list[str] = []
        deadline, end_seen = time.time() + timeout_s, None
        with serial.Serial(self.port, self.baud, timeout=0.2) as port:   # opening the port resets the Uno
            while time.time() < deadline and (end_seen is None or time.time() - end_seen < 0.5):
                data = port.read(256)
                if data:
                    chunks.append(data.decode("utf-8", errors="replace"))
                    if end_seen is None and "@RUN_END" in "".join(chunks[-3:]):
                        end_seen = time.time()
        return "".join(chunks)
