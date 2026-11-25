#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# @since    3.0.0
# @date     November, 2026
#
#
# @brief    Keeps the Options tables of the tool manual pages in step with the tools' --help.
"""Keep the Options tables of ``doc/website/docs/manual/tools-*.mdx`` in step with the tools.

For every page whose title names a built command-line tool, the script runs ``<tool> --help``, reads the
options it lists and rewrites the page's Options table: one row per option in the order of ``--help``,
existing rows for ``--help``, ``--version`` and positional arguments kept at the end. A row the page already has for an option keeps its description
(pages often explain more than the help text); options the tool does not have are dropped, options the
page lacks get the help text. Options documented in a later table of the section stay there.

Usage
-----
    python3 scripts/doc/sync_tool_options.py --bin out/Release/bin           # rewrite the pages
    python3 scripts/doc/sync_tool_options.py --bin out/Release/bin --check   # list differences, exit 1 if any

The tools must be built; pages of tools that are not found, whose help lists no options or that have no
``## Options`` section are skipped.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
PAGES = REPO / "doc" / "website" / "docs" / "manual"
SKIPPED = {"help", "version"}


@dataclass
class Option:
    names: list[str]  # long and short names without dashes, in help order
    value: str        # value placeholder, e.g. "<file>", or ""
    description: str

    def cell(self) -> str:
        spelled = ", ".join(("--" if len(n) > 1 else "-") + n for n in self.names)
        return f"`{spelled}{' ' + self.value if self.value else ''}`"


# "  -i, --in <file>    Description", also with bare value names ("--id id    Description")
OPTION_LINE = re.compile(r"^  (-{1,2}[^\s,]+(?:, -{1,2}[^\s,]+)*)((?: [^\s-]\S*)*)(?:\s{2,}(.*))?$")


def parse_help(text: str) -> list[Option]:
    """Options of a mne::CommandLineParser help text."""
    options: list[Option] = []
    in_options = False
    for line in text.splitlines():
        if line.strip() == "Options:":
            in_options = True
            continue
        if not in_options:
            continue
        if line.strip() == "" or line.startswith("Arguments:"):
            if line.startswith("Arguments:"):
                break
            continue
        match = OPTION_LINE.match(line)
        if match:
            names = [n.lstrip("-") for n in match.group(1).split(", ")]
            value = " ".join(v if v.startswith("<") else f"<{v}>" for v in match.group(2).split())
            options.append(Option(names, value, (match.group(3) or "").strip()))
        elif options and not line.startswith("  -"):
            # Wrapped description
            options[-1].description = (options[-1].description + " " + line.strip()).strip()
    return options


def escape(text: str) -> str:
    """Description text safe in an MDX table cell."""
    text = text.replace("|", "\\|")
    # Angle brackets outside code would start JSX elements
    return re.sub(r"(<[^>\s]+>)", r"`\1`", text) if "`" not in text else text.replace("<", "&lt;")


ROW = re.compile(r"^\|\s*`([^`]+)`\s*\|\s*(.*?)\s*\|\s*$")


def documented(table: list[str]) -> dict[str, str]:
    """Description of each documented option, by every name of the row."""
    rows: dict[str, str] = {}
    for line in table:
        match = ROW.match(line)
        if not match:
            continue
        for token in re.findall(r"-{1,2}([A-Za-z0-9_][\w\-]*)", match.group(1)):
            rows.setdefault(token, match.group(2))
    return rows


def sync(page: Path, binary_dir: Path, check: bool) -> list[str]:
    text = page.read_text(encoding="utf-8")
    title = re.search(r"^title:\s*(\S+)\s*$", text, re.M)
    if not title:
        return []
    tool = binary_dir / title.group(1)
    if not tool.exists():
        tool = tool.with_suffix(".exe")
        if not tool.exists():
            return []
    result = subprocess.run([str(tool), "--help"], capture_output=True, text=True, timeout=60)
    every = parse_help(result.stdout + result.stderr)
    options = [o for o in every if not SKIPPED & set(o.names)]
    if not options:
        return []

    lines = text.split("\n")
    try:
        start = next(i for i, l in enumerate(lines) if l.strip() == "## Options")
    except StopIteration:
        # Pages with their own layout (e.g. tools-anonymize.mdx) are kept by hand
        return []
    end = next((i for i in range(start + 1, len(lines)) if lines[i].startswith("## ")), len(lines))
    # The first table of the section; later tables (value lists and the like) stay
    first = next((i for i in range(start + 1, end) if lines[i].startswith("|")), None)
    table_rows = []
    if first is not None:
        i = first
        while i < end and lines[i].startswith("|"):
            table_rows.append(i)
            i += 1
    old = documented([lines[i] for i in table_rows])
    # Pages may document groups of options in later tables of the section; those stay where they are
    elsewhere = documented([lines[i] for i in range(start + 1, end) if i not in table_rows])
    differences = []
    help_names = {n for o in every for n in o.names}
    for name in sorted((set(old) | set(elsewhere)) - help_names - SKIPPED - {"h", "v"}):
        differences.append(f"{page.name}: --{name} is documented but {title.group(1)} has no such option")
    for option in options:
        if not any(n in old or n in elsewhere for n in option.names):
            differences.append(f"{page.name}: {option.cell()} is missing")
    options = [o for o in options if any(n in old for n in o.names) or not any(n in elsewhere for n in o.names)]

    # The page's header row, rows for --help/--version and rows of positional arguments stay as they are
    rows = [lines[i] for i in table_rows[:2]] if len(table_rows) >= 2 else ["| Option | Description |", "|---|---|"]
    for option in options:
        kept = next((old[n] for n in option.names if n in old), None)
        rows.append(f"| {option.cell()} | {kept if kept else escape(option.description)} |")
    for i in table_rows[2:]:
        match = ROW.match(lines[i])
        tokens = set(re.findall(r"-{1,2}([A-Za-z0-9_][\w\-]*)", match.group(1))) if match else set()
        if match and tokens <= SKIPPED | {"h", "v"}:
            rows.append(lines[i])
    if table_rows:
        new_lines = lines[: table_rows[0]] + rows + lines[table_rows[-1] + 1 :]
    else:
        new_lines = lines[: start + 1] + [""] + rows + lines[start + 1 :]
    new_text = "\n".join(new_lines)
    if new_text != text:
        if not differences:
            differences.append(f"{page.name}: option spelling or order differs from --help")
        if not check:
            page.write_text(new_text, encoding="utf-8")
    return differences


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--bin", type=Path, default=REPO / "out" / "Release" / "bin", help="directory of the built tools")
    parser.add_argument("--check", action="store_true", help="only report differences")
    args = parser.parse_args()
    differences = []
    for page in sorted(PAGES.glob("tools-*.mdx")):
        differences += sync(page, args.bin, args.check)
    for line in differences:
        print(line)
    print(f"{'FAIL' if differences and args.check else 'OK'}: {len(differences)} difference(s)")
    return 1 if differences and args.check else 0


if __name__ == "__main__":
    sys.exit(main())
