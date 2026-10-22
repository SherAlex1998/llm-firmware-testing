import pytest

from framework.harness.backends import run_firmware
from framework.harness.tests.conftest import requires_arduino

pytestmark = [requires_arduino, pytest.mark.arduino, pytest.mark.wokwi]


def test_fixture_passes(wokwi, compiled_pass):
    result = wokwi.run(compiled_pass.hex_path, 15)
    assert result.verdict == "pass", result.error or result.serial_log
    assert (result.tests_passed, result.tests_total) == (6, 6)


def test_failing_assert_gives_fail(wokwi, compiled_fail):
    result = wokwi.run(compiled_fail.hex_path, 15)
    assert result.verdict == "fail", result.error or result.serial_log
    assert (result.tests_passed, result.tests_total) == (1, 2)
    failed = [t for t in result.tests if not t.passed]
    assert failed[0].name == "fails_on_purpose" and "pin 13" in failed[0].msg


def test_infinite_loop_gives_timeout(wokwi, compiled_hang):
    result = wokwi.run(compiled_hang.hex_path, 5)
    assert result.verdict == "timeout", result.error or result.serial_log
    assert "@RUN_END" not in result.serial_log
    assert any(t.name == "first_passes" and t.passed for t in result.tests)


def test_default_backend_runs(wokwi, compiled_pass):
    result = run_firmware(compiled_pass.hex_path, 15, backend="wokwi")
    assert result.verdict == "pass", result.error or result.serial_log
