"""Top-k documentation chunks for a benchmark module -> the <reference> block of the RAG prompt.

    python -m framework.rag.retrieve --module servo_sweep

The query names the module, its pins, the Arduino API it calls and the functions to test. Query embeddings are
cached in kb/.query_cache, so a rerun retrieves the same chunks without calling the API.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

from framework.common.config import load_config, resolve_path
from framework.harness.sketch import load_module_yaml, module_sources, resolve_module_dir
from framework.rag.build_index import COLLECTION, Embedder, openai_embedder

SERVO_API = ["Servo", "attach", "write", "read", "detach", "writeMicroseconds"]
MAX_PER_DOC = 2               # at most this many chunks of one kb/ file in the top-k


@dataclass
class RagResult:
    query: str
    context: str                                          # text for the <reference> block
    chunks: list[dict] = field(default_factory=list)      # {id, title, source_path, kind, similarity, text}


def arduino_api(kb_dir: Path) -> set[str]:
    """Function and constant names that have a page in kb/reference/."""
    return {p.stem for p in (kb_dir / "reference").glob("*.md")}


def build_query(module: str | Path, kb_dir: Path) -> str:
    module_dir = resolve_module_dir(module)
    meta = load_module_yaml(module_dir)
    src = "\n".join(module_sources(module_dir).values())
    calls = set(re.findall(r"\b([A-Za-z_]\w*)\s*\(", src))
    api = sorted(calls & arduino_api(kb_dir))
    libs = sorted(set(re.findall(r"#include\s*<(\w+)\.h>", src)) - {"Arduino"})
    if "Servo" in libs:
        api += [a for a in SERVO_API if a in calls or a == "Servo"]
    pins = "; ".join(f"pin {p['pin']} {p['mode']} {p.get('role', '')}".strip() for p in meta["pins"])
    if meta.get("experiment") == 2:
        return exp2_query(meta, src, calls, kb_dir, pins)
    return (f"Unit tests for an Arduino Uno module: {meta['name']}. Pins: {pins}. "
            f"Arduino API used: {', '.join(api) or 'none'}. Libraries: {', '.join(libs) or 'none'}. "
            f"Functions to test: {', '.join(meta['entry_functions'])}.")


def library_api(meta: dict, calls: set[str], kb_dir: Path) -> list[str]:
    """Methods of the module's libraries (from kb/exp2/libraries/<lib>_api.md) that its code calls."""
    names: set[str] = set()
    for lib in meta.get("libraries") or []:
        doc = kb_dir / "exp2" / "libraries" / f"{re.sub(r'[^a-z0-9]+', '_', lib['name'].lower()).strip('_')}_api.md"
        if doc.exists():
            names |= set(re.findall(r"\b([A-Za-z_]\w{2,})\s*\(", doc.read_text(encoding="utf-8")))
    return sorted(calls & names)


def exp2_query(meta: dict, src: str, calls: set[str], kb_dir: Path, pins: str) -> str:
    """Experiment 2 query: also the core pages of kb/exp2/reference, library methods and the Wokwi devices."""
    api = sorted(calls & (arduino_api(kb_dir) | {p.stem for p in (kb_dir / "exp2" / "reference").glob("*.md")}))
    libs = [lib["name"] for lib in meta.get("libraries") or []]
    devices = [d["part"] for d in meta.get("devices") or []]
    return (f"Unit tests for an Arduino Uno module: {meta['name']}. Pins: {pins}. "
            f"Arduino API used: {', '.join(api) or 'none'}. Libraries: {', '.join(libs) or 'none'}. "
            f"Library API used: {', '.join(library_api(meta, calls, kb_dir)) or 'none'}. "
            f"Simulated devices: {', '.join(devices) or 'none'}. "
            f"Functions to test: {', '.join(meta['entry_functions'])}.")


def cached_embedding(query: str, model: str, cache_dir: Path, embed: Embedder | None) -> list[float]:
    path = Path(cache_dir) / f"{hashlib.sha256(f'{model}\0{query}'.encode('utf-8')).hexdigest()}.json"
    if path.exists():
        return json.loads(path.read_text(encoding="utf-8"))["embedding"]
    vector = (embed or openai_embedder(model))([query])[0]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps({"model": model, "query": query, "embedding": vector}), encoding="utf-8")
    return vector


def format_context(chunks: list[dict], max_tokens: int) -> str:
    """Whole chunks in rank order while they fit into ~max_tokens (4 characters per token)."""
    parts, used = [], 0
    for c in chunks:
        part = f"[{c['source_path']}]\n{c['text']}"
        if used + len(part) > max_tokens * 4:
            break
        parts.append(part)
        used += len(part) + 2
    return "\n\n".join(parts)


def retrieve_context(module: str | Path, top_k: int | None = None, embed: Embedder | None = None,
                     chroma_path: Path | None = None, cache_dir: Path | None = None) -> RagResult:
    import chromadb
    cfg = load_config()["rag"]
    chroma_path = Path(chroma_path or resolve_path(cfg["chroma_path"]))
    kb_dir = resolve_path(cfg["chroma_path"]).parent
    query = build_query(module, kb_dir)
    vector = cached_embedding(query, cfg["embedding_model"], cache_dir or kb_dir / ".query_cache", embed)
    col = chromadb.PersistentClient(path=str(chroma_path)).get_collection(COLLECTION)
    top_k = top_k or cfg["top_k"]
    r = col.query(query_embeddings=[vector], n_results=min(col.count(), top_k * 6),
                  include=["documents", "metadatas", "distances"])
    chunks, per_doc = [], {}
    for i, t, m, d in zip(r["ids"][0], r["documents"][0], r["metadatas"][0], r["distances"][0]):
        if per_doc.get(m["source_path"], 0) >= MAX_PER_DOC:      # diversity: one long document can't fill the top
            continue
        per_doc[m["source_path"]] = per_doc.get(m["source_path"], 0) + 1
        chunks.append({"id": i, "title": m["title"], "source_path": m["source_path"], "kind": m["kind"],
                       "similarity": round(1 - d, 4), "text": t})
        if len(chunks) == top_k:
            break
    return RagResult(query, format_context(chunks, cfg["max_context_tokens"]), chunks)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--module", required=True)
    ap.add_argument("--top-k", type=int)
    ap.add_argument("--show-context", action="store_true")
    args = ap.parse_args()
    res = retrieve_context(args.module, args.top_k)
    print("query:", res.query)
    for c in res.chunks:
        print(f"  {c['similarity']:.3f}  {c['kind']:8s} {c['title']}  [{c['source_path']}]")
    print(f"context: {len(res.context)} chars (~{len(res.context) // 4} tokens)")
    if args.show_context:
        print(res.context)
    return 0


if __name__ == "__main__":
    sys.exit(main())
