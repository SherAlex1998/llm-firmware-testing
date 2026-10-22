"""Download the official Arduino documentation that makes up the knowledge base into kb/ as markdown.

    python -m framework.rag.fetch_docs

Sources and licenses go into every file header and into kb/SOURCES.md; kb/guides/ is hand-written and not touched.
"""
from __future__ import annotations

import re
import sys
import urllib.parse
import urllib.request

from framework.common.config import REPO_ROOT

KB = REPO_ROOT / "kb"
RAW = "https://raw.githubusercontent.com"
REFERENCE = ("arduino/reference-en/master", "CC BY-SA 3.0", [
    "Language/Functions/Digital IO/pinMode", "Language/Functions/Digital IO/digitalWrite",
    "Language/Functions/Digital IO/digitalRead", "Language/Functions/Analog IO/analogWrite",
    "Language/Functions/Analog IO/analogRead", "Language/Functions/Time/delay",
    "Language/Functions/Time/delayMicroseconds", "Language/Functions/Time/millis", "Language/Functions/Time/micros",
    "Language/Functions/Math/map", "Language/Functions/Math/constrain", "Language/Functions/Math/abs",
    "Language/Functions/Math/min", "Language/Functions/Math/max", "Language/Functions/Bits and Bytes/bitRead",
    "Language/Functions/Bits and Bytes/bitWrite", "Language/Functions/Bits and Bytes/bitSet",
    "Language/Functions/Bits and Bytes/bitClear", "Language/Variables/Constants/highLow",
    "Language/Variables/Constants/inputOutputPullup", "Language/Variables/Constants/integerConstants",
    "Language/Variables/Constants/ledbuiltin"])
DOCS = ("arduino/docs-content/main/content", "CC BY-SA 4.0", {        # path under content/ -> file in kb/
    "learn/02.microcontrollers/01.digital-pins/digital-pins": "learn/digital-pins",
    "learn/02.microcontrollers/02.analog-input/analog-input": "learn/analog-input",
    "learn/02.microcontrollers/03.analog-output/analog-output": "learn/analog-output",
    "learn/04.electronics/04.stepper-motors/stepper-library-examples": "learn/stepper-motors",
    "learn/04.electronics/05.servo-motors/servo-motors": "learn/servo-motors",
    "learn/04.electronics/08.potentiometer-basics/potentiometer-basics": "learn/potentiometer-basics",
    "learn/04.electronics/09.transistor-motor-control/09.transistor-motor-control": "learn/transistor-motor-control",
    "hardware/02.uno/shields/motor-shield-rev3/tutorials/msr3-controlling-dc-motor/msr3-controlling-dc-motor":
        "learn/motor-shield-dc-motor",
    **{f"built-in-examples/{d}/{n}/{n}": f"examples/{n}" for d, names in {
        "02.digital": ["BlinkWithoutDelay", "Button", "Debounce", "InputPullupSerial", "StateChangeDetection"],
        "03.analog": ["AnalogInput", "AnalogInOutSerial", "Fading"],
        "05.control-structures": ["Arrays", "ForLoopIteration", "SwitchCase"]}.items() for n in names}})
LIBRARIES = [("arduino-libraries/Servo/master/docs", "LGPL-2.1", "servo"),
             ("arduino-libraries/Stepper/master/docs", "LGPL-2.1", "stepper")]


def get(url: str) -> str:
    with urllib.request.urlopen(urllib.parse.quote(url, safe=":/")) as resp:
        return resp.read().decode("utf-8")


def strip_front_matter(text: str) -> str:
    return re.sub(r"\A\s*---\n.*?\n---\n", "", text, count=1, flags=re.S)


def adoc_to_md(text: str) -> str:
    """Just enough AsciiDoc for the Arduino reference pages."""
    text = strip_front_matter(text)
    text = re.split(r"\n=== See also\b", text)[0]
    out, in_code = [], False
    for line in text.splitlines():
        if line.strip() == "----":
            out.append("```" if in_code else "```cpp")
            in_code = not in_code
            continue
        if in_code:
            out.append(line)
            continue
        if re.match(r"\s*(//|:|\[[^\]]*\]\s*$|--\s*$|image::)", line):
            continue
        heading = re.match(r"(=+)\s+(.*)", line)
        if heading:
            line = "#" * max(1, len(heading.group(1)) - 1) + " " + heading.group(2)
        line = re.sub(r"link:[^\[\s]*\[([^\]]*)\]", r"\1", line)
        out.append(line)
    return re.sub(r"\n{3,}", "\n\n", "\n".join(out)).strip() + "\n"


def md_clean(text: str) -> str:
    title = re.match(r"\A\s*---\n.*?^title:\s*['\"]?(.*?)['\"]?\s*$", text, flags=re.S | re.M)
    text = strip_front_matter(text)
    if title and not text.lstrip().startswith("# "):
        text = f"# {title.group(1)}\n\n{text}"
    text = re.sub(r"^\s*!\[[^\]]*\]\([^)]*\)\s*$", "", text, flags=re.M)          # images
    text = re.sub(r"^\s*<[^>]+>\s*$", "", text, flags=re.M)                       # html/video components
    return re.sub(r"\n{3,}", "\n\n", text).strip() + "\n"


def save(path, url: str, license_: str, body: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(f"<!-- source: {url} | license: {license_} -->\n\n{body}", encoding="utf-8")
    print(f"{path.relative_to(REPO_ROOT)}  ({len(body)} chars)")


def main() -> int:
    sources = []
    repo, lic, pages = REFERENCE
    for page in pages:
        url = f"{RAW}/{repo}/{page}.adoc"
        save(KB / "reference" / f"{page.rsplit('/', 1)[1]}.md", url, lic, adoc_to_md(get(url)))
    sources.append(f"| `kb/reference/` | https://github.com/{repo.split('/master')[0]} | {lic} |")
    repo, lic, pages = DOCS
    for page, dest in pages.items():
        url = f"{RAW}/{repo}/{page}.md"
        save(KB / f"{dest}.md", url, lic, md_clean(get(url)))
    sources.append(f"| `kb/learn/`, `kb/examples/` | https://github.com/arduino/docs-content | {lic} |")
    for repo, lic, name in LIBRARIES:
        for doc in ("readme", "api"):
            url = f"{RAW}/{repo}/{doc}.md"
            save(KB / "libraries" / f"{name}_{doc}.md", url, lic, md_clean(get(url)))
        sources.append(f"| `kb/libraries/{name}_*.md` | https://github.com/{repo.split('/master')[0]} | {lic} |")
    (KB / "SOURCES.md").write_text(
        "# Knowledge base sources\n\nGenerated by `python -m framework.rag.fetch_docs`; `kb/guides/` is hand-written.\n\n"
        "| Files | Source | License |\n|---|---|---|\n" + "\n".join(sources) + "\n"
        "| `kb/guides/` | written for this project from the Arduino AVR core 1.8.8 and Servo 1.3.0 sources | — |\n",
        encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
