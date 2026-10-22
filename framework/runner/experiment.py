"""Run the experiment matrix: generate a test, compile, execute, run the mutants, record everything.

    python -m framework.runner.experiment --all [--resume]                  # experiment 1
    python -m framework.runner.experiment --plan exp2 --reps 1 --max-cost 20  # experiment 2
"""
from __future__ import annotations

import argparse
import json
import random
import sys
import traceback
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from functools import lru_cache
from pathlib import Path
from typing import Callable

from framework.common.config import REPO_ROOT, load_config, resolve_path
from framework.common.protocol import ERROR, FAIL, PASS
from framework.generator.generate import generate_test, save_artifacts
from framework.generator.llm_clients import LLMClient, make_client
from framework.harness.backends import get_backend

MAX_CONSECUTIVE_ERRORS = 3


@dataclass(frozen=True)
class Condition:
    rag: str                  # "off" | "on"
    model: str

    def __str__(self) -> str:
        return f"rag={self.rag} model={self.model}"


def conditions(cfg: dict) -> list[Condition]:
    rags = ["on" if r in (True, "on") else "off" for r in cfg["experiment"]["conditions"]["rag"]]
    return [Condition(r, m["id"]) for m in cfg["models"] for r in rags]


def parse_condition(text: str) -> Condition:
    kv = dict(part.split("=", 1) for part in text.split(","))
    return Condition(kv["rag"], kv["model"])


def benchmark_modules() -> list[str]:
    """Experiment 1 modules (experiment 2 modules are listed in experiment2.modules)."""
    import yaml
    return sorted(p.parent.name for p in (REPO_ROOT / "benchmark").glob("*/module.yaml")
                  if yaml.safe_load(p.read_text(encoding="utf-8")).get("experiment") != 2)


def plan_runs(modules: list[str], conds: list[Condition], seed: int) -> list[tuple[str, Condition]]:
    pairs = [(m, c) for m in modules for c in conds]
    random.Random(seed).shuffle(pairs)
    return pairs


def plan_exp2(modules: list[str], conds: list[Condition], reps: int, seed: int,
              block_by_rep: bool = True) -> list[tuple[str, Condition, int]]:
    """Experiment 2: module x condition x rep; with block_by_rep all of rep 1 comes first, then rep 2, ..."""
    if not block_by_rep:
        jobs = [(m, c, r) for r in range(1, reps + 1) for m in modules for c in conds]
        random.Random(seed).shuffle(jobs)
        return jobs
    return [(m, c, r) for r in range(1, reps + 1) for m, c in plan_runs(modules, conds, seed + r - 1)]


