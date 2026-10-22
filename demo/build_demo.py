"""Offline test player: one HTML file that replays experiment runs step by step on a drawn Arduino Uno.

    python demo/build_demo.py            # -> demo/test_player.html
    python demo/build_demo.py --dump     # also writes demo/build/index.json

Includes the 90 runs of experiment 1 and the experiment 2 pilot runs that used avr8js. The saved firmware of every
run and of its mutants is re-executed with demo/trace.js, which uses the board model of the harness, and the build
fails unless every serial log and mutant verdict matches the experiment. Needs the runs' work/ build folders, which
are not part of the repository. Parts are drawn with @wokwi/elements (MIT, demo/vendor).
"""
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[1]
DEMO = ROOT / "demo"
EXPERIMENTS = [
    {"id": "exp1", "label": "Experiment 1 (simple modules)", "runs": ROOT / "results" / "main_v3" / "runs",
     "diagrams": DEMO / "diagrams" / "{m}.json"},
    {"id": "exp2-pilot", "label": "Experiment 2 pilot (complex module)", "runs": ROOT / "results" / "exp2" / "pilot" / "runs",
     "diagrams": DEMO / "diagrams" / "{m}.json", "backend": "avr8js"},
]
MODELS = {"claude-sonnet-5": "Claude Sonnet 5", "gpt-5.6-terra": "GPT-5.6 Terra", "claude-haiku-4-5": "Claude Haiku 4.5"}
START = {"module": "led_chaser", "model": "gpt-5.6-terra", "rag": "on"}     # opened first
TIMEOUT_MS = 15000                                                           # configs/experiment.yaml serial.timeout_s
SOURCE_SUFFIXES = (".cpp", ".hpp", ".h")


def objdump() -> Path:
    tools = Path(os.environ.get("LOCALAPPDATA", "")) / "Arduino15" / "packages" / "arduino" / "tools" / "avr-gcc"
    found = sorted(tools.glob("*/bin/avr-objdump.exe")) or sorted(tools.glob("*/bin/avr-objdump"))
    if not found:
        sys.exit(f"avr-objdump not found under {tools} (it comes with the arduino:avr core)")
    return found[-1]


OBJDUMP = objdump()


def line_rows(elf: Path, codes: dict[str, int]) -> list[list[int]]:
    """ELF line table as [byte_address, file_code, line]; rows of other files keep code 0 to end the ranges."""
    out = subprocess.run([str(OBJDUMP), "--dwarf=decodedline", str(elf)], capture_output=True, text=True,
                         check=True).stdout
    rows = []
    for line in out.splitlines():
        m = re.match(r"^(\S+)\s+(\d+)\s+(0x[0-9a-f]+)\s*$", line)
        if m:
            rows.append([int(m[3], 16), codes.get(m[1], 0), int(m[2])])
    return rows


def trace(build: Path, codes: dict[str, int], options: dict, tmp: Path) -> dict:
    linemap = tmp / f"linemap_{abs(hash(str(build)))}.json"
    linemap.write_text(json.dumps({"rows": line_rows(build / "test_main.ino.elf", codes)}), encoding="utf-8")
    out = subprocess.run(["node", str(DEMO / "trace.js"), str(build / "test_main.ino.hex"), str(linemap),
                          str(TIMEOUT_MS), json.dumps(options)], capture_output=True, text=True, check=True).stdout
    linemap.unlink()
    return json.loads(out)


def verdict(tr: dict) -> str:
    """Same rule as the harness: timeout, any FAIL line, or fewer passed than total -> the test failed."""
    text = [s[2] for s in tr["serial"]]
    if tr["timeout"]:
        return "timeout"
    end = next((t for t in text if t.startswith("@RUN_END")), "")
    passed, _, total = end.split(" ", 1)[1].partition("/") if end else ("0", "", "1")
    return "fail" if any(" FAIL " in t for t in text) or passed != total else "pass"


def encode(tr: dict) -> dict:
    """Compact trace: event lists flattened, cycles stored as deltas. Test lines are positive line numbers, module
    lines negative: -(file_index * 10000 + line), file_index counting the module files from 1."""
    def flat(events):
        out, prev = [], 0
        for e in events:
            out.append(e[0] - prev)
            prev = e[0]
            out.extend(e[1:])
        return out
    lines = [(c, line if f == 1 else -((f - 1) * 10000 + line)) for c, f, line in tr["lines"]]
    return {"c": tr["cycles"], "to": tr["timeout"], "tr": tr["truncated"], "p": flat(tr["pins"]),
            "v": flat(tr["servo"]), "d": flat(tr["lcd"]), "g": flat(tr["glyphs"]), "l": flat(lines), "s": tr["serial"]}


