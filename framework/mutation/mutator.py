from __future__ import annotations

import random
import re
import tempfile
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

from framework.common.config import load_config
from framework.harness.compile import compile_sketch
from framework.harness.sketch import module_sources, resolve_module_dir, stage_test_sketch

FLIP = {"==": "!=", "!=": "==", "<": ">=", ">": "<=", "<=": ">", ">=": "<"}
ARITH = {"+": "-", "-": "+", "*": "/", "/": "*", "%": "/"}
NOT_CALLS = {"if", "while", "for", "switch", "return", "sizeof",
             "delay", "delayMicroseconds",     # timing only, invisible to the tests
             "pinMode"}                        # the outcome depends on the simulator's pull-up model
COMPILE_STUB = '#include "logic.h"\nvoid setup() {}\nvoid loop() {}\n'

_NOISE = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'|^[ \t]*#[^\n]*', re.S | re.M)
_CMP = re.compile(r"==|!=|<=|>=|<<|>>|->|<|>")
_CMP_LITERAL = re.compile(r"(?<![<>=!-])(?:[=!<>]=|<|>)(?![<>=])\s*(\d+)(?![\w.])")
_ARG_LITERAL = re.compile(r"(?<=[(,{])\s*(\d+)(?=\s*[,)}])")          # call argument or initializer element
_HILO = re.compile(r"\b(HIGH|LOW)\b")
_CALL_STMT = re.compile(r"^[ \t]*(((?:[A-Za-z_]\w*(?:\.|->|::))*([A-Za-z_]\w*))\s*\([^;{}]*\)\s*;)", re.M)
_COND = re.compile(r"\b(?:if|while)\s*\(")
_LOGIC = re.compile(r"&&|\|\|")
_ARITH = re.compile(r"[\w)\]][ \t]*([-+*/%])(?![-+=>])")          # binary only: not ++ -- += -> or unary
_UNARY_AFTER = re.compile(r"\b(?:return|case)\s*$")               # `return -1`: the minus is unary


@dataclass(frozen=True)
class Site:
    operator: str
    start: int
    end: int
    original: str
    mutated: str
    line: int

    def apply(self, source: str) -> str:
        return source[:self.start] + self.mutated + source[self.end:]


@dataclass
class Mutant:
    id: int
    site: Site
    source: str               # mutated text of `file`
    before: str               # the changed line, original and mutated (for reports)
    after: str
    file: str = "logic.cpp"   # which module source the mutant replaces (experiment 2: any .cpp of `sources`)


def mutable_files(module_dir: Path) -> list[str]:
    """Module sources that get mutated: every .cpp/.c, plus headers other than logic.h (declarations and pins)."""
    return [f for f in module_sources(module_dir) if f.endswith((".cpp", ".c")) or f.endswith((".h", ".hpp")) and f != "logic.h"]


def _blank(src: str) -> str:
    return _NOISE.sub(lambda m: re.sub(r"[^\n]", " ", m.group()), src)


def _close_paren(text: str, open_idx: int) -> int:
    depth = 0
    for i in range(open_idx, len(text)):
        depth += {"(": 1, ")": -1}.get(text[i], 0)
        if depth == 0:
            return i
    return -1


def find_mutation_sites(source: str, operators: list[str] | None = None) -> list[Site]:
    code = _blank(source)
    raw: list[tuple[str, int, int, str]] = []          # operator, start, end, mutated text
    for m in _CMP.finditer(code):
        if m.group() in FLIP:
            raw.append(("flip_cmp", m.start(), m.end(), FLIP[m.group()]))
    for m in _HILO.finditer(code):
        raw.append(("swap_hilo", m.start(), m.end(), "LOW" if m.group() == "HIGH" else "HIGH"))
    for m in _CMP_LITERAL.finditer(code):
        raw.append(("off_by_one", m.start(1), m.end(1), str(int(m.group(1)) + 1)))
    for m in _ARG_LITERAL.finditer(code):
        value = int(m.group(1))
        raw.append(("const_change", m.start(1), m.end(1), str(value * 2 if value else 1)))
    for m in _CALL_STMT.finditer(code):
        if m.group(3) not in NOT_CALLS:
            raw.append(("drop_call", m.start(1), m.end(1), ";"))
    for m in _LOGIC.finditer(code):
        raw.append(("swap_logic", m.start(), m.end(), "||" if m.group() == "&&" else "&&"))
    for m in _ARITH.finditer(code):
        if not _UNARY_AFTER.search(code[:m.start(1)]):
            raw.append(("swap_arith", m.start(1), m.end(1), ARITH[m.group(1)]))
    for m in _COND.finditer(code):
        close = _close_paren(code, m.end() - 1)
        if close > m.end():
            raw.append(("negate_cond", m.end(), close, f"!({source[m.end():close]})"))
    sites = [Site(op, s, e, source[s:e], mut, source.count("\n", 0, s) + 1)
             for op, s, e, mut in raw if operators is None or op in operators]
    return sorted(sites, key=lambda x: (x.start, x.operator))


def compiles(module_dir: Path, text: str, work_dir: Path, file: str = "logic.cpp") -> bool:
    sketch = stage_test_sketch(module_dir, COMPILE_STUB, work_dir, overrides={file: text})
    return compile_sketch(sketch, work_dir / "build").ok


def make_mutants(module: str | Path, n: int | None = None, seed: int | None = None,
                 operators: list[str] | None = None, work_dir: Path | None = None,
                 check=compiles) -> list[Mutant]:
    cfg = load_config()["mutation"]
    n = cfg["mutants_per_module"] if n is None else n
    seed = cfg["seed"] if seed is None else seed
    operators = cfg["operators"] if operators is None else operators
    module_dir = resolve_module_dir(module)
    sources = module_sources(module_dir)
    work = Path(work_dir or tempfile.mkdtemp(prefix="mut_"))

    # one shuffle over all files keeps the experiment 1 mutants of single-file modules unchanged
    sites = [(f, s) for f in mutable_files(module_dir) for s in find_mutation_sites(sources[f], operators)]
    random.Random(seed).shuffle(sites)
    chosen: list[tuple[str, Site]] = []
    tried: set[int] = set()
    per_op: Counter[str] = Counter()
    for spread in (True, False):
        for i, (f, site) in enumerate(sites):
            if len(chosen) == n:
                break
            if i in tried or spread and (per_op[site.operator] >= 2 or (f, site.line) in {(cf, c.line) for cf, c in chosen}):
                continue
            tried.add(i)
            if check(module_dir, site.apply(sources[f]), work / f"candidate_{i}", f):
                chosen.append((f, site))
                per_op[site.operator] += 1
    return [Mutant(k, s, s.apply(sources[f]), _lines(sources[f], s.line, s.original),
                   _lines(s.apply(sources[f]), s.line, s.mutated), f)
            for k, (f, s) in enumerate(chosen, 1)]


def _lines(text: str, line: int, span: str) -> str:
    return " ".join(x.strip() for x in text.splitlines()[line - 1:line + span.count("\n")])
