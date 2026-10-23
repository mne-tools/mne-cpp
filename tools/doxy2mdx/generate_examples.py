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
``doc/website/docs/examples/<library>/``: the ``@brief`` and description of its
``main.cpp``, the scientific topics it demonstrates (with the manual sections
and API pages of each), the data it needs, how to build and run it, its
command-line options, the registered classes it uses, the API pages that quote
it, related examples and the full source.

Examples are filed under their primary library: the ``library`` of their entry
in ``doc/api_registry.json`` ``examples``, else the most specific library they
include. A page that moved away from its include-derived directory leaves a
redirect under ``doc/website/static`` so old links keep working.

``doc/website/src/data/knowledge.json`` carries the resolved topics and the
explorer metadata (tasks, modalities, level, libraries) to the website's
``Connections`` and ``ExamplesExplorer`` components.

Usage
-----
    python3 tools/doxy2mdx/generate_examples.py           # rewrite the pages
    python3 tools/doxy2mdx/generate_examples.py --check   # fail when stale

Exit codes
----------
    0   pages written, or committed pages are current
    1   committed pages are stale, missing or orphaned, or a registry reference does not resolve
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

sys.path.insert(0, str(Path(__file__).resolve().parent))

import knowledge as kn  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parents[2]
EXAMPLES = Path("src/examples")
OUT = Path("doc/website/docs/examples")
API = Path("doc/website/docs/api")
DATA = Path("doc/website/src/data/knowledge.json")
REDIRECTS = Path("doc/website/static/docs/examples")
GITHUB_BLOB = "https://github.com/mne-tools/mne-cpp/blob/staging"

# Most specific first: an example is filed under the first library it includes.
LIBRARIES = {
    "connectivity": "Connectivity", "decoding": "Decoding", "ml": "Machine Learning",
    "sts": "Statistics", "inv": "Inverse", "fwd": "Forward", "disp3D": "3D Visualization",
    "disp": "Visualization", "dsp": "Signal Processing", "ieeg": "iEEG", "bids": "BIDS",
    "lsl": "Lab Streaming Layer", "mna": "MNA Pipelines", "mri": "MRI", "fs": "FreeSurfer",
    "mne": "MNE Core", "fiff": "FIFF I/O", "math": "Math", "utils": "Utilities",
}
DATASETS = {
    "MNE-sample-data": "the [MNE sample dataset](../../manual/sample-dataset.mdx) in "
                       "`resources/data/MNE-sample-data`",
    "mne-cpp-test-data": "the [mne-cpp-test-data](https://github.com/mne-tools/mne-cpp-test-data) repository "
                         "cloned to `resources/data/mne-cpp-test-data`",
}
INCLUDE = re.compile(r"^#include\s*<(\w+)/", re.M)
BRIEF = re.compile(r"@brief\s+(.+?)\s*(?:\*/)?$", re.M)
NOT_CODE = re.compile(r"^\s*(#|using\b|//|\*|/\*)")
SNIPPET_MARKER = re.compile(r"^\s*//! \[\w+\]\s*$")
EXCERPT_CONTEXT = 3
EXCERPT_LINES = 14
KEY_CLASSES = 8
# Every example installs the logger; listing it as key API says nothing.
BOILERPLATE = {"MNELogger"}
RELATED = 5
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


def _header_text(main: str) -> tuple[str, str]:
    """(brief, description) of the file header: the ``@brief`` with its continuation lines and the paragraphs after it."""
    match = BRIEF.search(main)
    if not match:
        return "", ""
    lines = main[match.start():].splitlines()
    brief = [match.group(1).strip()]
    rest = []
    target = brief
    for line in lines[1:]:
        if "*/" in line:
            break
        body = re.sub(r"^\s*\*\s?", "", line).rstrip()
        if body.startswith("@"):
            break
        if not body.strip():
            target = rest
            if rest and rest[-1]:
                rest.append("")
            continue
        target.append(body.strip())
    paragraphs = "\n".join(rest).strip()
    return " ".join(brief), re.sub(r"(?<!\n)\n(?!\n)", " ", paragraphs)


@functools.lru_cache(maxsize=None)
def example_info(src: Path) -> tuple[list[str], str, dict[str, list[str]]]:
    """(libraries most specific first, brief, {file name: lines}) of one example directory."""
    texts = {p.name: p.read_text(encoding="utf-8") for p in sorted(src.iterdir()) if p.suffix in {".cpp", ".h"}}
    brief = _header_text(texts["main.cpp"])[0].rstrip(".") or src.name
    used = {m for text in texts.values() for m in INCLUDE.findall(text)}
    return [lib for lib in LIBRARIES if lib in used], brief, {n: t.splitlines() for n, t in texts.items()}


