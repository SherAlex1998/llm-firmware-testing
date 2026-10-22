"""Experiment 2 harness additions: module sources/overrides, per-module backend, Wokwi diagram, retries, quota log."""
import csv
import subprocess
from pathlib import Path

import pytest

from framework.common.config import load_config
from framework.harness import run_wokwi
from framework.harness.backends import backend_name, get_backend
from framework.harness.compile import compile_sketch
from framework.harness.run_wokwi import DIAGRAM, TIMEOUT_EXIT_CODE, WokwiBackend, wokwi_usage_summary
from framework.harness.sketch import module_diagram, module_sources, stage_test_sketch
from framework.harness.tests.conftest import requires_arduino

COUNTER = Path(__file__).resolve().parents[1] / "fixtures" / "counter_fixture"
PASS_LOG = "@RUN_BEGIN 1\n@TEST t PASS\n@RUN_END 1/1\n"


def test_sources_field_orders_files():
    assert list(module_sources(COUNTER)) == ["logic.h", "logic.cpp", "display.cpp"]


def test_stage_multifile_with_override(tmp_path):
    sketch = stage_test_sketch(COUNTER, "// t\n", tmp_path, overrides={"display.cpp": "// mutant\n"})
    assert sorted(p.name for p in sketch.iterdir()) == [
        "display.cpp", "logic.cpp", "logic.h", "test_main.ino", "test_runner.h"]
    assert (sketch / "display.cpp").read_text() == "// mutant\n"
    assert (sketch / "logic.cpp").read_text() == module_sources(COUNTER)["logic.cpp"]


def test_override_of_unknown_file_fails(tmp_path):
    with pytest.raises(KeyError):
        stage_test_sketch(COUNTER, "// t\n", tmp_path, overrides={"nope.cpp": ""})


def test_backend_priority():
    cfg = {"backends": {"default": "avr8js", "force": None}}
    assert backend_name(cfg) == "avr8js"
    assert backend_name(cfg, module_meta={"backend": "wokwi"}) == "wokwi"
    assert backend_name(cfg, "board", {"backend": "wokwi"}) == "board"
    assert backend_name({"backends": {"default": "avr8js", "force": "wokwi"}}, "board", {"backend": "avr8js"}) == "wokwi"


def test_module_backend_and_diagram(tmp_path):
    module = tmp_path / "dev_module"
    (module / "src").mkdir(parents=True)
    (module / "module.yaml").write_text("id: dev_module\nbackend: wokwi\n")
    (module / "diagram.json").write_text('{"parts": ["sonar"]}')
    backend = get_backend(module=module)
    assert backend.name == "wokwi" and backend.diagram == module_diagram(module) == '{"parts": ["sonar"]}'
    assert get_backend(module=COUNTER).name == "avr8js"
    assert get_backend("wokwi").diagram == DIAGRAM


@pytest.fixture()
def firmware(tmp_path):
    hex_path = tmp_path / "fw.ino.hex"
    hex_path.write_text(":00000001FF\n")
    hex_path.with_suffix(".elf").write_bytes(b"\x7fELF")
    return hex_path


def fake_cli(monkeypatch, outcomes):
    """Replace wokwi-cli: each call pops (exit_code, serial_log) and writes the log where the CLI would."""
    calls = []

    def run(cmd, **kw):
        code, serial = outcomes.pop(0)
        calls.append(Path(cmd[-1]))
        assert (Path(cmd[-1]) / "diagram.json").exists()
        Path(cmd[cmd.index("--serial-log-file") + 1]).write_text(serial)
        return subprocess.CompletedProcess(cmd, code, "", "cloud says no" if code not in (0, TIMEOUT_EXIT_CODE) else "")
    monkeypatch.setattr(run_wokwi.subprocess, "run", run)
    monkeypatch.setattr(run_wokwi.time, "sleep", lambda s: None)
    return calls


def make_backend(tmp_path, retries=2):
    cfg = {**load_config(), "wokwi": {"retries": retries, "retry_pause_s": 0}}
    b = WokwiBackend(cfg, cli=Path(__file__), token="dummy", diagram='{"x": 1}', usage_log=tmp_path / "usage.csv")
    b.context = {"run_id": "r1", "kind": "test", "module_id": "m"}
    return b


def test_cloud_failure_is_retried(monkeypatch, tmp_path, firmware):
    fake_cli(monkeypatch, [(1, ""), (0, PASS_LOG)])
    result = make_backend(tmp_path).run(firmware, 5)
    assert result.verdict == "pass" and result.retries == 1
    rows = list(csv.DictReader(open(tmp_path / "usage.csv", encoding="utf-8")))
    assert len(rows) == 1 and rows[0]["run_id"] == "r1" and rows[0]["retries"] == "1"


def test_retries_exhausted_gives_error(monkeypatch, tmp_path, firmware):
    fake_cli(monkeypatch, [(1, ""), (1, ""), (1, "")])
    result = make_backend(tmp_path).run(firmware, 5)
    assert result.verdict == "error" and result.retries == 2 and "cloud says no" in result.error
    assert wokwi_usage_summary(tmp_path / "usage.csv")["errors"] == 1


