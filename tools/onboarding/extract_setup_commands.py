#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Extract the documented developer-onboarding commands (v2.4.0 T10.8a).

The build guide marks each code block of the supported setup path with an MDX
comment on the line before its fence::

    {/* onboarding: unix */}
    ```bash
    ./init.sh
    ```

The blocks of one platform, in document order, are the commands a contributor
runs on a fresh clone. Onboarding CI runs exactly these, so ``--check`` fails
when the docs and the committed ``setup_commands.json`` differ.

    python3 tools/onboarding/extract_setup_commands.py          # print
    python3 tools/onboarding/extract_setup_commands.py --write  # update the JSON
    python3 tools/onboarding/extract_setup_commands.py --check  # CI drift check
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
DOCS = ("doc/website/docs/development/buildguide-cmake.mdx",)
COMMANDS_JSON = Path(__file__).with_name("setup_commands.json")

_MARKER = re.compile(r"^\{/\*\s*onboarding:\s*([a-z-]+)\s*\*/\}\s*$")
_FENCE = re.compile(r"^```")


def extract(text: str, source: str) -> dict[str, list[str]]:
    """Return the marked commands per platform; raise ValueError on a marker without a code block."""
    lines = text.splitlines()
    commands: dict[str, list[str]] = {}
    i = 0
    while i < len(lines):
        marker = _MARKER.match(lines[i].strip())
        i += 1
        if not marker:
            continue
        if i >= len(lines) or not _FENCE.match(lines[i].strip()):
            raise ValueError(f"{source}:{i}: onboarding marker '{marker.group(1)}' is not followed by a code block")
        i += 1
        block = commands.setdefault(marker.group(1), [])
        while i < len(lines) and not _FENCE.match(lines[i].strip()):
            line = lines[i].strip()
            if line and not line.startswith(("#", "REM ", "::")):
                block.append(line)
            i += 1
        i += 1
    return commands


def extract_docs(root: Path) -> dict[str, list[str]]:
    """Merge the marked commands of all onboarding docs, in DOCS order."""
    merged: dict[str, list[str]] = {}
    for doc in DOCS:
        for platform, lines in extract((root / doc).read_text(encoding="utf-8"), doc).items():
            merged.setdefault(platform, []).extend(lines)
    return dict(sorted(merged.items()))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--write", action="store_true", help=f"update {COMMANDS_JSON.name}")
    group.add_argument("--check", action="store_true", help=f"fail if {COMMANDS_JSON.name} differs from the docs")
    args = parser.parse_args(argv)

    commands = extract_docs(REPO_ROOT)
    if not commands:
        print("ERROR: no onboarding markers found in " + ", ".join(DOCS), file=sys.stderr)
        return 1
    rendered = json.dumps(commands, indent=2) + "\n"
    if args.write:
        COMMANDS_JSON.write_text(rendered, encoding="utf-8")
        print(f"Wrote tools/onboarding/{COMMANDS_JSON.name}")
        return 0
    if args.check:
        committed = COMMANDS_JSON.read_text(encoding="utf-8") if COMMANDS_JSON.exists() else ""
        if committed != rendered:
            print(f"ERROR: the documented onboarding commands differ from tools/onboarding/{COMMANDS_JSON.name}; "
                  "update both together (python3 tools/onboarding/extract_setup_commands.py --write)", file=sys.stderr)
            return 1
        print("Onboarding commands match the docs.")
        return 0
    sys.stdout.write(rendered)
    return 0


if __name__ == "__main__":
    sys.exit(main())
