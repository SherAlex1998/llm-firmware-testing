"""Complexity metrics of the benchmark modules (lizard) and the tier check of experiment 2.

    python benchmark/metrics.py            # table of all modules; writes `metrics:` and `loc:` into experiment 2 module.yaml
    python benchmark/metrics.py --check    # also exit 1 if an experiment 2 module does not meet its tier

Experiment 1 module.yaml files are never modified (their numbers only go to the table).
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

import lizard
import yaml

BENCH = Path(__file__).resolve().parent
NLOC = {"simple": (0, 130), "medium": (130, 300), "complex": (300, 850)}


def measure(module_dir: Path) -> dict:
    nloc, funcs, ccn, code = 0, 0, [], ""
    for f in sorted((module_dir / "src").iterdir()):
        if f.suffix not in (".h", ".hpp", ".cpp", ".c"):
            continue
        text = f.read_text(encoding="utf-8")
        code += text
        info = lizard.analyze_file.analyze_source_code(f.name if f.suffix not in (".h",) else f.stem + ".hpp", text)
        nloc += info.nloc
        funcs += len(info.function_list)
        ccn += [fn.cyclomatic_complexity for fn in info.function_list]
    return {"nloc": nloc, "ccn_sum": sum(ccn), "ccn_max": max(ccn, default=0), "functions": funcs,
            "eeprom": "EEPROM." in code, "millis_fsm": "millis()" in code and len(re.findall(r"\bcase\s", code)) >= 4,
            "bus_device": bool(re.search(r"#include\s*<(Wire|SPI|OneWire)\.h>|\bshiftOut\s*\(", code)),
            "interrupts": bool(re.search(r"attachInterrupt|ISR\s*\(|TIMSK", code))}


def features(meta: dict, m: dict) -> list[str]:
    out = []
    if meta.get("libraries"):
        out.append("library")
    if meta.get("devices"):
        out.append("device")
    if m["bus_device"] or any(d.get("part") in ("wokwi-hc-sr04", "wokwi-ir-receiver", "wokwi-neopixel", "wokwi-dht22")
                              for d in meta.get("devices") or []):
        out.append("bus_or_pulse")
    if m["interrupts"] or meta.get("interrupts"):
        out.append("interrupts")
    if m["eeprom"]:
        out.append("eeprom")
    if m["millis_fsm"]:
        out.append("millis_fsm")
    return out


def tier_ok(tier: str, m: dict, feats: list[str]) -> tuple[bool, str]:
    lo, hi = NLOC[tier]
    if not lo <= m["nloc"] <= hi:
        return False, f"NLOC {m['nloc']} outside {lo}-{hi}"
    if tier == "medium" and not feats:
        return False, "medium needs at least one feature"
    complex_feats = {"library", "bus_or_pulse", "interrupts", "millis_fsm", "eeprom"} & set(feats)
    if tier == "complex" and not complex_feats:
        return False, "complex needs at least one feature"
    return True, "ok"


def write_metrics(module_dir: Path, m: dict) -> None:
    path = module_dir / "module.yaml"
    text = path.read_text(encoding="utf-8")
    line = f"metrics: {{nloc: {m['nloc']}, ccn_sum: {m['ccn_sum']}, ccn_max: {m['ccn_max']}, functions: {m['functions']}}}"
    text = re.sub(r"^metrics:.*$", line, text, flags=re.M) if re.search(r"^metrics:", text, re.M) \
        else re.sub(r"^notes:", line + "\nnotes:", text, count=1, flags=re.M)
    text = re.sub(r"^loc: \d+$", f"loc: {m['nloc']}", text, flags=re.M)
    path.write_text(text, encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    bad = 0
    print(f"{'module':26s} {'exp':3s} {'tier':8s} {'nloc':>5s} {'ccn':>4s} {'max':>4s} {'fn':>3s}  features  check")
    for module_dir in sorted(p.parent for p in BENCH.glob("*/module.yaml")):
        meta = yaml.safe_load((module_dir / "module.yaml").read_text(encoding="utf-8"))
        exp = meta.get("experiment", 1)
        m = measure(module_dir)
        feats = features(meta, m)
        ok, why = tier_ok(meta["complexity"], m, feats)
        if exp == 2:
            write_metrics(module_dir, m)
            bad += not ok
        print(f"{module_dir.name:26s} {exp!s:3s} {meta['complexity']:8s} {m['nloc']:5d} {m['ccn_sum']:4d} "
              f"{m['ccn_max']:4d} {m['functions']:3d}  {','.join(feats) or '-'}  {why}")
    return 1 if args.check and bad else 0


if __name__ == "__main__":
    sys.exit(main())
