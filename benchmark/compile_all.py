"""Compile every benchmark module for Arduino Uno with arduino-cli.

    python benchmark/compile_all.py              # all modules
    python benchmark/compile_all.py led_chaser   # selected modules

Exits with 1 if any module fails to compile.
"""
import json
import pathlib
import shutil
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parent.parent
BENCH = ROOT / "benchmark"
FQBN = "arduino:avr:uno"


def find_cli() -> str:
    local = ROOT / "tools" / "arduino-cli" / ("arduino-cli.exe" if sys.platform == "win32" else "arduino-cli")
    if local.exists():
        return str(local)
    on_path = shutil.which("arduino-cli")
    if on_path:
        return on_path
    raise SystemExit("arduino-cli not found: install it into tools/arduino-cli/ or add it to PATH")


def module_dirs(selected):
    dirs = []
    for d in sorted(BENCH.iterdir()):
        if d.is_dir() and (d / (d.name + ".ino")).exists():
            if not selected or d.name in selected:
                dirs.append(d)
    return dirs


def compile_module(cli: str, module_dir: pathlib.Path) -> dict:
    cmd = [cli, "compile", "--fqbn", FQBN, "--warnings", "all", "--format", "json", str(module_dir)]
    t0 = time.time()
    proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    elapsed = time.time() - t0
    result = {"id": module_dir.name, "ok": False, "flash": None, "ram": None, "seconds": elapsed, "stderr": ""}
    try:
        data = json.loads(proc.stdout or "{}")
    except json.JSONDecodeError:
        data = {}
    result["ok"] = bool(data.get("success")) and proc.returncode == 0
    for section in (data.get("builder_result") or {}).get("executable_sections_size") or []:
        if section.get("name") == "text":
            result["flash"] = section.get("size")
        elif section.get("name") == "data":
            result["ram"] = section.get("size")
    if not result["ok"]:
        result["stderr"] = (data.get("compiler_err") or "") + (data.get("error") or "") + proc.stderr
    return result


def main(argv):
    cli = find_cli()
    results = [compile_module(cli, d) for d in module_dirs(set(argv))]
    print(f"{'module':24s} {'status':6s} {'flash':>7s} {'ram':>5s} {'time':>6s}")
    for r in results:
        status = "OK" if r["ok"] else "FAIL"
        flash = "-" if r["flash"] is None else str(r["flash"])
        ram = "-" if r["ram"] is None else str(r["ram"])
        print(f"{r['id']:24s} {status:6s} {flash:>7s} {ram:>5s} {r['seconds']:5.1f}s")
        if not r["ok"]:
            print(r["stderr"].strip())
    failed = [r["id"] for r in results if not r["ok"]]
    print(f"\n{len(results) - len(failed)}/{len(results)} modules compile for {FQBN}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