def tests_of(source: str) -> list[dict]:
    lines = source.splitlines()
    starts = [(i + 1, m[1]) for i, l in enumerate(lines) if (m := re.match(r"\s*TEST\((\w+)\)", l))]
    setup = next((i + 1 for i, l in enumerate(lines) if l.startswith("void setup")), len(lines) + 1)
    return [{"name": name, "from": start, "to": (starts[k + 1][0] if k + 1 < len(starts) else setup) - 1}
            for k, (start, name) in enumerate(starts)]


def truthy(v) -> bool:
    return v is True or str(v).lower() == "true"


def module_info(exp: dict, module_id: str) -> dict:
    meta = yaml.safe_load((ROOT / "benchmark" / module_id / "module.yaml").read_text(encoding="utf-8"))
    diagram = json.loads(Path(str(exp["diagrams"]).format(m=module_id)).read_text(encoding="utf-8"))
    types = {p["id"]: p["type"] for p in diagram["parts"]}
    servo, lcd = [], {}
    for a, b, *_ in diagram["connections"]:
        for x, y in ((a, b), (b, a)):
            if not x.startswith("uno:") or ":" not in y:
                continue
            raw, (part, pin) = x.split(":")[1], y.split(":")
            n = 14 + int(raw[1:]) if re.fullmatch(r"A\d", raw) else int(raw) if raw.isdigit() else None
            if n is None:
                continue
            if types.get(part) == "wokwi-servo" and pin == "PWM":
                servo.append(n)
            if types.get(part) == "wokwi-lcd1602" and pin in ("RS", "E", "D4", "D5", "D6", "D7"):
                lcd[pin] = n
    coils = sorted((int(m[1]), p["pin"]) for p in meta["pins"] if (m := re.search(r"\bIN(\d)\b", str(p.get("role", ""))))
                   and "wokwi-stepper-motor" in types.values())
    options = {"servo": sorted(servo)}
    if len(lcd) == 6:
        options["lcd"] = {"rs": lcd["RS"], "e": lcd["E"], "d": [lcd["D4"], lcd["D5"], lcd["D6"], lcd["D7"]]}
    return {"id": module_id, "name": meta.get("name", module_id), "experiment": exp["label"], "diagram": diagram,
            "servo_pins": sorted(servo), "coil_pins": [pin for _, pin in coils], "options": options,
            "backend": meta.get("backend", "avr8js"), "complexity": meta.get("complexity", "")}


