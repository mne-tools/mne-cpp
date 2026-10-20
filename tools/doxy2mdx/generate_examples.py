#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     October, 2026
#
#
# @brief    Generate the website examples gallery from src/examples.
"""Generate the website examples gallery from ``src/examples``.

Every ``src/examples/ex_*`` directory becomes one page under
``doc/website/docs/examples/<library>/``: the ``@brief`` of its ``main.cpp``,
the libraries it uses (linked to their API overview), its command-line options,
how to build and run it, the API pages that quote it, and the full source.
Examples are grouped by the most specific library they include.

Usage
-----
    python3 tools/doxy2mdx/generate_examples.py           # rewrite the pages
    python3 tools/doxy2mdx/generate_examples.py --check   # fail when stale

Exit codes
----------
    0   pages written, or committed pages are current
    1   committed pages are stale, missing or orphaned
"""

from __future__ import annotations

import argparse
import functools
import json
import re
import sys
import tempfile
import textwrap
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
EXAMPLES = Path("src/examples")
OUT = Path("doc/website/docs/examples")
API = Path("doc/website/docs/api")

# Most specific first: an example is filed under the first library it includes.
LIBRARIES = {
    "connectivity": "Connectivity", "decoding": "Decoding", "ml": "Machine Learning",
    "sts": "Statistics", "inv": "Inverse", "fwd": "Forward", "disp3D": "3D Visualization",
    "disp": "Visualization", "dsp": "Signal Processing", "ieeg": "iEEG", "bids": "BIDS",
    "lsl": "Lab Streaming Layer", "mna": "MNA Pipelines", "mri": "MRI", "fs": "FreeSurfer",
    "mne": "MNE Core", "fiff": "FIFF I/O", "math": "Math", "utils": "Utilities",
}
INCLUDE = re.compile(r"^#include\s*<(\w+)/", re.M)
BRIEF = re.compile(r"@brief\s+(.+?)\s*(?:\*/)?$", re.M)
NOT_CODE = re.compile(r"^\s*(#|using\b|//|\*|/\*)")
SNIPPET_MARKER = re.compile(r"^\s*//! \[\w+\]\s*$")
EXCERPT_CONTEXT = 3
EXCERPT_LINES = 14
OPTION = re.compile(r'QCommandLineOption\s+\w+\(\s*(\{[^}]*\}|"[^"]*")\s*,\s*"((?:[^"\\]|\\.)*)"')


def api_slug(lib: str) -> str:
    return re.sub(r"(?<=[0-9a-z])([A-Z])", r"-\1", lib).lower()


def mdx_text(text: str) -> str:
    """Escape characters that MDX would read as table cells, JSX or expressions."""
    for char, entity in {"&": "&amp;", "|": "&#124;", "<": "&lt;", ">": "&gt;", "{": "&#123;", "}": "&#125;"}.items():
        text = text.replace(char, entity)
    return text


def quoted_by(api_dir: Path) -> dict[str, list[tuple[str, str]]]:
    """Example name -> [(label, page path relative to docs/)] of API pages that quote it."""
    refs: dict[str, list[tuple[str, str]]] = {}
    for page in sorted(api_dir.rglob("*.mdx")):
        text = page.read_text(encoding="utf-8")
        label = re.search(r"^sidebar_label:\s*(.+)$", text, re.M)
        for name in sorted(set(re.findall(r"src/examples/(ex_\w+)/", text))):
            refs.setdefault(name, []).append(
                (label.group(1).strip() if label else page.stem, page.relative_to(api_dir.parent).as_posix()))
    return refs


@functools.lru_cache(maxsize=None)
def example_info(src: Path) -> tuple[list[str], str, dict[str, list[str]]]:
    """(libraries most specific first, brief, {file name: lines}) of one example directory."""
    texts = {p.name: p.read_text(encoding="utf-8") for p in sorted(src.iterdir()) if p.suffix in {".cpp", ".h"}}
    match = BRIEF.search(texts["main.cpp"])
    brief = match.group(1).strip().rstrip(".") if match else src.name
    used = {m for text in texts.values() for m in INCLUDE.findall(text)}
    return [lib for lib in LIBRARIES if lib in used], brief, {n: t.splitlines() for n, t in texts.items()}


def _excerpt(lines: list[str], hit: int) -> tuple[int, list[str]]:
    """0-based start and dedented lines of the statement block around line *hit*."""
    start = hit
    while start > 0 and hit - start < EXCERPT_CONTEXT and lines[start - 1].strip() \
            and not lines[start - 1].rstrip().endswith(("{", "}")):
        start -= 1
    while start < hit and SNIPPET_MARKER.match(lines[start]):
        start += 1
    end = hit + 1
    while end < len(lines) and end - start < EXCERPT_LINES and lines[end].strip():
        end += 1
    block = [line for line in lines[start:end] if not SNIPPET_MARKER.match(line)]
    return start, textwrap.dedent("\n".join(block)).splitlines()