@functools.lru_cache(maxsize=None)
def _registry_examples(root: Path) -> dict[str, dict]:
    path = root / kn.REGISTRY
    if not path.is_file():
        return {}
    return json.loads(path.read_text(encoding="utf-8")).get("examples", {}).get("entries", {})


def primary_library(src: Path, root: Path = REPO_ROOT) -> str | None:
    """Directory an example's page lives in: the registry override, else its most specific library."""
    override = _registry_examples(root).get(src.name, {}).get("library")
    libraries = example_info(src)[0]
    return override or (libraries[0] if libraries else None)


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


def _uses(name: str, files: dict[str, list[str]]) -> dict[str, list[int]]:
    word = re.compile(rf"(?<!\w){re.escape(name)}\b")
    return {f: [i for i, line in enumerate(lines) if word.search(line) and not NOT_CODE.match(line)]
            for f, lines in files.items()}


def usages(name: str, library: str, root: Path = REPO_ROOT) -> list[dict]:
    """Examples using class *name* of *library*, most uses first, each with an excerpt of the first use."""
    found = []
    for src in sorted((root / EXAMPLES).glob("ex_*/")):
        libs, brief, files = example_info(src)
        if library not in libs:
            continue
        hits = _uses(name, files)
        count = sum(map(len, hits.values()))
        if not count:
            continue
        file = next(f for f in files if hits[f])
        start, excerpt = _excerpt(files[file], hits[file][0])
        found.append({"name": src.name, "group": primary_library(src, root), "brief": brief, "uses": count,
                      "file": f"{EXAMPLES.as_posix()}/{src.name}/{file}", "line": start + 1, "code": excerpt})
    return sorted(found, key=lambda u: (-u["uses"], u["name"]))


def key_classes(src: Path, pages: dict[str, tuple[str, Path]]) -> list[tuple[str, str, int]]:
    """(class, API URL, uses) of the documented classes an example uses, most used first."""
    libs, _brief, files = example_info(src)
    found = []
    for name, (url, page) in pages.items():
        if name in BOILERPLATE or page.parent.name not in {api_slug(lib) for lib in libs}:
            continue
        count = sum(map(len, _uses(name, files).values()))
        if count:
            found.append((name, url, count))
    return sorted(found, key=lambda c: (-c[2], c[0]))[:KEY_CLASSES]


def render(name: str, src: Path, refs: list[tuple[str, str]], know: kn.Knowledge | None = None,
           related: list[str] | None = None, root: Path = REPO_ROOT) -> tuple[str, str, str]:
    """Return (library, brief, page text) for one example directory."""
    libs, brief, files = example_info(src)
    main = "\n".join(files["main.cpp"])
    description = _header_text(main)[1]
    group = primary_library(src, root)
    meta = know.examples.get(name) if know else None
    topics = kn.topics_of_example(know, name) if know else []
    lines = [
        "---", f"title: {json.dumps(name)}", f"description: {json.dumps(brief)}", "---", "",
        "{/* Generated by tools/doxy2mdx/generate_examples.py from src/examples — do not edit. */}", "",
        f"# {name}", "", f"{mdx_text(brief)}.", "",
    ]
    if meta:
        lines += [f'<ExampleFacts name="{name}" />', ""]
    lines += ["**Libraries:** " + ", ".join(f"[{LIBRARIES[lib]}](../../api/{api_slug(lib)}/index.mdx)" for lib in libs), ""]
    if description:
        lines += ["## What it demonstrates", "", mdx_text(description), ""]
    if topics:
        lines += ["## Background", "", f"<Connections topics={{{json.dumps(topics)}}} context=\"example\" "
                                       f"current=\"{name}\" />", ""]
    data = [text for marker, text in DATASETS.items() if marker in main]
    lines += ["## Build and run", ""]
    if data:
        lines += ["Needs " + " and ".join(data) + ".", ""]
    options = [(o.replace('"', "").strip("{}").replace(",", ", "), d) for o, d in OPTION.findall(main)]
    lines += ["```bash", f"cmake --build build/developer-dynamic --target {name}",
              f"./out/Release/examples/{name}" + (" --help" if options else ""), "```", ""]
    if options:
        lines += ["### Options", "", "| Option | Description |", "|---|---|"]
        lines += [f"| `--{o.split(', ')[0]}` | {mdx_text(d)} |" for o, d in options]
        lines.append("")
    classes = key_classes(src, know.api_pages) if know else []
    if classes:
        lines += ["## Key API", "", "The documented classes this example uses most:", "",
                  "| Class | Library |", "|---|---|"]
        for cls, _url, _count in classes:
            page = know.api_pages[cls][1]
            library = next((LIBRARIES[lib] for lib in LIBRARIES if api_slug(lib) == page.parent.name), "")
            lines.append(f"| [`{cls}`]({_api_link(page, root)}) | {library} |")
        lines.append("")
    if refs:
        lines += ["## Used in the API reference", ""] + [f"- [{label}](../../{path})" for label, path in refs] + [""]
    lines += ["## Source", "", f"[`{EXAMPLES.as_posix()}/{name}`]({GITHUB_BLOB.replace('/blob/', '/tree/')}/"
              f"{EXAMPLES.as_posix()}/{name}) on GitHub.", ""]
    for file, code in files.items():
        lines += [f"````cpp title=\"{EXAMPLES.as_posix()}/{name}/{file}\" showLineNumbers",
                  "\n".join(code).rstrip("\n"), "````", ""]
    if related:
        lines += ["## Related examples", ""]
        for other in related:
            other_src = root / EXAMPLES / other
            lines.append(f"- [{other}](../{primary_library(other_src, root)}/{other}.mdx) — "
                         f"{mdx_text(example_info(other_src)[1])}.")
        lines.append("")
    return group, brief, "\n".join(lines)


