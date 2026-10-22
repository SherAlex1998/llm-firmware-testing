import pytest

from framework.harness.backends import get_backend, run_firmware
from framework.harness.run_avr8js import Avr8jsBackend
from framework.harness.tests.conftest import FIXTURE_DIR, _compile, requires_arduino

pytestmark = [requires_arduino, pytest.mark.arduino, pytest.mark.avr8js]

ADC_TEST = ('#include "logic.h"\n#include "test_runner.h"\n'
            "TEST(adc_reads) { int v = analogRead(A0); ASSERT_TRUE(v >= 0 && v <= 1023); }\n"
            "void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }\nvoid loop() {}\n")


def protocol_lines(log):
    return [line.strip() for line in log.splitlines() if line.startswith("@")]


def test_fixture_passes(avr8js, compiled_pass):
    result = avr8js.run(compiled_pass.hex_path, 15)
    assert result.verdict == "pass", result.error or result.serial_log
    assert (result.tests_passed, result.tests_total) == (6, 6) and result.backend == "avr8js"


def test_failing_assert_gives_fail(avr8js, compiled_fail):
    result = avr8js.run(compiled_fail.hex_path, 15)
    assert result.verdict == "fail" and (result.tests_passed, result.tests_total) == (1, 2)
    assert "pin 13" in [t for t in result.tests if not t.passed][0].msg


def test_infinite_loop_gives_timeout(avr8js, compiled_hang):
    result = avr8js.run(compiled_hang.hex_path, 3)
    assert result.verdict == "timeout" and "@RUN_END" not in result.serial_log
    assert any(t.name == "first_passes" and t.passed for t in result.tests)


def test_analog_read_does_not_hang(avr8js, tmp_path):
    from framework.harness.compile import compile_sketch
    from framework.harness.sketch import stage_test_sketch
    comp = compile_sketch(stage_test_sketch(FIXTURE_DIR, ADC_TEST, tmp_path), tmp_path / "build")
    assert comp.ok, comp.stderr
    assert avr8js.run(comp.hex_path, 5).verdict == "pass"


PULLUP_TEST = ('#include "logic.h"\n#include "test_runner.h"\n'
               "TEST(pullup_reads_high) { pinMode(2, INPUT_PULLUP); ASSERT_PIN(2, HIGH); }\n"
               "TEST(plain_input_reads_low) { pinMode(3, INPUT); ASSERT_PIN(3, LOW); }\n"
               "TEST(write_high_on_input_enables_pullup) { pinMode(4, INPUT); digitalWrite(4, HIGH); ASSERT_PIN(4, HIGH); }\n"
               "TEST(floating_input_keeps_level) { pinMode(5, INPUT_PULLUP); pinMode(5, INPUT); ASSERT_PIN(5, HIGH); }\n"
               "void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }\nvoid loop() {}\n")


def test_internal_pullup_is_modelled(avr8js, tmp_path):
    """avr8js alone reads INPUT_PULLUP as LOW; run.js emulates the pull-up and a floating input keeps its level,
    like the real board and Wokwi."""
    from framework.harness.compile import compile_sketch
    from framework.harness.sketch import stage_test_sketch
    comp = compile_sketch(stage_test_sketch(FIXTURE_DIR, PULLUP_TEST, tmp_path), tmp_path / "build")
    result = avr8js.run(comp.hex_path, 5)
    assert (result.verdict, result.tests_passed) == ("pass", 4), result.serial_log


def test_selected_by_name(avr8js, compiled_pass):
    assert get_backend("avr8js").name == "avr8js"
    assert run_firmware(compiled_pass.hex_path, 15, backend="avr8js").verdict == "pass"


def test_missing_node_and_firmware_are_errors(tmp_path, compiled_pass):
    r = Avr8jsBackend(node=str(tmp_path / "no_node.exe")).run(compiled_pass.hex_path, 1)
    assert r.verdict == "error" and "node not found" in r.error
    r = Avr8jsBackend().run(tmp_path / "missing.hex", 1)
    assert r.verdict == "error"


@pytest.mark.wokwi
@pytest.mark.parametrize("name", ["test_main", "test_failing", "test_hang"])
def test_same_output_as_wokwi(avr8js, wokwi, build_root, name):
    comp = _compile(build_root, name)
    a, w = avr8js.run(comp.hex_path, 5), wokwi.run(comp.hex_path, 5)
    assert a.verdict == w.verdict and protocol_lines(a.serial_log) == protocol_lines(w.serial_log), \
        (a.verdict, w.verdict, w.error, protocol_lines(a.serial_log), protocol_lines(w.serial_log))
