from __future__ import annotations

import os
import shutil
from pathlib import Path

import yaml

REPO_ROOT = Path(__file__).resolve().parents[2]
CONFIG_PATH = REPO_ROOT / "configs" / "experiment.yaml"
_env_loaded = False


def load_dotenv(path: Path = REPO_ROOT / ".env") -> None:
    global _env_loaded
    _env_loaded = True
    if not path.exists():
        return
    for line in path.read_text(encoding="utf-8").splitlines():
        key, sep, value = line.strip().partition("=")
        if sep and key and not key.startswith("#"):
            os.environ.setdefault(key.strip(), value.strip().strip("\"'"))


def load_config(path: Path = CONFIG_PATH) -> dict:
    if not _env_loaded:
        load_dotenv()
    return yaml.safe_load(path.read_text(encoding="utf-8")) or {}


def resolve_path(p: str | Path) -> Path:
    p = Path(p)
    return p if p.is_absolute() else REPO_ROOT / p


def tool_path(cfg: dict, key: str, exe: str) -> Path | None:
    configured = (cfg.get(key) or {}).get("path")
    if configured and resolve_path(configured).exists():
        return resolve_path(configured)
    found = shutil.which(exe)
    return Path(found) if found else None


def get_secret(name: str) -> str | None:
    if not _env_loaded:
        load_dotenv()
    return os.environ.get(name) or None