def _api_link(page: Path, root: Path) -> str:
    return "../../" + page.relative_to(root / kn.DOCS).as_posix()


def _related(name: str, know: kn.Knowledge, root: Path) -> list[str]:
    """Examples sharing a topic first, then those sharing tasks and the primary library."""
    topics = set(kn.topics_of_example(know, name))
    me = know.examples.get(name, {})
    group = primary_library(root / EXAMPLES / name, root)
    scores = []
    for other, entry in know.examples.items():
        if other == name:
            continue
        shared = sum(1 for t in know.topics if t["id"] in topics and any(e["name"] == other for e in t["examples"]))
        score = 3 * shared + len(set(entry["tasks"]) & set(me.get("tasks", [])))
        score += 1 if primary_library(root / EXAMPLES / other, root) == group else 0
        if score >= 2:
            scores.append((-score, other))
    return [other for _score, other in sorted(scores)[:RELATED]]


def _knowledge_json(root: Path, know: kn.Knowledge, briefs: dict[str, str]) -> str:
    def example_url(name: str) -> str:
        return f"/docs/examples/{primary_library(root / EXAMPLES / name, root)}/{name}/"

    examples = []
    for name in sorted(briefs):
        libs = example_info(root / EXAMPLES / name)[0]
        entry = know.examples.get(name, {})
        examples.append({"name": name, "brief": briefs[name], "url": example_url(name),
                         "library": primary_library(root / EXAMPLES / name, root), "libraries": libs,
                         "tasks": entry.get("tasks", []), "modalities": entry.get("modalities", []),
                         "level": entry.get("level", ""), "topics": kn.topics_of_example(know, name)})
    topics = []
    for topic in know.topics:
        topic = dict(topic)
        topic["examples"] = [dict(e, brief=briefs[e["name"]], url=example_url(e["name"])) for e in topic["examples"]]
        topics.append(topic)
    data = {"$comment": "Generated by tools/doxy2mdx/generate_examples.py from doc/api_registry.json — do not edit.",
            "facets": know.facets, "libraries": LIBRARIES, "examples": examples, "topics": topics}
    return json.dumps(data, indent=2, ensure_ascii=False) + "\n"


def _redirect(target: str) -> str:
    return ("<!DOCTYPE html>\n<!-- Generated by tools/doxy2mdx/generate_examples.py — do not edit. -->\n"
            f'<html lang="en"><head><meta charset="utf-8"><title>Moved</title>'
            f'<meta http-equiv="refresh" content="0; url={target}"><link rel="canonical" href="{target}">'
            f'</head><body><a href="{target}">This example moved.</a></body></html>\n')


