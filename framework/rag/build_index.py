"""Index kb/ (documentation only) into ChromaDB with OpenAI embeddings.

    python -m framework.rag.build_index                  # experiment 1: kb/ -> kb/.chroma (full rebuild)
    python -m framework.rag.build_index --kb-set exp2    # experiment 2: kb/ + kb/exp2/ -> kb/.chroma_exp2

Chunks: markdown split at headings; a chunk shorter than MERGE_BELOW absorbs the next section, long sections are
split at blank lines (long code blocks at line boundaries). Every chunk starts with "<document title> / <section>".
"""
from __future__ import annotations

import argparse
import hashlib
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Callable

from framework.common.config import load_config, load_dotenv, resolve_path

COLLECTION = "arduino_kb"
KINDS = {"reference": "core_ref", "learn": "learn", "examples": "example", "libraries": "library",
         "guides": "guide", "devices": "device"}
MAX_CHARS = 1500
MERGE_BELOW = 400             # a chunk shorter than this absorbs the next section (reference pages: tiny sections)

Embedder = Callable[[list[str]], list[list[float]]]


@dataclass
class Chunk:
    id: str
    text: str
    source_path: str          # relative to the repo root
    kind: str                 # core_ref | learn | example | library | guide
    title: str


def _blocks(text: str) -> list[str]:
    """Paragraphs separated by blank lines; a fenced code block is never split."""
    blocks, current, fenced = [], [], False
    for line in text.splitlines():
        if line.strip().startswith("```"):
            fenced = not fenced
        if not line.strip() and not fenced:
            if current:
                blocks.append("\n".join(current))
                current = []
            continue
        current.append(line)
    if current:
        blocks.append("\n".join(current))
    return [piece for block in blocks for piece in _split_code(block)]


def _split_code(block: str) -> list[str]:
    """A code block longer than MAX_CHARS -> several fenced blocks cut at line boundaries."""
    lines = block.splitlines()
    if len(block) <= MAX_CHARS or not lines[0].strip().startswith("```"):
        return [block]
    fence, body = lines[0].strip(), [ln for ln in lines[1:] if not ln.strip().startswith("```")]
    pieces, current = [], []
    for line in body:
        if current and sum(len(x) + 1 for x in current) + len(line) > MAX_CHARS - 20:
            pieces.append("\n".join([fence, *current, "```"]))
            current = []
        current.append(line)
    return pieces + ["\n".join([fence, *current, "```"])] if current else pieces


def chunk_markdown(text: str, fallback_title: str) -> list[tuple[str, str]]:
    """[(title, body)] for one document."""
    text = re.sub(r"\A<!--.*?-->\s*", "", text, flags=re.S)
    doc = re.search(r"^# (.+)$", text, flags=re.M)
    doc_title = doc.group(1).strip() if doc else fallback_title
    sections, heading, lines, fenced = [], "", [], False
    for line in text.splitlines():
        if line.strip().startswith("```"):
            fenced = not fenced
        match = None if fenced else re.match(r"#{1,4} (.+)", line)
        if match and lines:
            sections.append((heading, "\n".join(lines).strip()))
            lines = []
        if match:
            heading = match.group(1).strip()
        lines.append(line)
    sections.append((heading, "\n".join(lines).strip()))

    pieces = []                                   # long sections -> pieces of at most MAX_CHARS
    for heading, body in sections:
        if len(body) <= MAX_CHARS:
            pieces.append((heading, body))
            continue
        part = ""
        for block in _blocks(body):
            if part and len(part) + len(block) + 2 > MAX_CHARS:
                pieces.append((heading, part))
                part = ""
            part = f"{part}\n\n{block}" if part else block
        pieces.append((heading, part))

    chunks, heading, body = [], "", ""            # merge tiny neighbours; sections of real size stay separate
    for h, piece in pieces:
        if body and (len(body) >= MERGE_BELOW or len(body) + len(piece) + 2 > MAX_CHARS):
            chunks.append((heading, body))
            body = ""
        heading, body = (heading, f"{body}\n\n{piece}") if body else (h, piece)
    if body.strip():
        chunks.append((heading, body))
    has_text = lambda b: len(re.sub(r"^#{1,4} .*$", "", b, flags=re.M).strip()) >= 40   # not just a heading
    return [(f"{doc_title} / {h}" if h and h != doc_title else doc_title, b) for h, b in chunks if has_text(b)]


