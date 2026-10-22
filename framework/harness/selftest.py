"""Manual cycle stage -> compile -> run -> parse.

    python -m framework.harness.selftest                     # blink_fixture, expect pass 6/6
    python -m framework.harness.selftest --test test_failing # expect fail 1/2
    python -m framework.harness.selftest --test test_hang --timeout 5
    python -m framework.harness.selftest --module benchmark/led_chaser --test-file my_test.ino
"""
from __future__ import annotations

import argparse
import sys
import tempfile
from pathlib import Path

from framework.common.config import REPO_ROOT, load_config
from framework.harness.backends import get_backend
from framework.harness.compile import compile_sketch
from framework.harness.sketch import stage_test_sketch

FIXTURE = REPO_ROOT / "framework" / "harness" / "fixtures" / "blink_fixture"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--module", default=str(FIXTURE))
    ap.add_argument("--test", default="test_main", help="<module>/tests/<name>.ino")
    ap.add_argument("--test-file")
    ap.add_argument("--backend")
    ap.add_argument("--timeout", type=int)
    ap.add_argument("--compile-only", action="store_true")
    args = ap.parse_args()

    cfg = load_config()
    module = Path(args.module)
    test_path = Path(args.test_file) if args.test_file else module / "tests" / f"{args.test}.ino"
    out = Path(tempfile.mkdtemp(prefix="harness_"))
    sketch = stage_test_sketch(module, test_path.read_text(encoding="utf-8"), out)
    comp = compile_sketch(sketch, out / "build")
    print(f"compile: ok={comp.ok} flash={comp.flash_bytes} ram={comp.ram_bytes} ({comp.duration_s:.1f}s) -> {out}")
    if not comp.ok:
        print(comp.stderr)
        return 2
    if args.compile_only:
        return 0
    backend = get_backend(args.backend, cfg, module=module)   # module.yaml `backend` + diagram.json when no --backend
    ok, reason = backend.check()
    if not ok:
        print(f"backend {backend.name} unavailable: {reason}")
        return 3
    res = backend.run(comp.hex_path, args.timeout or cfg["serial"]["timeout_s"])
    print(f"run ({backend.name}, {res.duration_s:.1f}s): verdict={res.verdict} "
          f"tests={res.tests_passed}/{res.tests_total} exit={res.exit_code}")
    for t in res.tests:
        print(f"  [{'PASS' if t.passed else 'FAIL'}] {t.name} {t.msg}")
    if res.error:
        print("error:", res.error)
    print("--- serial ---")
    print(res.serial_log.strip())
    return 0 if res.verdict == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
