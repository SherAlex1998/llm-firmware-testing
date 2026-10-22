"""python -m framework.generator.generate --module blink_fixture --model claude-sonnet-5 [--run] [--no-cache]"""
from __future__ import annotations

import argparse
import json
import sys
import tempfile
from dataclasses import asdict, dataclass
from datetime import datetime
from pathlib import Path

from framework.common.config import REPO_ROOT, load_config
from framework.generator.llm_clients import LLMClient, LLMError, make_client
from framework.generator.prompts import (build_repair_prompt, build_system_prompt, build_user_prompt,
                                         extract_code, module_context)
from framework.harness.compile import compile_sketch
from framework.harness.sketch import load_module_yaml, resolve_module_dir, stage_test_sketch


@dataclass
class Attempt:
    kind: str                 # generate | repair
    prompt: str               # user prompt of this call (the system prompt is in GenResult)
    response: str
    code: str
    compile_ok: bool
    compile_stderr: str
    tokens_in: int
    tokens_out: int
    latency_s: float
    cached: bool
    stop_reason: str


@dataclass
class GenResult:
    module_id: str
    model_id: str
    rag: bool
    system_prompt: str
    attempts: list[Attempt]
    hex_path: Path | None     # compiled firmware; None when the last attempt did not compile
    cost_usd: float

    @property
    def code(self) -> str:
        return self.attempts[-1].code

    @property
    def compile_ok(self) -> bool:
        return self.attempts[-1].compile_ok

    @property
    def gen_attempts(self) -> int:
        return len(self.attempts)

    @property
    def tokens_in(self) -> int:
        return sum(a.tokens_in for a in self.attempts)

    @property
    def tokens_out(self) -> int:
        return sum(a.tokens_out for a in self.attempts)


def generate_test(module: str | Path, client: LLMClient, rag_context: str | None = None,
                  work_dir: Path | None = None, max_repair_attempts: int | None = None) -> GenResult:
    """LLM -> extract code -> stage -> compile; on failure repeat with the compiler output (max_repair_attempts)."""
    module_dir = resolve_module_dir(module)
    if max_repair_attempts is None:
        max_repair_attempts = load_config()["generation"]["max_repair_attempts"]
    work = Path(work_dir or tempfile.mkdtemp(prefix="gen_"))
    system, context = build_system_prompt(module_dir), module_context(module_dir)
    prompt, attempts = build_user_prompt(module_dir, rag_context), []
    for n in range(1, max_repair_attempts + 2):
        resp = client.generate(system, prompt)
        code = extract_code(resp.text) + "\n"
        sketch = stage_test_sketch(module_dir, code, work / f"attempt_{n}")
        comp = compile_sketch(sketch, work / f"attempt_{n}" / "build")
        # strip the sketch path so that repair prompts, and with them the cache keys, do not depend on the run folder
        stderr = comp.stderr
        for prefix in {str(sketch.resolve()), str(sketch), sketch.resolve().as_posix()}:
            stderr = stderr.replace(prefix + "\\", "").replace(prefix + "/", "")
        attempts.append(Attempt("generate" if n == 1 else "repair", prompt, resp.text, code, comp.ok, stderr,
                                resp.tokens_in, resp.tokens_out, resp.latency_s, resp.cached, resp.stop_reason))
        if comp.ok:
            break
        prompt = build_repair_prompt(code, stderr, context)
    cost = client.cost_usd(sum(a.tokens_in for a in attempts), sum(a.tokens_out for a in attempts))
    return GenResult(load_module_yaml(module_dir)["id"], client.model_id, bool(rag_context), system, attempts,
                     comp.hex_path, cost)


def save_artifacts(result: GenResult, out_dir: Path) -> list[str]:
    """Final sketch, prompts, replies, per-attempt sketches and compiler output, gen.json; returns file names."""
    out = Path(out_dir)
    out.mkdir(parents=True, exist_ok=True)
    files = {"test_main.ino": result.code, "prompt_system.txt": result.system_prompt}
    for n, a in enumerate(result.attempts, 1):
        files[f"attempt_{n}_prompt.txt"] = a.prompt
        files[f"attempt_{n}_response.txt"] = a.response
        files[f"attempt_{n}_test_main.ino"] = a.code
        if a.compile_stderr.strip():
            files[f"attempt_{n}_compile_stderr.txt"] = a.compile_stderr
    meta = {"module_id": result.module_id, "model_id": result.model_id, "rag": result.rag,
            "compile_ok": result.compile_ok, "gen_attempts": result.gen_attempts,
            "tokens_in": result.tokens_in, "tokens_out": result.tokens_out, "cost_usd": result.cost_usd,
            "attempts": [{k: v for k, v in asdict(a).items() if k not in ("prompt", "response", "code", "compile_stderr")}
                         for a in result.attempts]}
    files["gen.json"] = json.dumps(meta, indent=1)
    for name, text in files.items():
        (out / name).write_text(text, encoding="utf-8")
    return list(files)


def main() -> int:
    cfg = load_config()
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--module", required=True, help="benchmark id, fixture id or module folder")
    ap.add_argument("--model", default=cfg["models"][0]["id"], help="model id from configs/experiment.yaml")
    ap.add_argument("--rag", action="store_true", help="retrieve a <reference> block from kb/")
    ap.add_argument("--rag-context", help="RAG on with a fixed <reference> text file instead of retrieval")
    ap.add_argument("--no-cache", action="store_true", help="always call the API")
    ap.add_argument("--rep", type=int, default=1, help="repetition number; rep >= 2 bypasses the rep-1 cache entry")
    ap.add_argument("--out", help="artifact folder (default results/gen/<module>__<model>__<ts>)")
    ap.add_argument("--run", action="store_true", help="also execute the compiled test on the default backend")
    args = ap.parse_args()

    rag, rag_result = Path(args.rag_context).read_text(encoding="utf-8") if args.rag_context else None, None
    if args.rag and not rag:
        from framework.rag.retrieve import retrieve_context
        rag_result = retrieve_context(args.module)
        rag = rag_result.context
    try:
        res = generate_test(args.module, make_client(args.model, cfg, not args.no_cache, args.rep), rag)
    except LLMError as exc:
        print("error:", exc)
        return 2
    out = Path(args.out or REPO_ROOT / "results" / "gen" / f"{res.module_id}__{res.model_id}__{datetime.now():%Y%m%d-%H%M%S}")
    save_artifacts(res, out)
    if rag_result:
        (out / "rag_chunks.json").write_text(json.dumps(asdict(rag_result), indent=1), encoding="utf-8")
        print("rag: " + ", ".join(f"{c['source_path']} ({c['similarity']:.2f})" for c in rag_result.chunks))
    for n, a in enumerate(res.attempts, 1):
        print(f"attempt {n} {a.kind}: compile_ok={a.compile_ok} tokens={a.tokens_in}/{a.tokens_out} "
              f"latency={a.latency_s:.1f}s cached={a.cached} stop={a.stop_reason}")
    print(f"result: compile_ok={res.compile_ok} attempts={res.gen_attempts} tokens={res.tokens_in}/{res.tokens_out} "
          f"cost=${res.cost_usd:.4f} -> {out}")
    if not res.compile_ok:
        print(res.attempts[-1].compile_stderr.strip()[:3000])
        return 1
    if args.run:
        from framework.harness.backends import run_firmware
        r = run_firmware(res.hex_path)
        (out / "serial.log").write_text(r.serial_log, encoding="utf-8")
        print(f"run ({r.backend}): verdict={r.verdict} tests={r.tests_passed}/{r.tests_total} {r.error}".rstrip())
        return 0 if r.verdict == "pass" else 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
