"""arduino-cli compile wrapper. The sketch folder must contain <folder>.ino."""
from __future__ import annotations

import json
import os
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path

from framework.common.config import load_config, tool_path


@dataclass
class CompileResult:
    ok: bool
    hex_path: Path | None
    elf_path: Path | None
    stderr: str                       # full compiler diagnostics (feeds the LLM repair prompt)
    flash_bytes: int | None = None
    ram_bytes: int | None = None
    duration_s: float = 0.0


def find_arduino_cli(cfg: dict | None = None) -> Path:
    cli = tool_path(cfg or load_config(), "arduino_cli", "arduino-cli")
    if cli is None:
        raise FileNotFoundError("arduino-cli not found: tools/arduino-cli/arduino-cli.exe or PATH")
    return cli


def arduino_cli_version() -> str:
    out = subprocess.run([str(find_arduino_cli()), "version", "--format", "json"], capture_output=True, text=True).stdout
    return json.loads(out or "{}").get("VersionString", "")


def installed_libraries(cfg: dict | None = None) -> dict[str, str]:
    out = subprocess.run([str(find_arduino_cli(cfg)), "lib", "list", "--format", "json"], capture_output=True,
                         text=True, encoding="utf-8", errors="replace").stdout
    return {it["library"]["name"]: it["library"]["version"]
            for it in json.loads(out or "{}").get("installed_libraries") or []}


def ensure_libraries(cfg: dict | None = None, install: bool = True) -> dict[str, str]:
    """Install the pinned arduino_cli.libraries ("Name@version"); raise before an experiment if a version differs."""
    cfg = cfg or load_config()
    want = dict(spec.rsplit("@", 1) for spec in cfg["arduino_cli"].get("libraries") or [])
    have = installed_libraries(cfg)
    missing = [f"{n}@{v}" for n, v in want.items() if have.get(n) != v]
    if missing and install:
        proc = subprocess.run([str(find_arduino_cli(cfg)), "lib", "install", *missing], capture_output=True,
                              text=True, encoding="utf-8", errors="replace")
        if proc.returncode != 0:
            raise RuntimeError(f"arduino-cli lib install failed: {proc.stderr.strip() or proc.stdout.strip()}")
        have = installed_libraries(cfg)
    wrong = {n: (v, have.get(n)) for n, v in want.items() if have.get(n) != v}
    if wrong:
        raise RuntimeError(f"library versions differ from the config (want, have): {wrong}")
    return {n: have[n] for n in want}


def compile_sketch(sketch_dir: Path, out_dir: Path, fqbn: str | None = None) -> CompileResult:
    cfg = load_config()
    sketch_dir, out_dir = Path(sketch_dir).resolve(), Path(out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    cmd = [str(find_arduino_cli(cfg)), "compile", "--fqbn", fqbn or cfg["arduino_cli"]["fqbn"],
           "--output-dir", str(out_dir), "--format", "json", "--no-color", str(sketch_dir)]
    t0 = time.time()
    proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace",
                          env={**os.environ, "ARDUINO_LOCALE": "en"})
    try:
        data = json.loads(proc.stdout)
    except ValueError:
        data = {}
    sizes = {s["name"]: s["size"] for s in (data.get("builder_result") or {}).get("executable_sections_size") or []}
    hex_path = out_dir / f"{sketch_dir.name}.ino.hex"
    elf_path = out_dir / f"{sketch_dir.name}.ino.elf"
    ok = bool(proc.returncode == 0 and data.get("success") and hex_path.exists() and elf_path.exists())
    stderr = data.get("compiler_err") or ""
    if not ok:
        stderr = "\n".join(s for s in (stderr, data.get("error") or "", proc.stderr) if s.strip())
    return CompileResult(ok, hex_path if ok else None, elf_path if ok else None, stderr,
                         sizes.get("text"), sizes.get("data"), time.time() - t0)
