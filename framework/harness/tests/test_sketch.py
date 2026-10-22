from framework.harness.sketch import load_module_yaml, module_sources, stage_test_sketch
from framework.harness.tests.conftest import FIXTURE_DIR


def test_flat_layout(tmp_path):
    sketch = stage_test_sketch(FIXTURE_DIR, "// test\n", tmp_path)
    assert sorted(p.name for p in sketch.iterdir()) == ["logic.cpp", "logic.h", "test_main.ino", "test_runner.h"]
    assert (sketch / "test_main.ino").read_text() == "// test\n"
    assert "RUN_ALL_TESTS" in (sketch / "test_runner.h").read_text()


def test_source_override(tmp_path):
    sketch = stage_test_sketch(FIXTURE_DIR, "// t\n", tmp_path, overrides={"logic.cpp": "// mutant\n"})
    assert (sketch / "logic.cpp").read_text() == "// mutant\n"
    assert (sketch / "logic.h").read_text() == module_sources(FIXTURE_DIR)["logic.h"]


def test_restage_replaces_folder(tmp_path):
    sketch = stage_test_sketch(FIXTURE_DIR, "// a\n", tmp_path)
    (sketch / "stale.txt").write_text("x")
    assert stage_test_sketch(FIXTURE_DIR, "// b\n", tmp_path) == sketch
    assert not (sketch / "stale.txt").exists()


def test_fixture_module_yaml():
    meta = load_module_yaml(FIXTURE_DIR)
    assert meta["id"] == "blink_fixture" and len(meta["entry_functions"]) >= 2