def module_files(attempt: Path) -> list[str]:
    """Module source files compiled with the test, logic.cpp first (the line table names them by basename)."""
    names = [p.name for p in (attempt / "test_main").iterdir()
             if p.suffix in SOURCE_SUFFIXES and not p.name.startswith("test_runner")]
    return sorted(names, key=lambda n: (n != "logic.cpp", n.endswith((".h", ".hpp")), n.lower()))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dump", action="store_true", help="also write demo/build/index.json")
    args = ap.parse_args()

    modules: dict[str, dict] = {}
    runs, jobs, skipped = [], [], set()
    for exp in EXPERIMENTS:
        for run in sorted(p for p in exp["runs"].iterdir() if p.is_dir()):
            module_id, rag, model = run.name.split("__")[:3]
            if module_id not in modules:
                info = module_info(exp, module_id) if Path(str(exp["diagrams"]).format(m=module_id)).exists() else None
                if info is None or ("backend" in exp and info["backend"] != exp["backend"]):
                    skipped.add(module_id)
                    continue
                modules[module_id] = info
            elif module_id in skipped:
                continue
            attempts = sorted((run / "work").glob("attempt_*"), key=lambda p: int(p.name.split("_")[1]))
            attempt = attempts[-1]
            built = (attempt / "build" / "test_main.ino.hex").exists()
            mutants_file = run / "mutants.json"
            mutants = json.loads(mutants_file.read_text(encoding="utf-8"))["per_mutant"] if mutants_file.exists() else []
            for m in mutants:
                m.update(id=int(m["id"]), line=int(m["line"]), killed=truthy(m["killed"]), file=m.get("file", "logic.cpp"))
            files = module_files(attempt)
            codes = {"test_main.ino": 1, **{name: 2 + i for i, name in enumerate(files)}}
            runs.append({"dir": run, "exp": exp, "module": module_id, "model": model, "rag": rag, "attempt": attempt,
                         "built": built, "mutants": mutants, "files": files,
                         "test_source": (run / "test_main.ino").read_text(encoding="utf-8")})
            if not built:
                continue
            options = modules[module_id]["options"]
            r = len(runs) - 1
            jobs.append((r, 0, attempt / "build", codes, options))
            jobs += [(r, m["id"], run / "work" / "mutation" / f"mutant_{m['id']}" / "build", codes, options) for m in mutants]
    if skipped:
        print(f"skipped (not replayable on avr8js): {', '.join(sorted(skipped))}")

    print(f"{len(runs)} runs, {len(jobs)} firmwares to re-execute")
    traces: dict[tuple[int, int], dict] = {}
    with tempfile.TemporaryDirectory() as tmp, ThreadPoolExecutor(max_workers=min(12, os.cpu_count() or 4)) as pool:
        futures = {pool.submit(trace, build, codes, options, Path(tmp)): (r, fw) for r, fw, build, codes, options in jobs}
        for done, (future, key) in enumerate(futures.items(), 1):
            traces[key] = future.result()
            if done % 100 == 0:
                print(f"  {done}/{len(jobs)}")

    problems, index_modules, blobs = [], {}, []
    for r, run in enumerate(runs):
        mod = modules[run["module"]]
        mod.setdefault("files", [{"name": n, "source": (run["attempt"] / "test_main" / n).read_text(encoding="utf-8")}
                                 for n in run["files"]])
        entry = {"run": run["dir"].name, "path": run["dir"].relative_to(ROOT).as_posix(),
                 "model": run["model"], "model_label": MODELS.get(run["model"], run["model"]),
                 "rag": run["rag"], "built": run["built"], "tests": tests_of(run["test_source"]),
                 "test_source": run["test_source"], "n_mutants": len(run["mutants"]), "killed": 0}
        index_modules.setdefault(run["module"], []).append(entry)
        if not run["built"]:
            entry.update(blob=-1, valid=False, passed=0, total=len(entry["tests"]))
            continue
        original = traces[(r, 0)]
        serial_log = (run["dir"] / "serial.log").read_text(encoding="utf-8").strip().splitlines()
        if [s[2] for s in original["serial"]] != serial_log:
            problems.append(f"{run['dir'].name}: re-executed original does not match serial.log")
        catalog = mod.setdefault("mutants", {})
        blob = {"o": encode(original), "m": {}}
        for m in run["mutants"]:
            tr = traces[(r, m["id"])]
            got = verdict(tr)
            if got != m["verdict"]:
                problems.append(f"{run['dir'].name} mutant {m['id']}: re-executed {got}, experiment {m['verdict']}")
            meta = {k: m[k] for k in ("id", "operator", "file", "line", "before", "after")}
            if m["id"] not in catalog:
                meta["source"] = (run["dir"] / "mutants" / str(m["id"]) / m["file"]).read_text(encoding="utf-8")
                catalog[m["id"]] = meta
            elif any(catalog[m["id"]][k] != meta[k] for k in ("operator", "file", "line", "before", "after")):
                problems.append(f"{run['dir'].name} mutant {m['id']}: differs from other runs of {run['module']}")
            entry["killed"] += m["killed"]
            blob["m"][m["id"]] = {"k": m["killed"], "vd": got, "t": encode(tr)}
        end = next((s[2] for s in original["serial"] if s[2].startswith("@RUN_END")), "@RUN_END 0/0")
        passed, total = map(int, end.split(" ", 1)[1].split("/"))
        entry.update(blob=len(blobs), valid=verdict(original) == "pass", passed=passed, total=total)
        blobs.append(blob)
    if problems:
        print("\n".join(problems))
        return 1
    n_orig = sum(1 for r in runs if r["built"])
    print(f"checked: {n_orig} serial logs identical, {sum(len(r['mutants']) for r in runs)} mutant verdicts identical")

    order = list(MODELS)
    index = {"start": START, "modules": [
        {**{k: v for k, v in modules[m].items() if k not in ("mutants", "options", "backend")},
         "mutants": [modules[m]["mutants"][i] for i in sorted(modules[m].get("mutants", {}))],
         "runs": sorted(index_modules[m], key=lambda x: (order.index(x["model"]) if x["model"] in order else 9, x["rag"]))}
        for m in modules]}

    def js(obj) -> str:
        return json.dumps(obj, ensure_ascii=False, separators=(",", ":")).replace("</", "<\\/")
    blob_tags = "\n".join(f'<script type="application/json" id="run-{i}">{js(b)}</script>' for i, b in enumerate(blobs))
    sizes = sorted(((len(js(b)), i) for i, b in enumerate(blobs)), reverse=True)
    cut = sum(t["truncated"] for t in traces.values())
    print(f"run data {sum(s for s, _ in sizes) / 1e6:.1f} MB; {cut} traces truncated")
    if args.dump:
        (DEMO / "build").mkdir(exist_ok=True)
        (DEMO / "build" / "index.json").write_text(js(index), encoding="utf-8")
    template = (DEMO / "player_template.html").read_text(encoding="utf-8")
    bundle = (DEMO / "vendor" / "wokwi-elements.bundle.min.js").read_text(encoding="utf-8")
    player = (DEMO / "player.js").read_text(encoding="utf-8")
    html = (template.replace("/*__WOKWI_ELEMENTS__*/", bundle.replace("</script", "<\\/script"))
            .replace("/*__PLAYER_JS__*/", player.replace("</script", "<\\/script"))
            .replace("__PLAYER_INDEX__", js(index))
            .replace("<!--__RUN_DATA__-->", blob_tags))
    out = DEMO / "test_player.html"
    out.write_text(html, encoding="utf-8")
    print(f"{out} ({out.stat().st_size / 1e6:.2f} MB)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
