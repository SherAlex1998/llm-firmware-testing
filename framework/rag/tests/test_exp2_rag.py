"""Experiment 2 RAG: experiment 1 queries unchanged, experiment 2 queries name libraries/devices, separate KB sets."""
import json
from pathlib import Path

import pytest

from framework.common.config import REPO_ROOT
from framework.rag.build_index import kb_set, kb_version, load_chunks
from framework.rag.retrieve import build_query

KB = REPO_ROOT / "kb"
V1_MODULES = sorted(p.parent.name for p in (REPO_ROOT / "benchmark").glob("*/module.yaml")
                    if "experiment: 2" not in p.read_text(encoding="utf-8"))


def cached_queries() -> set[str]:
    return {json.loads(p.read_text(encoding="utf-8"))["query"] for p in (KB / ".query_cache").glob("*.json")}


@pytest.mark.skipif(not (KB / ".query_cache").exists(), reason="no query cache")
@pytest.mark.parametrize("module", V1_MODULES)
def test_experiment1_query_unchanged(module):
    assert build_query(module, KB) in cached_queries()


def test_experiment2_query_names_libraries_and_devices():
    q = build_query("ultrasonic_parking", KB)
    assert "NewPing" in q and "wokwi-hc-sr04" in q and "Library API used:" in q
    assert "ping_median" in q or "convert_cm" in q


def test_experiment1_kb_set_excludes_exp2_folder():
    kb_dir, _, extra = kb_set("exp1")
    assert not any("/exp2/" in c.source_path for c in load_chunks(kb_dir, extra))
    kb_dir, _, extra2 = kb_set("exp2")
    assert any("/exp2/" in c.source_path for c in load_chunks(kb_dir, extra2))
    assert kb_version(kb_dir, extra) != kb_version(kb_dir, extra2)


def test_preview_library_of():
    from framework.rag.exp2_preview import library_of
    assert library_of("kb/exp2/libraries/liquidcrystal_i2c_api.md") == "liquidcrystal_i2c"
    assert library_of("kb/exp2/libraries/md_max72xx_example_md_max72xx_test.md") == "md_max72xx"
    assert library_of("kb/exp2/libraries/adafruit_neopixel_readme.md") == "adafruit_neopixel"
    assert library_of("kb/libraries/servo_api.md") is None
    assert library_of("kb/exp2/devices/wokwi-hc-sr04.md") is None