def test_firmware_timeout_is_not_retried(monkeypatch, tmp_path, firmware):
    calls = fake_cli(monkeypatch, [(TIMEOUT_EXIT_CODE, "@RUN_BEGIN 2\n@TEST a PASS\n")])
    result = make_backend(tmp_path).run(firmware, 5)
    assert result.verdict == "timeout" and result.retries == 0 and len(calls) == 1


def test_diagram_is_written(monkeypatch, tmp_path, firmware):
    seen = []

    def run(cmd, **kw):
        seen.append((Path(cmd[-1]) / "diagram.json").read_text())
        Path(cmd[cmd.index("--serial-log-file") + 1]).write_text(PASS_LOG)
        return subprocess.CompletedProcess(cmd, 0, "", "")
    monkeypatch.setattr(run_wokwi.subprocess, "run", run)
    make_backend(tmp_path).run(firmware, 5)
    assert seen == ['{"x": 1}']


SONAR = COUNTER.parent / "sonar_fixture"


@requires_arduino
@pytest.mark.arduino
@pytest.mark.wokwi
def test_module_diagram_reaches_wokwi(tmp_path, avr8js, wokwi):
    """HC-SR04 at 100 cm exists only in the module's diagram.json: pass on wokwi, fail (0 cm) on avr8js."""
    source = (SONAR / "tests" / "test_main.ino").read_text(encoding="utf-8")
    comp = compile_sketch(stage_test_sketch(SONAR, source, tmp_path), tmp_path / "build")
    assert comp.ok, comp.stderr
    backend = get_backend(module=SONAR)
    assert backend.name == "wokwi"
    result = backend.run(comp.hex_path, 10)
    assert result.verdict == "pass", result.error or result.serial_log
    assert avr8js.run(comp.hex_path, 10).verdict == "fail"


@requires_arduino
@pytest.mark.arduino
def test_multifile_module_runs_on_avr8js(tmp_path, avr8js):
    source = (COUNTER / "tests" / "test_main.ino").read_text(encoding="utf-8")
    comp = compile_sketch(stage_test_sketch(COUNTER, source, tmp_path), tmp_path / "build")
    assert comp.ok, comp.stderr
    result = avr8js.run(comp.hex_path, 15)
    assert result.verdict == "pass" and (result.tests_passed, result.tests_total) == (3, 3), result.serial_log
    broken = module_sources(COUNTER)["display.cpp"].replace("v % 10", "v % 9")
    comp = compile_sketch(stage_test_sketch(COUNTER, source, tmp_path / "m", overrides={"display.cpp": broken}),
                          tmp_path / "m" / "build")
    assert comp.ok and avr8js.run(comp.hex_path, 15).verdict == "fail"


def test_experiment2_modules_get_runner_v2(tmp_path):
    from framework.harness.sketch import TEST_RUNNER_H, TEST_RUNNER_V2_H
    assert (stage_test_sketch(COUNTER, "// t\n", tmp_path / "a") / "test_runner.h").read_text() == TEST_RUNNER_V2_H.read_text()
    blink = COUNTER.parent / "blink_fixture"
    assert (stage_test_sketch(blink, "// t\n", tmp_path / "b") / "test_runner.h").read_text() == TEST_RUNNER_H.read_text()


@requires_arduino
@pytest.mark.arduino
def test_runner_v2_runs_more_than_16_tests(tmp_path, avr8js):
    tests = "".join(f"TEST(case_number_{i:02d}) {{ cnt_init(); cnt_add({i}); ASSERT_EQ(cnt_value(), {i}); }}\n"
                    for i in range(30))
    source = f'#include "logic.h"\n#include "test_runner.h"\n{tests}' \
             "void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }\nvoid loop() {}\n"
    comp = compile_sketch(stage_test_sketch(COUNTER, source, tmp_path), tmp_path / "build")
    assert comp.ok, comp.stderr
    result = avr8js.run(comp.hex_path, 15)
    assert result.verdict == "pass" and (result.tests_passed, result.tests_total) == (30, 30), result.serial_log
    assert any(t.name == "case_number_29" for t in result.tests)


@requires_arduino
@pytest.mark.arduino
def test_avr8js_has_eeprom_and_spi(tmp_path, avr8js):
    """EEPROM writes wait for EEPE and SPI.transfer() for SPIF: without the peripherals both hang."""
    source = ('#include "logic.h"\n#include "test_runner.h"\n#include <EEPROM.h>\n#include <SPI.h>\n'
              "TEST(eeprom_starts_erased_and_keeps_writes) { ASSERT_EQ(EEPROM.read(5), 255); EEPROM.write(5, 42);"
              " ASSERT_EQ(EEPROM.read(5), 42); }\n"
              "TEST(spi_transfer_returns) { SPI.begin(); ASSERT_EQ(SPI.transfer(0x55), 0); }\n"
              "void setup() { Serial.begin(9600); RUN_ALL_TESTS(); }\nvoid loop() {}\n")
    comp = compile_sketch(stage_test_sketch(COUNTER, source, tmp_path), tmp_path / "build")
    assert comp.ok, comp.stderr
    result = avr8js.run(comp.hex_path, 10)
    assert result.verdict == "pass" and result.tests_passed == 2, result.serial_log