def kb_files(kb_dir: Path, extra_dirs: tuple[Path, ...] = ()) -> list[tuple[Path, str]]:
    """(path, kind) of every indexed document: kb/<folder>/*.md, then the same folders under each extra dir."""
    return [(path, kind) for base in (Path(kb_dir), *extra_dirs) for folder, kind in KINDS.items()
            for path in sorted((base / folder).glob("*.md"))]


def load_chunks(kb_dir: Path, extra_dirs: tuple[Path, ...] = ()) -> list[Chunk]:
    kb_dir = Path(kb_dir)
    chunks = []
    for path, kind in kb_files(kb_dir, extra_dirs):
        rel = path.relative_to(kb_dir.parent).as_posix()
        for i, (title, body) in enumerate(chunk_markdown(path.read_text(encoding="utf-8"), path.stem)):
            chunks.append(Chunk(f"{rel}#{i}", f"{title}\n\n{body}", rel, kind, title))
    return chunks


def kb_version(kb_dir: Path, extra_dirs: tuple[Path, ...] = ()) -> str:
    """sha256 over the sorted (relative path, sha256 of content) of the document set; 12 hex chars."""
    kb_dir = Path(kb_dir)
    items = sorted((p.relative_to(kb_dir.parent).as_posix(), hashlib.sha256(p.read_bytes()).hexdigest())
                   for p, _ in kb_files(kb_dir, extra_dirs))
    return hashlib.sha256("\n".join(f"{a} {b}" for a, b in items).encode()).hexdigest()[:12]


def kb_set(name: str, cfg: dict | None = None) -> tuple[Path, Path, tuple[Path, ...]]:
    """exp1: kb/ -> rag.chroma_path; exp2: kb/ + kb/exp2/ -> experiment2.kb_chroma. Returns (kb_dir, chroma, extra)."""
    cfg = cfg or load_config()
    kb_dir = resolve_path(cfg["rag"]["chroma_path"]).parent
    if name == "exp1":
        return kb_dir, resolve_path(cfg["rag"]["chroma_path"]), ()
    if name == "exp2":
        return kb_dir, resolve_path(cfg["experiment2"]["kb_chroma"]), (kb_dir / "exp2",)
    raise ValueError(f"unknown kb set {name!r}")


def openai_embedder(model: str) -> Embedder:
    import openai
    load_dotenv()
    client = openai.OpenAI(max_retries=3)

    def embed(texts: list[str]) -> list[list[float]]:
        vectors = []
        for i in range(0, len(texts), 100):
            resp = client.embeddings.create(model=model, input=texts[i:i + 100])
            vectors += [d.embedding for d in resp.data]
        return vectors
    return embed


def build_index(kb_dir: Path | None = None, chroma_path: Path | None = None, embed: Embedder | None = None,
                extra_dirs: tuple[Path, ...] = ()) -> list[Chunk]:
    import chromadb
    cfg = load_config()["rag"]
    kb_dir = Path(kb_dir or resolve_path(cfg["chroma_path"]).parent)
    chroma_path = Path(chroma_path or resolve_path(cfg["chroma_path"]))
    embed = embed or openai_embedder(cfg["embedding_model"])
    chunks = load_chunks(kb_dir, extra_dirs)
    vectors = embed([c.text for c in chunks])
    client = chromadb.PersistentClient(path=str(chroma_path))
    if COLLECTION in [c.name for c in client.list_collections()]:
        client.delete_collection(COLLECTION)
    col = client.create_collection(COLLECTION, configuration={"hnsw": {"space": "cosine"}}, embedding_function=None,
                                   metadata={"kb_version": kb_version(kb_dir, extra_dirs)})
    col.add(ids=[c.id for c in chunks], embeddings=vectors, documents=[c.text for c in chunks],
            metadatas=[{"source_path": c.source_path, "kind": c.kind, "title": c.title} for c in chunks])
    return chunks


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--kb-set", choices=["exp1", "exp2"], default="exp1")
    args = ap.parse_args()
    kb_dir, chroma, extra = kb_set(args.kb_set)
    chunks = build_index(kb_dir, chroma, extra_dirs=extra)
    by_kind: dict[str, int] = {}
    for c in chunks:
        by_kind[c.kind] = by_kind.get(c.kind, 0) + 1
    chars = sum(len(c.text) for c in chunks)
    print(f"indexed {len(chunks)} chunks {by_kind}; {chars} chars (~{chars // 4} tokens) -> {chroma} ({COLLECTION}, kb_version {kb_version(kb_dir, extra)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
