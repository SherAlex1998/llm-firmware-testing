"""Documents for the libraries of arduino_cli.libraries, taken from the installed (pinned) versions.

    python -m framework.rag.exp2_library_docs            # writes kb/exp2/libraries/<lib>_{readme,api,example_*}.md

Per library: README, public headers and up to two examples. Servo and Stepper are already in kb/libraries/.
"""
from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path

from framework.common.config import REPO_ROOT, load_config
from framework.harness.compile import find_arduino_cli

OUT = REPO_ROOT / "kb" / "exp2" / "libraries"
SKIP = {"Servo", "Stepper"}
MAX_EXAMPLES = 2
MAX_HEADER_CHARS = 12000


def installed(cfg: dict) -> dict[str, dict]:
    out = subprocess.run([str(find_arduino_cli(cfg)), "lib", "list", "--format", "json"], capture_output=True,
                         text=True, encoding="utf-8", errors="replace").stdout
    return {it["library"]["name"]: it["library"] for it in json.loads(out)["installed_libraries"]}


def slug(name: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", name.lower()).strip("_")


def license_of(folder: Path) -> str:
    for name in ("LICENSE", "LICENSE.txt", "LICENSE.md", "COPYING", "license.txt"):
        p = folder / name
        if p.exists():
            first = next((line.strip() for line in p.read_text(encoding="utf-8", errors="replace").splitlines()
                          if line.strip()), "")
            return first[:80]
    return "see library sources (no LICENSE file)"


def public_headers(lib: dict, folder: Path) -> list[Path]:
    names = lib.get("provides_includes") or []
    found = []
    for n in names:
        for base in (folder / "src", folder):
            if (base / n).exists():
                found.append(base / n)
                break
    return found


OTHER_HW = re.compile(r"attiny|esp|ds1302|ds3231|ds3234|ds1342|ds1388|pcf85|rgbw|teensy|samd|rp2040|stm32", re.I)


def examples(folder: Path) -> list[Path]:
    """Examples for an Uno with the parts used here: names of other chips/boards are skipped."""
    ex = sorted(p for p in (folder / "examples").rglob("*.ino") if not OTHER_HW.search(str(p.relative_to(folder)))) \
        if (folder / "examples").exists() else []

    def score(p: Path) -> tuple:
        name = p.stem.lower()
        s = 3 * bool(re.search(r"simple|basic|hello", name)) + 2 * bool(re.search(r"example|recv|receive|ds18", name)) \
            + bool(re.search(r"demo|test", name)) - 3 * bool(re.search(r"send|bose|ds2408|\d+sensor|multi|timer", name))
        return (-s, len(name), name)
    return sorted(ex, key=score)[:MAX_EXAMPLES]


def header_comment(url: str, lic: str, version: str) -> str:
    return f"<!-- source: {url} (version {version}) | license: {lic} -->\n\n"


def write_docs(name: str, lib: dict) -> list[str]:
    folder = Path(lib["install_dir"])
    url, version, lic = lib.get("website") or "", lib["version"], license_of(folder)
    head, s, written = header_comment(url, lic, version), slug(name), []
    readme = next((folder / n for n in ("README.md", "README.adoc", "readme.md", "README") if (folder / n).exists()), None)
    if readme:
        (OUT / f"{s}_readme.md").write_text(head + f"# {name} library: README\n\n"
                                            + readme.read_text(encoding="utf-8", errors="replace"), encoding="utf-8")
        written.append(f"{s}_readme.md")
    headers = public_headers(lib, folder)
    if headers:
        body = [head, f"# {name} library: API\n\n",
                f"{lib.get('sentence', '')} {lib.get('paragraph', '')}".strip() + "\n\n"]
        for h in headers:
            text = h.read_text(encoding="utf-8", errors="replace")[:MAX_HEADER_CHARS]
            body += [f"## {h.name}\n\n", "```cpp\n", text.rstrip(), "\n```\n\n"]
        (OUT / f"{s}_api.md").write_text("".join(body), encoding="utf-8")
        written.append(f"{s}_api.md")
    for ex in examples(folder):
        out = OUT / f"{s}_example_{slug(ex.stem)}.md"
        out.write_text(head + f"# {name} library: example {ex.stem}\n\n```cpp\n"
                       + ex.read_text(encoding="utf-8", errors="replace").rstrip() + "\n```\n", encoding="utf-8")
        written.append(out.name)
    return written


def main() -> int:
    cfg = load_config()
    OUT.mkdir(parents=True, exist_ok=True)
    have = installed(cfg)
    rows = []
    for spec in cfg["arduino_cli"].get("libraries") or []:
        name, version = spec.rsplit("@", 1)
        if name in SKIP:
            continue
        lib = have.get(name)
        if lib is None or lib["version"] != version:
            print(f"{name}@{version}: not installed (run compile.ensure_libraries())")
            return 2
        files = write_docs(name, lib)
        rows.append((name, version, lib.get("website") or "", license_of(Path(lib["install_dir"])), files))
        print(f"{name}@{version}: {', '.join(files)}")
    lines = ["# Experiment 2 library documents (kb/exp2/libraries)", "",
             "Generated by `python -m framework.rag.exp2_library_docs` from the installed, pinned library versions.", "",
             "| Library | Version | Source | License | Files |", "|---|---|---|---|---|"]
    lines += [f"| {n} | {v} | {u} | {lic} | {len(f)} |" for n, v, u, lic, f in rows]
    sources = OUT.parent / "SOURCES.md"                      # outside the indexed folders
    old = sources.read_text(encoding="utf-8") if sources.exists() else ""
    tail = old[old.index("\n## "):] if "\n## " in old else ""   # later sections (reference, guides, devices) kept
    sources.write_text("\n".join(lines) + "\n" + tail, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
