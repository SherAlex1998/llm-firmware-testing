"""Fixed mutant sets of experiment 2, shared by all conditions and repetitions.

    python -m framework.mutation.catalog --exp2                    # build missing catalogs, smoke-test avr8js modules
    python -m framework.mutation.catalog --exp2 --smoke all        # also smoke-test wokwi modules (uses Wokwi quota)
    python -m framework.mutation.catalog --exp2 --modules a,b --force

A mutant is `trivial` when the module's own tests/smoke_test.ino kills it. Existing catalogs are only rebuilt with --force.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import sys
import tempfile
from pathlib import Path

from framework.common.config import load_config, resolve_path
from framework.harness.sketch import load_module_yaml, module_sources, resolve_module_dir
from framework.mutation.mutator import Mutant, Site, make_mutants

SITE_FIELDS = ("operator", "start", "end", "original", "mutated", "line")


def catalog_root(cfg: dict | None = None) -> Path:
    cfg = cfg or load_config()
    return resolve_path(cfg["experiment2"].get("mutants_catalog", "results/exp2/mutants_catalog"))


def module_sha256(module_dir: Path) -> str:
    h = hashlib.sha256()
    for name, text in sorted(module_sources(module_dir).items()):
        h.update(f"{name}\0{text}\0".encode("utf-8"))
    return h.hexdigest()


def build_catalog(module: str, root: Path, force: bool = False, **make_kw) -> dict:
    """Writes <root>/<module>/ unless it exists (or force); returns catalog.json content."""
    module_dir = resolve_module_dir(module)
    out = Path(root) / module_dir.name
    if (out / "catalog.json").exists() and not force:
        return json.loads((out / "catalog.json").read_text(encoding="utf-8"))
    cfg = load_config()["mutation"]
    mutants = make_mutants(module_dir, work_dir=Path(tempfile.mkdtemp(prefix="catalog_")), **make_kw)
    for m in mutants:
        (out / str(m.id)).mkdir(parents=True, exist_ok=True)
        (out / str(m.id) / m.file).write_text(m.source, encoding="utf-8")
    cat = {"module_id": module_dir.name, "seed": cfg["seed"], "operators": cfg["operators"],
           "module_sha256": module_sha256(module_dir), "smoke": None,
           "mutants": [{"id": m.id, "file": m.file, **{f: getattr(m.site, f) for f in SITE_FIELDS},
                        "before": m.before, "after": m.after, "trivial": None, "smoke_verdict": None,
                        "equivalent": None} for m in mutants]}
    _save(out, cat)
    return cat


def _save(out: Path, cat: dict) -> None:
    out.mkdir(parents=True, exist_ok=True)
    (out / "catalog.json").write_text(json.dumps(cat, ensure_ascii=False, indent=1), encoding="utf-8")


def load_catalog(module: str, root: Path | None = None) -> tuple[Mutant, ...]:
    """The module's mutants as make_mutants() returned them; refuses a catalog built from other sources."""
    module_dir = resolve_module_dir(module)
    out = Path(root or catalog_root()) / module_dir.name
    if not (out / "catalog.json").exists():
        raise FileNotFoundError(f"no mutant catalog for {module_dir.name}: run python -m framework.mutation.catalog --exp2")
    cat = json.loads((out / "catalog.json").read_text(encoding="utf-8"))
    if cat["module_sha256"] != module_sha256(module_dir):
        raise ValueError(f"{module_dir.name} changed after its mutant catalog was built; rebuild it with --force")
    return tuple(Mutant(e["id"], Site(**{f: e[f] for f in SITE_FIELDS}),
                        (out / str(e["id"]) / e["file"]).read_text(encoding="utf-8"), e["before"], e["after"], e["file"])
                 for e in cat["mutants"])


