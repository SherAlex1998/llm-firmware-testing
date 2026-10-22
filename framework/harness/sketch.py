from __future__ import annotations

import shutil
from pathlib import Path

import yaml

TEST_RUNNER_H = Path(__file__).resolve().parent / "testlib" / "test_runner.h"
TEST_RUNNER_V2_H = TEST_RUNNER_H.with_name("test_runner_v2.h")     # experiment 2: up to 40 tests
REPO_ROOT = Path(__file__).resolve().parents[2]


def resolve_module_dir(module: str | Path) -> Path:
    """A module folder, a benchmark id (benchmark/<id>) or a fixture id (framework/harness/fixtures/<id>)."""
    for c in (Path(module), REPO_ROOT / "benchmark" / str(module), REPO_ROOT / "framework/harness/fixtures" / str(module)):
        if (c / "module.yaml").exists():
            return c.resolve()
    raise FileNotFoundError(f"module {str(module)!r}: no module.yaml found")


def load_module_yaml(module_dir: Path) -> dict:
    return yaml.safe_load((Path(module_dir) / "module.yaml").read_text(encoding="utf-8"))


def module_sources(module_dir: Path) -> dict[str, str]:
    """Files of src/ in prompt order: module.yaml `sources` (experiment 2), else logic.h + logic.cpp."""
    src = Path(module_dir) / "src"
    names = load_module_yaml(module_dir).get("sources") or ["logic.h", "logic.cpp"]
    return {name: (src / name).read_text(encoding="utf-8") for name in names}


def is_exp2(module_dir: Path) -> bool:
    return load_module_yaml(module_dir).get("experiment") == 2


def test_runner_for(module_dir: Path) -> Path:
    """The test framework staged as test_runner.h (and shown in the system prompt) for this module."""
    return TEST_RUNNER_V2_H if is_exp2(module_dir) else TEST_RUNNER_H


def module_diagram(module_dir: Path) -> str | None:
    """benchmark/<id>/diagram.json (Wokwi circuit with the module's devices), if the module has one."""
    path = Path(module_dir) / "diagram.json"
    return path.read_text(encoding="utf-8") if path.exists() else None


def stage_test_sketch(module_dir: Path, test_source: str, out_dir: Path, name: str = "test_main",
                      overrides: dict[str, str] | None = None) -> Path:
    """overrides: source file name -> replacement text (a mutant)."""
    sketch = Path(out_dir) / name
    shutil.rmtree(sketch, ignore_errors=True)
    sketch.mkdir(parents=True)
    files = module_sources(module_dir)
    overrides = overrides or {}
    unknown = set(overrides) - set(files)
    if unknown:
        raise KeyError(f"override of files the module does not have: {sorted(unknown)}")
    files.update(overrides)
    files[f"{name}.ino"] = test_source
    files["test_runner.h"] = test_runner_for(module_dir).read_text(encoding="utf-8")
    for fname, text in files.items():
        (sketch / fname).write_text(text, encoding="utf-8")
    return sketch
