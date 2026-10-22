from pathlib import Path

import pytest

from framework.harness.backends import get_backend
from framework.harness.run_wokwi import WokwiBackend


def test_get_backend():
    assert get_backend("wokwi").name == "wokwi"
    assert get_backend("board").name == "board"
    with pytest.raises(ValueError):
        get_backend("nope")


def test_missing_cli_is_error(tmp_path):
    backend = WokwiBackend(cli=Path("C:/missing/wokwi-cli.exe"), token="")
    result = backend.run(tmp_path / "x.ino.hex", 1)
    assert result.verdict == "error" and "wokwi-cli" in result.error


def test_missing_firmware_is_error(tmp_path):
    backend = WokwiBackend(cli=Path(__file__), token="dummy")
    result = backend.run(tmp_path / "missing.ino.hex", 1)
    assert result.verdict == "error" and "firmware not found" in result.error
