"""Arduino reference pages for the core API used by the experiment 2 modules.

    python -m framework.rag.exp2_reference_docs     # writes kb/exp2/reference/<name>.md for pages not in kb/reference/

A page is fetched only if its identifier occurs in the sources of an experiment 2 module.
"""
from __future__ import annotations

import re
import sys
import urllib.error
from pathlib import Path

import yaml

from framework.common.config import REPO_ROOT
from framework.rag.fetch_docs import RAW, adoc_to_md, get, save

REPO, LICENSE = "arduino/reference-en/master", "CC BY-SA 3.0"
OUT = REPO_ROOT / "kb" / "exp2" / "reference"
PAGES = {   # identifier in the code -> reference page
    "attachInterrupt": "Language/Functions/External Interrupts/attachInterrupt",
    "detachInterrupt": "Language/Functions/External Interrupts/detachInterrupt",
    "digitalPinToInterrupt": "Language/Functions/External Interrupts/digitalPinToInterrupt",
    "interrupts": "Language/Functions/Interrupts/interrupts",
    "noInterrupts": "Language/Functions/Interrupts/noInterrupts",
    "pulseIn": "Language/Functions/Advanced IO/pulseIn",
    "shiftOut": "Language/Functions/Advanced IO/shiftOut",
    "tone": "Language/Functions/Advanced IO/tone",
    "noTone": "Language/Functions/Advanced IO/noTone",
    "random": "Language/Functions/Random Numbers/random",
    "randomSeed": "Language/Functions/Random Numbers/randomSeed",
    "PROGMEM": "Language/Variables/Utilities/PROGMEM",
    "sizeof": "Language/Variables/Utilities/sizeof",
    "volatile": "Language/Variables/Variable Scope & Qualifiers/volatile",
    "static": "Language/Variables/Variable Scope & Qualifiers/static",
    "String": "Language/Variables/Data Types/stringObject",
    "analogReference": "Language/Functions/Analog IO/analogReference",
    "lowByte": "Language/Functions/Bits and Bytes/lowByte",
    "highByte": "Language/Functions/Bits and Bytes/highByte",
}


def used_identifiers() -> set[str]:
    code = ""
    for path in (REPO_ROOT / "benchmark").glob("*/module.yaml"):
        meta = yaml.safe_load(path.read_text(encoding="utf-8"))
        if meta.get("experiment") == 2:
            code += "".join(p.read_text(encoding="utf-8") for p in (path.parent / "src").iterdir()
                            if p.suffix in (".h", ".hpp", ".cpp", ".c"))
    return {name for name in PAGES if re.search(rf"\b{name}\b", code)}


def main() -> int:
    have = {p.stem for p in (REPO_ROOT / "kb" / "reference").glob("*.md")}
    for name in sorted(used_identifiers()):
        page = PAGES[name]
        stem = page.rsplit("/", 1)[1]
        if stem in have:
            continue
        url = f"{RAW}/{REPO}/{page}.adoc"
        try:
            save(OUT / f"{stem}.md", url, LICENSE, adoc_to_md(get(url)))
        except urllib.error.HTTPError as e:
            print(f"{name}: {e}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