def smoke_mark(module: str, root: Path, backend, timeout_s: int | None = None) -> dict:
    """Runs every not yet checked mutant against tests/smoke_test.ino; trivial = the smoke test kills it."""
    from framework.harness.compile import compile_sketch
    from framework.harness.sketch import stage_test_sketch
    from framework.mutation.run_mutation import mutation_score
    module_dir = resolve_module_dir(module)
    out = Path(root) / module_dir.name
    cat = json.loads((out / "catalog.json").read_text(encoding="utf-8"))
    todo = [m for m in load_catalog(module, root) if cat["mutants"][m.id - 1]["trivial"] is None]
    if not todo:
        return cat
    smoke = (module_dir / "tests" / "smoke_test.ino").read_text(encoding="utf-8")
    timeout_s = timeout_s or load_config()["serial"]["timeout_s"]
    work = Path(tempfile.mkdtemp(prefix="smoke_"))
    comp = compile_sketch(stage_test_sketch(module_dir, smoke, work / "original"), work / "original" / "build")
    base = backend.run(comp.hex_path, timeout_s).verdict if comp.ok else "error"
    cat["smoke"] = {"backend": backend.name, "original_verdict": base}
    if base == "pass":
        res = mutation_score(module_dir, smoke, todo, backend, timeout_s=timeout_s, work_dir=work)
        for r in res.per_mutant:
            cat["mutants"][r.id - 1].update(trivial=r.killed, smoke_verdict=r.verdict)
    _save(out, cat)
    return cat


def write_summary(root: Path, modules: list[str]) -> Path:
    path = Path(root) / "catalog.csv"
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["module_id", "backend", "mutants", "files", "operators", "smoke_backend", "smoke_original",
                    "trivial", "not_trivial", "unchecked"])
        for module in modules:
            cat = json.loads((Path(root) / module / "catalog.json").read_text(encoding="utf-8"))
            ms, smoke = cat["mutants"], cat["smoke"] or {}
            ops = {}
            for m in ms:
                ops[m["operator"]] = ops.get(m["operator"], 0) + 1
            w.writerow([module, load_module_yaml(resolve_module_dir(module)).get("backend"), len(ms),
                        " ".join(sorted({m["file"] for m in ms})), " ".join(f"{k}:{v}" for k, v in sorted(ops.items())),
                        smoke.get("backend", ""), smoke.get("original_verdict", ""),
                        sum(m["trivial"] is True for m in ms), sum(m["trivial"] is False for m in ms),
                        sum(m["trivial"] is None for m in ms)])
    return path


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exp2", action="store_true", required=True, help="experiment 2 modules (experiment2.modules)")
    ap.add_argument("--modules", help="comma-separated subset")
    ap.add_argument("--force", action="store_true", help="rebuild existing catalogs (changes the mutant sets)")
    ap.add_argument("--smoke", choices=["avr8js", "all", "none"], default="avr8js",
                    help="which modules get the smoke-test marking (default: avr8js only, no Wokwi quota)")
    args = ap.parse_args()

    from framework.runner.experiment import _wokwi, exp2_backends
    cfg = load_config()
    root = catalog_root(cfg)
    modules = args.modules.split(",") if args.modules else cfg["experiment2"]["modules"]
    backend_for = exp2_backends(cfg, None, resolve_path(cfg["experiment2"]["results_dir"]) / "wokwi_usage.csv")
    for module in modules:
        cat = build_catalog(module, root, force=args.force)
        backend = load_module_yaml(resolve_module_dir(module)).get("backend", "avr8js")
        if args.smoke == "all" or args.smoke == backend:
            b = backend_for(module)
            if wokwi := _wokwi(b):
                wokwi.context = {"run_id": f"catalog__{module}", "kind": "smoke", "module_id": module}
            cat = smoke_mark(module, root, b)
        ms = cat["mutants"]
        print(f"{module}: {len(ms)} mutants in {sorted({m['file'] for m in ms})}; trivial "
              f"{sum(m['trivial'] is True for m in ms)}, not {sum(m['trivial'] is False for m in ms)}, "
              f"unchecked {sum(m['trivial'] is None for m in ms)}")
    print("summary:", write_summary(root, [resolve_module_dir(m).name for m in modules]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