def generate(root: Path, out: Path, data: Path | None = None, redirects: Path | None = None) -> None:
    sources = sorted((root / EXAMPLES).glob("ex_*/"))
    know = kn.load(root, [s.name for s in sources]) if (root / kn.REGISTRY).is_file() else None
    refs = quoted_by(root / API)
    groups: dict[str, list[tuple[str, str]]] = {}
    briefs = {}
    for src in sources:
        related = _related(src.name, know, root) if know and know.examples else []
        group, brief, text = render(src.name, src, refs.get(src.name, []), know, related, root)
        (out / group).mkdir(parents=True, exist_ok=True)
        (out / group / f"{src.name}.mdx").write_text(text + "\n", encoding="utf-8")
        groups.setdefault(group, []).append((src.name, brief))
        briefs[src.name] = brief
        legacy = example_info(src)[0][0]
        if redirects is not None and legacy != group:
            stub = redirects / legacy / src.name / "index.html"
            stub.parent.mkdir(parents=True, exist_ok=True)
            stub.write_text(_redirect(f"../../{group}/{src.name}/"), encoding="utf-8")
    order = list(LIBRARIES)
    total = sum(map(len, groups.values()))
    index = ["---", "title: Examples", "sidebar_position: 0", "slug: /examples", "hide_table_of_contents: true",
             "---", "", "{/* Generated by tools/doxy2mdx/generate_examples.py from src/examples — do not edit. */}", "",
             "# Examples", "",
             f"{total} runnable programs in `src/examples`, built with the rest of MNE-CPP. Find one by what you want "
             "to do, the recordings you work with, or the library you are learning; each page explains the method, "
             "links the API it uses and shows the complete source.", ""]
    if know and know.examples:
        index += ["<ExamplesExplorer />", ""]
    for group in sorted(groups, key=order.index):
        (out / group / "_category_.json").write_text(
            json.dumps({"label": LIBRARIES[group], "position": order.index(group) + 1}, indent=2) + "\n",
            encoding="utf-8")
        if not (know and know.examples):
            index += [f"## {LIBRARIES[group]}", "", "| Example | What it shows |", "|---|---|"]
            index += [f"| [{name}](./{group}/{name}.mdx) | {mdx_text(brief)} |" for name, brief in groups[group]] + [""]
    (out / "index.mdx").write_text("\n".join(index), encoding="utf-8")
    if data is not None and know is not None:
        data.parent.mkdir(parents=True, exist_ok=True)
        data.write_text(_knowledge_json(root, know, briefs), encoding="utf-8")


def files(directory: Path) -> dict[str, bytes]:
    return {p.relative_to(directory).as_posix(): p.read_bytes() for p in directory.rglob("*") if p.is_file()}


def _clear(directory: Path) -> None:
    for stale in sorted(directory.rglob("*"), reverse=True) if directory.exists() else []:
        stale.unlink() if stale.is_file() else stale.rmdir()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="fail when the committed pages differ")
    args = parser.parse_args(argv)
    try:
        if not args.check:
            _clear(REPO_ROOT / OUT)
            _clear(REPO_ROOT / REDIRECTS)
            (REPO_ROOT / OUT).mkdir(parents=True, exist_ok=True)
            generate(REPO_ROOT, REPO_ROOT / OUT, REPO_ROOT / DATA, REPO_ROOT / REDIRECTS)
            print(f"wrote {OUT}, {DATA} and {REDIRECTS}")
            return 0
        with tempfile.TemporaryDirectory() as tmp:
            out, data, redirects = Path(tmp) / "out", Path(tmp) / "data" / DATA.name, Path(tmp) / "redirects"
            out.mkdir()
            generate(REPO_ROOT, out, data, redirects)
            bad = []
            for fresh_dir, committed_dir in ((out, REPO_ROOT / OUT), (redirects, REPO_ROOT / REDIRECTS)):
                fresh = files(fresh_dir) if fresh_dir.exists() else {}
                committed = files(committed_dir) if committed_dir.exists() else {}
                bad += [committed_dir / k for k in sorted(fresh.keys() | committed.keys())
                        if fresh.get(k) != committed.get(k)]
            committed_data = REPO_ROOT / DATA
            if not committed_data.is_file() or committed_data.read_bytes() != data.read_bytes():
                bad.append(committed_data)
    except ValueError as error:
        print(f"FAIL: doc/api_registry.json references that do not resolve:\n{error}")
        return 1
    for path in bad:
        print(f"FAIL: {path.relative_to(REPO_ROOT)} is stale; run python3 tools/doxy2mdx/generate_examples.py")
    if not bad:
        print(f"PASS: {OUT}, {DATA} and {REDIRECTS} match src/examples and doc/api_registry.json.")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