def write_plan(jobs: list[tuple[str, Condition, int]], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = ["order,module_id,rag,model,rep"] + [f"{i},{m},{c.rag},{c.model},{r}" for i, (m, c, r) in enumerate(jobs, 1)]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


class RetryOnce:
    """Backend wrapper: an infrastructure error (verdict error) is retried once."""

    def __init__(self, backend):
        self.backend, self.name = backend, backend.name

    def run(self, firmware, timeout_s):
        result = self.backend.run(firmware, timeout_s)
        return self.backend.run(firmware, timeout_s) if result.verdict == ERROR else result


def _wokwi(backend):
    """The WokwiBackend under a RetryOnce wrapper, else None."""
    inner = getattr(backend, "backend", backend)
    return inner if getattr(inner, "name", "") == "wokwi" else None


# official Arduino libraries do not count as third-party (hypothesis H5)
OFFICIAL_LIBS = {"LiquidCrystal", "EEPROM", "Wire", "SPI", "Servo", "Stepper", "SoftwareSerial"}


def module_facts(module_id: str) -> dict:
    """Experiment 2 record fields taken from module.yaml; uses_libs = at least one third-party library."""
    from framework.harness.sketch import load_module_yaml, resolve_module_dir
    meta = load_module_yaml(resolve_module_dir(module_id))
    third_party = [lib for lib in meta.get("libraries") or [] if lib["name"] not in OFFICIAL_LIBS]
    return {"complexity": meta.get("complexity"), "uses_libs": bool(third_party),
            "has_devices": bool(meta.get("devices")),
            "libraries": [f"{lib['name']}@{lib['version']}" for lib in meta.get("libraries") or []]}


def _now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


def _event(run_dir: Path, event: str, **data) -> None:
    with open(run_dir / "run.jsonl", "a", encoding="utf-8") as f:
        f.write(json.dumps({"event": event, "ts": _now(), **data}, ensure_ascii=False) + "\n")


@lru_cache(maxsize=None)
def module_mutants(module_id: str) -> tuple:
    from framework.mutation.mutator import make_mutants
    return tuple(make_mutants(module_id))


def mutate(module_id: str, run_dir: Path, backend, mutants_for: Callable[[str], tuple]) -> dict:
    from framework.mutation.run_mutation import mutation_score, save_mutation
    mutants = list(mutants_for(module_id))
    test_source = (run_dir / "test_main.ino").read_text(encoding="utf-8")
    res = mutation_score(module_id, test_source, mutants, backend, work_dir=run_dir / "work" / "mutation")
    save_mutation(res, mutants, run_dir)
    _event(run_dir, "mutation", mutants_total=res.mutants_total, killed=res.killed, score=res.score,
           warnings=res.warnings)
    return {"mutants_total": res.mutants_total, "killed": res.killed, "score": res.score}


def mutate_pruned(module_id: str, run_dir: Path, backend, mutants_for: Callable[[str], tuple],
                  failed: list[str]) -> dict:
    """Experiment 2 secondary metric for a test that failed on the original: drop the failed tests, run the rest."""
    from framework.mutation.run_mutation import pruned_mutation_score
    test_source = (run_dir / "test_main.ino").read_text(encoding="utf-8")
    res = pruned_mutation_score(module_id, test_source, failed, list(mutants_for(module_id)), backend,
                                work_dir=run_dir / "work" / "pruned")
    (run_dir / "test_pruned.ino").write_text(res.source, encoding="utf-8")
    m = res.mutation
    if m:
        (run_dir / "mutants_pruned.json").write_text(json.dumps(asdict(m), ensure_ascii=False, indent=1), encoding="utf-8")
    out = {"status": res.status, "disabled": res.disabled, "tests_kept": res.tests_kept,
           "mutants_total": m.mutants_total if m else None, "killed": m.killed if m else None,
           "score": m.score if m else None}
    _event(run_dir, "mutation_pruned", **out, warnings=m.warnings if m else [])
    return out


def pruned_from_strict(mutation: dict | None, tests_total: int) -> dict | None:
    """A test that passed on the original is its own pruned test: same mutants, same score."""
    return {**mutation, "status": PASS, "disabled": [], "tests_kept": tests_total} if mutation else None


def run_one(module_id: str, cond: Condition, runs_dir: Path, client: LLMClient, backend, *,
            retrieve: Callable | None = None, mutants_for: Callable[[str], tuple] = module_mutants,
            skip_mutation: bool = False, pilot: bool = False, timeout_s: int | None = None,
            rep: int | None = None, exp2: dict | None = None) -> dict:
    """rep / exp2 (experiment 2 only): repetition number and {kb_version, mutation_reps} for the record."""
    start = datetime.now(timezone.utc)
    run_id = f"{module_id}__{cond.rag}__{cond.model}__{f'r{rep}__' if rep else ''}{start:%Y%m%d-%H%M%S}"
    run_dir = Path(runs_dir) / run_id
    run_dir.mkdir(parents=True, exist_ok=True)
    rec = {"run_id": run_id, "module_id": module_id, "condition": {"rag": cond.rag, "model": cond.model},
           "pilot": pilot, "ts_start": start.isoformat(timespec="seconds"), "ts_end": None, "duration_s": None,
           "gen_attempts": 0, "tokens_in": 0, "tokens_out": 0, "cost_usd": 0.0, "cost_spent_usd": 0.0,
           "cache_hits": 0, "compile_ok": False, "exec_backend": None, "exec_verdict": None, "tests_total": 0,
           "tests_passed": 0, "mutation": None, "rag": None, "error": None, "artifacts": []}
    if rep:
        rec.update(experiment=2, rep=rep, **module_facts(module_id), kb_version=(exp2 or {}).get("kb_version"),
                   max_tokens=(exp2 or {}).get("max_tokens"), wokwi_wall_s=0.0, mutation_skipped=False,
                   mutation_pruned=None)
        if rep not in (exp2 or {}).get("mutation_reps", [rep]):
            skip_mutation = rec["mutation_skipped"] = True
    wokwi = _wokwi(backend)
    wall0 = wokwi.wall_total if wokwi else 0.0
    if wokwi:
        wokwi.context = {"run_id": run_id, "kind": "test", "module_id": module_id}
    _event(run_dir, "start", run_id=run_id, module_id=module_id, condition=rec["condition"])
    try:
        context = None
        if cond.rag == "on":
            if retrieve is None:
                from framework.rag.retrieve import retrieve_context as retrieve
            rag = retrieve(module_id)
            context = rag.context
            (run_dir / "rag_chunks.json").write_text(json.dumps(asdict(rag), ensure_ascii=False, indent=1),
                                                     encoding="utf-8")
            sims = [c["similarity"] for c in rag.chunks]
            rec["rag"] = {"n_chunks": len(sims), "similarity_mean": round(sum(sims) / len(sims), 4) if sims else None,
                          "similarity_max": max(sims, default=None), "sources": [c["source_path"] for c in rag.chunks]}
            _event(run_dir, "rag", **rec["rag"])

        gen = generate_test(module_id, client, context, work_dir=run_dir / "work")
        save_artifacts(gen, run_dir)
        live = [a for a in gen.attempts if not a.cached]
        rec.update(gen_attempts=gen.gen_attempts, tokens_in=gen.tokens_in, tokens_out=gen.tokens_out,
                   cost_usd=round(gen.cost_usd, 6), compile_ok=gen.compile_ok,
                   cache_hits=len(gen.attempts) - len(live),
                   cost_spent_usd=round(client.cost_usd(sum(a.tokens_in for a in live),
                                                        sum(a.tokens_out for a in live)), 6))
        _event(run_dir, "generation", gen_attempts=gen.gen_attempts, compile_ok=gen.compile_ok,
               tokens_in=gen.tokens_in, tokens_out=gen.tokens_out, cost_usd=rec["cost_usd"],
               stop_reasons=[a.stop_reason for a in gen.attempts])

        if gen.compile_ok:
            ex = backend.run(gen.hex_path, timeout_s or load_config()["serial"]["timeout_s"])
            (run_dir / "serial.log").write_text(ex.serial_log, encoding="utf-8")
            rec.update(exec_backend=backend.name, exec_verdict=ex.verdict, tests_total=ex.tests_total,
                       tests_passed=ex.tests_passed)
            _event(run_dir, "execution", verdict=ex.verdict, tests_total=ex.tests_total,
                   tests_passed=ex.tests_passed, failed=[f"{t.name}: {t.msg}" for t in ex.tests if not t.passed],
                   error=ex.error or None)
            if ex.verdict == PASS and not skip_mutation:
                if wokwi:
                    wokwi.context = {**wokwi.context, "kind": "mutant"}
                rec["mutation"] = mutate(module_id, run_dir, backend, mutants_for)
            if (exp2 or {}).get("pruned") and not skip_mutation:
                if ex.verdict == PASS:
                    rec["mutation_pruned"] = pruned_from_strict(rec["mutation"], ex.tests_total)
                elif ex.verdict == FAIL:
                    if wokwi:
                        wokwi.context = {**wokwi.context, "kind": "mutant_pruned"}
                    rec["mutation_pruned"] = mutate_pruned(module_id, run_dir, backend, mutants_for,
                                                           [t.name for t in ex.tests if not t.passed])
    except Exception as exc:                                  # isolate the run, keep the matrix going
        rec["error"] = f"{type(exc).__name__}: {exc}"
        rec["exec_verdict"] = rec["exec_verdict"] or ERROR
        _event(run_dir, "error", message=rec["error"], traceback=traceback.format_exc())
    finally:
        end = datetime.now(timezone.utc)
        rec["ts_end"] = end.isoformat(timespec="seconds")
        rec["duration_s"] = round((end - start).total_seconds(), 1)
        if rep and wokwi:
            rec["wokwi_wall_s"] = round(wokwi.wall_total - wall0, 1)
        rec["artifacts"] = sorted(p.relative_to(run_dir).as_posix() for p in run_dir.rglob("*")
                                  if p.is_file() and "work" not in p.relative_to(run_dir).parts)
        _event(run_dir, "run", **rec)
    return rec


def latest_records(runs_dir: Path) -> dict[tuple, dict]:
    best: dict[tuple, dict] = {}
    for log in sorted(Path(runs_dir).glob("*/run.jsonl")):
        records = [json.loads(line) for line in log.read_text(encoding="utf-8").splitlines() if line.strip()]
        records = [r for r in records if r.get("event") == "run"]
        if not records:
            continue
        rec = records[-1]
        key = (rec["module_id"], rec["condition"]["rag"], rec["condition"]["model"]) + ((rec["rep"],) if rec.get("rep") else ())
        old = best.get(key)
        if old is None or (old["error"] and not rec["error"]) or \
                (bool(old["error"]) == bool(rec["error"]) and rec["ts_start"] > old["ts_start"]):
            best[key] = rec
    return best


def mutation_only(runs_dir: Path, backend, mutants_for=module_mutants) -> int:
    done = 0
    for rec in latest_records(runs_dir).values():
        if rec["exec_verdict"] != PASS or rec["mutation"] is not None or rec["error"]:
            continue
        run_dir = Path(runs_dir) / rec["run_id"]
        rec = {**rec, "mutation": mutate(rec["module_id"], run_dir, backend, mutants_for), "ts_mutation": _now()}
        rec["artifacts"] = sorted(p.relative_to(run_dir).as_posix() for p in run_dir.rglob("*")
                                  if p.is_file() and "work" not in p.relative_to(run_dir).parts)
        _event(run_dir, "run", **{k: v for k, v in rec.items() if k not in ("event", "ts")})
        m = rec["mutation"]
        print(f"{rec['run_id']}: mutation {m['killed']}/{m['mutants_total']}")
        done += 1
    return done


def pruned_only(runs_dir: Path, backend_for: Callable, mutants_for: Callable[[str], tuple]) -> int:
    """Fill mutation_pruned of finished experiment 2 runs."""
    from framework.common.protocol import parse_serial_log
    done = 0
    for rec in latest_records(runs_dir).values():
        if (rec.get("experiment") != 2 or rec["error"] or rec.get("mutation_skipped") or rec.get("mutation_pruned")
                or rec["exec_verdict"] not in (PASS, FAIL)):
            continue
        run_dir = Path(runs_dir) / rec["run_id"]
        if rec["exec_verdict"] == PASS:
            pruned = pruned_from_strict(rec["mutation"], rec["tests_total"])
        else:
            backend = backend_for(rec["module_id"])
            if wokwi := _wokwi(backend):
                wokwi.context = {"run_id": rec["run_id"], "kind": "mutant_pruned", "module_id": rec["module_id"]}
            failed = [t.name for t in parse_serial_log((run_dir / "serial.log").read_text(encoding="utf-8")).tests
                      if not t.passed]
            pruned = mutate_pruned(rec["module_id"], run_dir, backend, mutants_for, failed)
        rec = {**{k: v for k, v in rec.items() if k not in ("event", "ts")}, "mutation_pruned": pruned,
               "ts_pruned": _now()}
        rec["artifacts"] = sorted(p.relative_to(run_dir).as_posix() for p in run_dir.rglob("*")
                                  if p.is_file() and "work" not in p.relative_to(run_dir).parts)
        _event(run_dir, "run", **rec)
        print(f"{rec['run_id']}: pruned {pruned and pruned['status']} "
              f"{pruned and pruned['killed']}/{pruned and pruned['mutants_total']} (disabled {pruned and pruned['disabled']})")
        done += 1
    return done


def wokwi_runs(runs_dir: Path) -> int:
    """Rows in <results>/wokwi_usage.csv (the quota log next to runs/)."""
    from framework.harness.run_wokwi import wokwi_usage_summary
    return wokwi_usage_summary(Path(runs_dir).parent / "wokwi_usage.csv")["runs"]


def exp2_backends(cfg: dict, name: str | None, usage_log: Path) -> Callable:
    """module_id -> backend (module.yaml `backend`, its diagram.json; Wokwi runs logged to usage_log)."""
    cache: dict[str, object] = {}

    def backend_for(module_id: str):
        if module_id not in cache:
            b = get_backend(name, cfg, module=module_id)
            if b.name == "wokwi":
                b.usage_log = usage_log
            ok, reason = b.check()
            if not ok:
                raise RuntimeError(f"backend {b.name} unavailable for {module_id}: {reason}")
            cache[module_id] = RetryOnce(b) if b.name != "wokwi" else b     # wokwi retries cloud failures itself
        return cache[module_id]
    return backend_for


def run_matrix(pairs: list[tuple], runs_dir: Path, clients: dict, backend, *, resume: bool = False,
               max_cost: float | None = None, max_wokwi_runs: int | None = None, **kw) -> list[dict]:
    """pairs: (module, condition) for experiment 1 or (module, condition, rep) for experiment 2.
    clients: model -> client (experiment 1) or (model, rep) -> client. backend: a backend or module_id -> backend.
    Experiment 2 also stops before max_cost (spent $) or max_wokwi_runs (rows in the quota log) is exceeded."""
    jobs = [(p[0], p[1], p[2] if len(p) > 2 else None) for p in pairs]
    finished = {k for k, r in latest_records(runs_dir).items() if not r["error"]} if resume else set()
    todo = [(m, c, r) for m, c, r in jobs if (m, c.rag, c.model) + ((r,) if r else ()) not in finished]
    if resume:
        print(f"resume: {len(jobs) - len(todo)} of {len(jobs)} runs already done")
    backend_for = backend if callable(backend) and not hasattr(backend, "run") else (lambda m: backend)
    records, errors_in_row, total = [], 0, 0.0
    for i, (module_id, cond, rep) in enumerate(todo, 1):
        if max_cost is not None and total >= max_cost:
            print(f"stopping: spent ${total:.2f} >= --max-cost {max_cost}; rerun with --resume")
            break
        if max_wokwi_runs is not None and wokwi_runs(runs_dir) >= max_wokwi_runs:
            print(f"stopping: Wokwi runs reached --max-wokwi-runs {max_wokwi_runs}; rerun with --resume")
            break
        client = clients[(cond.model, rep)] if rep else clients[cond.model]
        extra = {"rep": rep} if rep else {}
        rec = run_one(module_id, cond, runs_dir, client, backend_for(module_id), **extra, **kw)
        records.append(rec)
        total += rec["cost_spent_usd"]
        m = rec["mutation"]
        print(f"run {i}/{len(todo)}: {module_id} {cond}{f' rep={rep}' if rep else ''} -> compile_ok={rec['compile_ok']} "
              f"attempts={rec['gen_attempts']} verdict={rec['exec_verdict']} "
              f"tests={rec['tests_passed']}/{rec['tests_total']}"
              + (f" mutants={m['killed']}/{m['mutants_total']}" if m else "")
              + f" cost=${rec['cost_usd']:.4f} spent=${total:.4f}" + (f" ERROR {rec['error']}" if rec["error"] else ""),
              flush=True)
        errors_in_row = errors_in_row + 1 if rec["error"] else 0
        if errors_in_row >= MAX_CONSECUTIVE_ERRORS:
            print(f"stopping: {errors_in_row} runs in a row failed with an error; fix the cause and rerun with --resume")
            break
    return records


def main() -> int:
    cfg = load_config()
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--all", action="store_true", help="every module x every condition")
    ap.add_argument("--module", help="one module (with --condition)")
    ap.add_argument("--modules", help="comma-separated subset of modules, every condition")
    ap.add_argument("--condition", help="rag=on|off,model=<id> (default: every condition)")
    ap.add_argument("--resume", action="store_true", help="skip runs that already finished without error")
    ap.add_argument("--skip-mutation", action="store_true")
    ap.add_argument("--mutation-only", action="store_true", help="only fill the mutation field of passed runs")
    ap.add_argument("--pilot", action="store_true", help="results go to <results_dir>/pilot, marked pilot=true")
    ap.add_argument("--backend", help="avr8js | wokwi | board (default from configs/experiment.yaml)")
    ap.add_argument("--results-dir", help="default: experiment.results_dir (+ /pilot with --pilot)")
    ap.add_argument("--plan", choices=["exp2"], help="experiment 2: experiment2.modules x conditions x reps")
    ap.add_argument("--reps", help="exp2: comma-separated reps to run (default 1..experiment2.reps)")
    ap.add_argument("--max-cost", type=float, help="exp2: stop before spending more than this many $")
    ap.add_argument("--max-wokwi-runs", type=int, help="exp2: stop when the quota log has this many Wokwi runs")
    ap.add_argument("--pruned-only", action="store_true",
                    help="exp2: only fill mutation_pruned of finished runs (tests that failed on the original disabled)")
    args = ap.parse_args()
    if args.plan == "exp2":
        return main_exp2(cfg, args)

    root = resolve_path(args.results_dir or cfg["experiment"]["results_dir"])
    if args.pilot and not args.results_dir:
        root = root / "pilot"
    runs_dir = root / "runs"
    backend = get_backend(args.backend, cfg)
    ok, reason = backend.check()
    if not ok:
        print(f"backend {backend.name} unavailable: {reason}")
        return 2
    backend = RetryOnce(backend)
    if args.mutation_only:
        print(f"mutation filled for {mutation_only(runs_dir, backend)} runs")
        return 0

    modules = [args.module] if args.module else args.modules.split(",") if args.modules else \
        benchmark_modules() if args.all else None
    if not modules:
        ap.error("choose --all, --modules or --module")
    conds = [parse_condition(args.condition)] if args.condition else conditions(cfg)
    pairs = plan_runs(modules, conds, cfg["experiment"]["seed"])
    clients = {c.model: make_client(c.model, cfg) for c in conds}
    print(f"{len(pairs)} runs: {len(modules)} modules x {len(conds)} conditions -> {runs_dir}")
    records = run_matrix(pairs, runs_dir, clients, backend, resume=args.resume, skip_mutation=args.skip_mutation,
                         pilot=args.pilot)
    spent = sum(r["cost_spent_usd"] for r in records)
    print(f"done: {len(records)} runs, {sum(1 for r in records if r['error'])} with errors, spent ${spent:.4f}")
    return 0


def main_exp2(cfg: dict, args) -> int:
    """Experiment 2: per-module backend, repetitions, its own KB index and results dir."""
    from functools import partial

    from framework.harness.compile import ensure_libraries
    from framework.rag.retrieve import retrieve_context
    e2 = cfg["experiment2"]
    root = resolve_path(args.results_dir or e2["results_dir"])
    if args.pilot and not args.results_dir:
        root = root / "pilot"
    runs_dir = root / "runs"
    modules = [args.module] if args.module else args.modules.split(",") if args.modules else e2["modules"]
    if not modules:
        print("experiment2.modules is empty")
        return 2
    print("libraries:", ensure_libraries(cfg))
    conds = [parse_condition(args.condition)] if args.condition else conditions(cfg)
    reps = [int(r) for r in args.reps.split(",")] if args.reps else list(range(1, int(e2["reps"]) + 1))
    full = plan_exp2(modules, conds, int(e2["reps"]), cfg["experiment"]["seed"], e2.get("block_by_rep", True))
    jobs = [j for j in full if j[2] in reps]
    if not args.pruned_only:                                 # the whole plan (deterministic), whatever --reps runs now
        write_plan(full, root / "plan.csv")
    backend_for = exp2_backends(cfg, args.backend, root / "wokwi_usage.csv")
    if args.mutation_only:
        print("--mutation-only is not supported with --plan exp2")
        return 2
    if args.pruned_only:
        from framework.mutation.catalog import catalog_root, load_catalog
        print(f"pruned mutation filled for "
              f"{pruned_only(runs_dir, backend_for, partial(load_catalog, root=catalog_root(cfg)))} runs")
        return 0
    from framework.mutation.catalog import catalog_root, load_catalog
    for module in sorted({j[0] for j in jobs}):              # the fixed catalog must exist before any run
        load_catalog(module, catalog_root(cfg))
    clients = {(c.model, r): make_client(c.model, cfg, rep=r, max_tokens=e2.get("max_tokens")) for c in conds for r in reps}
    chroma = resolve_path(e2["kb_chroma"])
    exp2_meta = {"kb_version": kb_version(chroma), "mutation_reps": list(cfg["mutation"].get("reps") or reps),
                 "max_tokens": e2.get("max_tokens"), "pruned": bool(e2.get("pruned_mutation"))}
    print(f"{len(jobs)} runs: {len(modules)} modules x {len(conds)} conditions x reps {reps} -> {runs_dir}")
    records = run_matrix(jobs, runs_dir, clients, backend_for, resume=args.resume, skip_mutation=args.skip_mutation,
                         pilot=args.pilot, retrieve=partial(retrieve_context, chroma_path=chroma), exp2=exp2_meta,
                         mutants_for=partial(load_catalog, root=catalog_root(cfg)),
                         max_cost=args.max_cost, max_wokwi_runs=args.max_wokwi_runs)
    spent = sum(r["cost_spent_usd"] for r in records)
    print(f"done: {len(records)} runs, {sum(1 for r in records if r['error'])} with errors, spent ${spent:.4f}, "
          f"wokwi runs logged: {wokwi_runs(runs_dir)}")
    return 0


def kb_version(chroma_path: Path) -> str | None:
    """kb_version stored in the collection metadata by build_index; None until the index exists."""
    try:
        import chromadb
        from framework.rag.retrieve import COLLECTION
        return (chromadb.PersistentClient(path=str(chroma_path)).get_collection(COLLECTION).metadata or {}).get("kb_version")
    except Exception:
        return None


if __name__ == "__main__":
    sys.exit(main())
