#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     September, 2026
#
#
# @brief    Ratchet Doxygen warnings against a shrink-only allowlist (T4.1).
"""Fail on any Doxygen warning that is not in the allowlist.

Doxygen writes its warnings to ``doc/xml_out/doxygen-warnings.log``
(``WARN_LOGFILE`` in ``doc/Doxyfile``).  Each warning becomes a key of its
repository-relative file and its message, with continuation lines folded in and
line numbers removed, so that an unrelated edit that shifts lines does not
change the key.  ``tools/quality/doxygen_warning_allowlist.json`` records how
often each key may occur; the check fails when a key is new or occurs more often.

Usage
-----
    python3 tools/quality/check_doxygen_warnings.py doc/xml_out/doxygen-warnings.log
    python3 tools/quality/check_doxygen_warnings.py LOG --update   # only after fixing warnings

``--update`` rewrites the allowlist from the log and refuses to add a key or
raise a count, so the list can only shrink.  ``--init`` seeds a missing
allowlist and refuses to overwrite an existing one.

Exit codes
----------
    0   no warning outside the allowlist
    1   new warnings, or --update would grow the allowlist
    2   unreadable input
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
ALLOWLIST = Path(__file__).resolve().parent / "doxygen_warning_allowlist.json"
_WARNING = re.compile(r"^(?P<file>.*?):(?P<line>\d+): (?P<kind>warning|error): (?P<text>.*)$")
_LINE_REF = re.compile(r"\b(line|lines) \d+(-\d+)?\b")


def parse_log(text: str, repo_root: Path = REPO_ROOT) -> Counter:
    """Count warnings by ``<file>: <message>``, with paths made relative and line numbers removed."""
    root = repo_root.as_posix().rstrip("/") + "/"
    keys: Counter = Counter()
    current: list[str] | None = None
    for raw in text.splitlines():
        match = _WARNING.match(raw)
        if match:
            if current:
                keys[_key(*current, root)] += 1
            current = [match.group("file"), f"{match.group('kind')}: {match.group('text')}"]
        elif current and raw.strip():
            current[1] += " | " + raw.strip()
        elif raw.strip():
            keys[_key("<no file>", raw.strip(), root)] += 1
    if current:
        keys[_key(*current, root)] += 1
    return keys


def _key(file: str, message: str, root: str) -> str:
    file = file.replace("\\", "/").replace(root, "")
    message = _LINE_REF.sub(r"\1 N", message.replace(root, "").replace(root.replace("/", "\\"), ""))
    return f"{file}: {message}"


def load_allowlist(path: Path) -> Counter:
    data = json.loads(path.read_text(encoding="utf-8"))
    return Counter(data.get("warnings", {}))


def write_allowlist(path: Path, keys: Counter) -> None:
    data = {
        "$comment": "Doxygen warnings tolerated by tools/quality/check_doxygen_warnings.py (release gate G3, "
                    "v2.4.0 T4.1). Keys are '<file>: <message>' without line numbers; values are how often the "
                    "key may occur. The list may only shrink: fix a warning, then run the checker with --update.",
        "total": sum(keys.values()),
        "warnings": dict(sorted(keys.items())),
    }
    path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def excess(current: Counter, allowed: Counter) -> Counter:
    return Counter({key: count - allowed[key] for key, count in current.items() if count > allowed[key]})


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("log", type=Path, help="Doxygen WARN_LOGFILE output")
    parser.add_argument("--allowlist", type=Path, default=ALLOWLIST)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--update", action="store_true", help="shrink the allowlist to the current warnings")
    mode.add_argument("--init", action="store_true", help="create the allowlist when none exists")
    args = parser.parse_args(argv)

    if args.init:
        if args.allowlist.exists():
            print(f"FAIL: {args.allowlist} exists; use --update, which can only shrink it.")
            return 1
        try:
            current = parse_log(args.log.read_text(encoding="utf-8", errors="replace"))
        except OSError as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 2
        write_allowlist(args.allowlist, current)
        print(f"Allowlist created with {sum(current.values())} warnings.")
        return 0

    try:
        current = parse_log(args.log.read_text(encoding="utf-8", errors="replace"))
        allowed = load_allowlist(args.allowlist) if args.allowlist.is_file() else Counter()
    except (OSError, json.JSONDecodeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    new = excess(current, allowed)
    fixed = sum((allowed - current).values())
    print(f"Doxygen warnings : {sum(current.values())} (allowlist {sum(allowed.values())}, "
          f"{fixed} fixed since, {sum(new.values())} new)")
    for key, count in sorted(new.items())[:200]:
        print(f"NEW{f' x{count}' if count > 1 else ''}: {key}")
    if len(new) > 200:
        print(f"... and {len(new) - 200} more")

    if args.update:
        if new:
            print("FAIL: --update cannot add warnings or raise a count; fix the new warnings first.")
            return 1
        write_allowlist(args.allowlist, current)
        print(f"Allowlist shrunk to {sum(current.values())} warnings.")
        return 0
    if new:
        print("FAIL: fix the warnings above (see doc/xml_out/doxygen-warnings.log for line numbers).")
        return 1
    if fixed:
        print(f"Run with --update to lock in the {fixed} fixed warning(s).")
    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
