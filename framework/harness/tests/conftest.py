from pathlib import Path

import pytest

from framework.common.config import load_config, tool_path
from framework.harness.compile import compile_sketch
from framework.harness.sketch import stage_test_sketch

FIXTURE_DIR = Path(__file__).resolve().parents[1] / "fixtures" / "blink_fixture"
FIXTURE_TESTS = FIXTURE_DIR / "tests"

requires_arduino = pytest.mark.skipif(tool_path(load_config(), "arduino_cli", "arduino-cli") is None,
                                      reason="arduino-cli not installed")


def _compile(build_root: Path, name: str):
    source = (FIXTURE_TESTS / f"{name}.ino").read_text(encoding="utf-8")
    return compile_sketch(stage_test_sketch(FIXTURE_DIR, source, build_root, name=name), build_root / name / "build")


@pytest.fixture(scope="session")
def build_root(tmp_path_factory) -> Path:
    return tmp_path_factory.mktemp("harness_build")


@pytest.fixture(scope="session")
def compiled_pass(build_root):
    return _compile(build_root, "test_main")


@pytest.fixture(scope="session")
def compiled_fail(build_root):
    return _compile(build_root, "test_failing")


@pytest.fixture(scope="session")
def compiled_hang(build_root):
    return _compile(build_root, "test_hang")


@pytest.fixture(scope="session")
def avr8js():
    from framework.harness.run_avr8js import Avr8jsBackend
    backend = Avr8jsBackend()
    ok, reason = backend.check()
    if not ok:
        pytest.skip(f"avr8js backend unavailable: {reason}")
    return backend


@pytest.fixture(scope="session")
def wokwi():
    from framework.harness.run_wokwi import WokwiBackend
    backend = WokwiBackend()
    ok, reason = backend.check()
    if not ok:
        pytest.skip(f"wokwi backend unavailable: {reason}")
    return backend
