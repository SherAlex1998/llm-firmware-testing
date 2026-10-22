import hashlib
import re
from pathlib import Path

import pytest

from framework.rag.build_index import MAX_CHARS, build_index, chunk_markdown, load_chunks
from framework.rag.fetch_docs import adoc_to_md
from framework.rag.retrieve import build_query, format_context, retrieve_context

chromadb = pytest.importorskip("chromadb")
KB = Path(__file__).resolve().parents[3] / "kb"


def fake_embed(texts):
    """Deterministic bag-of-words vectors: texts sharing words are close."""
    vectors = []
    for text in texts:
        v = [0.0] * 256
        for word in re.findall(r"[a-z]+", text.lower()):
            v[int(hashlib.md5(word.encode()).hexdigest(), 16) % 256] += 1.0
        vectors.append(v)
    return vectors


def make_kb(root: Path) -> Path:
    kb = root / "kb"
    docs = {"reference/digitalWrite.md": "# digitalWrite()\n\n## Description\nWrite a HIGH or LOW value to a digital "
                                         "pin, for example to switch an LED on or off.\n",
            "libraries/servo_api.md": "# Servo library\n\n## attach()\nAttach the Servo variable to a pin.\n\n"
                                      "## write()\nWrites a value to the servo, controlling the shaft angle.\n\n"
                                      "## read()\nRead the current angle of the servo.\n",
            "guides/testing_timing.md": "# Testing timing\n\n## millis\nUse millis() and synthetic times to test "
                                        "debounce logic without waiting.\n",
            "learn/digital-pins.md": "# Digital Pins\n\nPins configured as OUTPUT can provide current to LEDs.\n"}
    for rel, text in docs.items():
        (kb / rel).parent.mkdir(parents=True, exist_ok=True)
        (kb / rel).write_text(text, encoding="utf-8")
    return kb


def test_adoc_to_md():
    src = ("---\ntitle: x()\n---\n\n//\n:ext-relative: .html\n\n= x()\n\n[float]\n=== Description\nDoes x with "
           "link:../y[y()].\n\n[source,arduino]\n----\nx(1);\n----\n\n[float]\n=== See also\n* link:../z[z()]\n")
    md = adoc_to_md(src)
    assert md.startswith("# x()") and "## Description" in md and "Does x with y()." in md
    assert "```cpp\nx(1);\n```" in md and "See also" not in md and "title:" not in md and "[float]" not in md


def test_chunk_merges_small_sections_and_titles_them():
    text = "<!-- source: s -->\n\n# pinMode()\n\n## Description\nConfigures a pin.\n\n## Syntax\n`pinMode(pin, mode)`\n"
    chunks = chunk_markdown(text, "pinMode")
    assert len(chunks) == 1 and chunks[0][0] == "pinMode()" and "Syntax" in chunks[0][1] and "source" not in chunks[0][1]


def test_chunk_splits_long_code_into_fenced_pieces():
    code = "\n".join(f"  analogWrite(9, {i});  // step {i}" for i in range(200))
    chunks = chunk_markdown(f"# Big\n\n## Example\nIntro text for the example.\n\n```cpp\n{code}\n```\n", "big")
    assert len(chunks) > 3 and all(len(body) <= MAX_CHARS + 60 for _, body in chunks)
    assert all(body.count("```") % 2 == 0 for _, body in chunks) and chunks[1][0] == "Big / Example"


def test_chunk_drops_heading_only_sections():
    paragraphs = "\n\n".join("word " * 60 for _ in range(8))     # a section too long to merge with the heading
    chunks = chunk_markdown(f"# T\n\n## Empty heading\n\n## Big\n{paragraphs}\n", "t")
    assert chunks and all("word" in body for _, body in chunks)
    assert not any(body.strip() == "## Empty heading" for _, body in chunks)


def test_load_chunks_kinds(tmp_path):
    chunks = load_chunks(make_kb(tmp_path))
    assert {c.kind for c in chunks} == {"core_ref", "library", "guide", "learn"}
    assert all(c.source_path.startswith("kb/") and c.text.startswith(c.title) for c in chunks)


def test_build_query_describes_module():
    q = build_query("servo_sweep", KB)
    assert "Servo" in q and "attach" in q and "pin 9" in q and "sweep_clamp_angle" in q
    q = build_query("led_chaser", KB)
    assert "digitalWrite" in q and "pinMode" in q and "Libraries: none" in q


def test_retrieve_ranks_relevant_docs_and_caches_query(tmp_path):
    kb = make_kb(tmp_path)
    chroma, cache = tmp_path / "chroma", tmp_path / "cache"
    build_index(kb, chroma, embed=fake_embed)
    res = retrieve_context("servo_sweep", top_k=2, embed=fake_embed, chroma_path=chroma, cache_dir=cache)
    assert res.chunks[0]["kind"] == "library" and res.chunks[0]["source_path"] == "kb/libraries/servo_api.md"
    assert res.chunks[0]["similarity"] >= res.chunks[1]["similarity"] and "[kb/libraries/servo_api.md]" in res.context

    def no_api(texts):
        raise AssertionError("query embedding must come from the cache")
    again = retrieve_context("servo_sweep", top_k=2, embed=no_api, chroma_path=chroma, cache_dir=cache)
    assert [c["id"] for c in again.chunks] == [c["id"] for c in res.chunks]


def test_at_most_two_chunks_per_document(tmp_path):
    kb = make_kb(tmp_path)
    sections = [f"## Servo part {i}\n" + "Servo attach write read angle. " * 20 for i in range(6)]
    long_doc = "# Servo guide\n\n" + "\n\n".join(sections)
    (kb / "guides" / "servo_long.md").write_text(long_doc, encoding="utf-8")
    build_index(kb, tmp_path / "chroma", embed=fake_embed)
    res = retrieve_context("servo_sweep", top_k=4, embed=fake_embed, chroma_path=tmp_path / "chroma",
                           cache_dir=tmp_path / "cache")
    sources = [c["source_path"] for c in res.chunks]
    assert len(sources) == 4 and sources.count("kb/guides/servo_long.md") == 2


def test_rebuild_replaces_collection(tmp_path):
    kb = make_kb(tmp_path)
    build_index(kb, tmp_path / "chroma", embed=fake_embed)
    n = len(build_index(kb, tmp_path / "chroma", embed=fake_embed))
    count = chromadb.PersistentClient(path=str(tmp_path / "chroma")).get_collection("arduino_kb").count()
    assert count == n


def test_format_context_keeps_whole_chunks_within_budget():
    chunks = [{"source_path": f"kb/x{i}.md", "text": "a" * 390} for i in range(5)]
    ctx = format_context(chunks, max_tokens=250)             # ~1000 characters -> two chunks
    assert ctx.count("[kb/x") == 2 and ctx.endswith("a")
