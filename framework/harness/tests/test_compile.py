import pytest

from framework.common.config import load_config
from framework.harness.compile import arduino_cli_version, compile_sketch
from framework.harness.sketch import stage_test_sketch
from framework.harness.tests.conftest import FIXTURE_DIR, FIXTURE_TESTS, requires_arduino

pytestmark = [requires_arduino, pytest.mark.arduino]


def test_cli_version_is_pinned():
    assert arduino_cli_version() == load_config()["arduino_cli"]["version"]


def test_fixture_compiles(compiled_pass):
    assert compiled_pass.ok, compiled_pass.stderr
    assert compiled_pass.hex_path.exists() and compiled_pass.elf_path.exists()
    assert 0 < compiled_pass.flash_bytes < 32256
    assert 0 < compiled_pass.ram_bytes < 2048


def test_failing_and_hanging_tests_compile(compiled_fail, compiled_hang):
    assert compiled_fail.ok, compiled_fail.stderr
    assert compiled_hang.ok, compiled_hang.stderr


def test_broken_code_gives_stderr(tmp_path):
    source = (FIXTURE_TESTS / "test_main.ino").read_text().replace("led_on();", "led_onn();", 1)
    result = compile_sketch(stage_test_sketch(FIXTURE_DIR, source, tmp_path), tmp_path / "build")
    assert not result.ok and result.hex_path is None
    assert "led_onn" in result.stderr and "error" in result.stderr.lower()