def usages(name: str, library: str, root: Path = REPO_ROOT) -> list[dict]:
    """Examples using class *name* of *library*, most uses first, each with an excerpt of the first use."""
    word = re.compile(rf"(?<!\w){re.escape(name)}\b")
    found = []
    for src in sorted((root / EXAMPLES).glob("ex_*/")):
        libs, brief, files = example_info(src)
        if library not in libs:
            continue
        hits = {f: [i for i, line in enumerate(lines) if word.search(line) and not NOT_CODE.match(line)]
                for f, lines in files.items()}
        count = sum(map(len, hits.values()))
        if not count:
            continue
        file = next(f for f in files if hits[f])
        start, excerpt = _excerpt(files[file], hits[file][0])
        found.append({"name": src.name, "group": libs[0], "brief": brief, "uses": count,
                      "file": f"{EXAMPLES.as_posix()}/{src.name}/{file}", "line": start + 1, "code": excerpt})
    return sorted(found, key=lambda u: (-u["uses"], u["name"]))


def render(name: str, src: Path, refs: list[tuple[str, str]]) -> tuple[str, str, str]:
    """Return (library, brief, page text) for one example directory."""
    libs, brief, files = example_info(src)
    main = "\n".join(files["main.cpp"])
    group = libs[0]
    lines = [
        "---", f"title: {json.dumps(name)}", f"description: {json.dumps(brief)}", "---", "",
        "{/* Generated by tools/doxy2mdx/generate_examples.py from src/examples — do not edit. */}", "",
        f"# {name}", "", f"{mdx_text(brief)}.", "",
        "**Libraries:** " + ", ".join(f"[{LIBRARIES[lib]}](../../api/{api_slug(lib)}/index.mdx)" for lib in libs), "",
    ]
    options = [(o.replace('"', "").strip("{}").replace(",", ", "), d) for o, d in OPTION.findall(main)]
    if options:
        lines += ["## Options", "", "| Option | Description |", "|---|---|"]
        lines += [f"| `--{o.split(', ')[0]}` | {mdx_text(d)} |" for o, d in options]
        lines.append("")
    lines += ["## Build and run", "", "```bash", f"cmake --build build/developer-dynamic --target {name}",
              f"./out/Release/examples/{name}" + (" --help" if options else ""), "```", ""]
    if "MNE-sample-data" in main:
        lines += ["The default inputs come from the [MNE sample dataset](../../manual/sample-dataset.mdx).", ""]
    if refs:
        lines += ["## Used in the API reference", ""] + [f"- [{label}](../../{path})" for label, path in refs] + [""]
    lines += ["## Source", ""]
    for file, code in files.items():
        lines += [f"````cpp title=\"{EXAMPLES.as_posix()}/{name}/{file}\"", "\n".join(code).rstrip("\n"), "````", ""]
    return group, brief, "\n".join(lines)


def generate(root: Path, out: Path) -> None:
    refs = quoted_by(root / API)
    groups: dict[str, list[tuple[str, str]]] = {}
    for src in sorted((root / EXAMPLES).glob("ex_*/")):
        group, brief, text = render(src.name, src, refs.get(src.name, []))
        (out / group).mkdir(parents=True, exist_ok=True)
        (out / group / f"{src.name}.mdx").write_text(text + "\n", encoding="utf-8")
        groups.setdefault(group, []).append((src.name, brief))
    order = list(LIBRARIES)
    index = ["---", "title: Examples", "sidebar_position: 0", "slug: /examples", "---", "",
             "{/* Generated by tools/doxy2mdx/generate_examples.py from src/examples — do not edit. */}", "",
             "# Examples", "",
             f"{sum(map(len, groups.values()))} runnable programs in `src/examples`, built with the rest of "
             "MNE-CPP. Each page lists the libraries the example uses, its options, and its full source.", ""]
    for group in sorted(groups, key=order.index):
        (out / group / "_category_.json").write_text(
            json.dumps({"label": LIBRARIES[group], "position": order.index(group) + 1}, indent=2) + "\n",
            encoding="utf-8")
        index += [f"## {LIBRARIES[group]}", "", "| Example | What it shows |", "|---|---|"]
        index += [f"| [{name}](./{group}/{name}.mdx) | {mdx_text(brief)} |" for name, brief in groups[group]] + [""]
    (out / "index.mdx").write_text("\n".join(index), encoding="utf-8")


def files(directory: Path) -> dict[str, bytes]:
    return {p.relative_to(directory).as_posix(): p.read_bytes() for p in directory.rglob("*") if p.is_file()}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="fail when the committed pages differ")
    args = parser.parse_args(argv)
    if not args.check:
        for stale in sorted((REPO_ROOT / OUT).rglob("*"), reverse=True) if (REPO_ROOT / OUT).exists() else []:
            stale.unlink() if stale.is_file() else stale.rmdir()
        (REPO_ROOT / OUT).mkdir(parents=True, exist_ok=True)
        generate(REPO_ROOT, REPO_ROOT / OUT)
        print(f"wrote {OUT}")
        return 0
    with tempfile.TemporaryDirectory() as tmp:
        generate(REPO_ROOT, Path(tmp))
        fresh, committed = files(Path(tmp)), files(REPO_ROOT / OUT) if (REPO_ROOT / OUT).exists() else {}
    bad = sorted(k for k in fresh.keys() | committed.keys() if fresh.get(k) != committed.get(k))
    for rel in bad:
        print(f"FAIL: {OUT / rel} is stale; run python3 tools/doxy2mdx/generate_examples.py")
    if not bad:
        print(f"PASS: {OUT} matches src/examples.")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
