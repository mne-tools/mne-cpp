"""One-shot migration to the v2 unified SPDX+Doxygen header form.

Source form (legacy):

    //=============================================================================================================
    /**
     * @file     foo.h
     * @author   Some Person <s@p>; ...
     * @since    0.1.0
     * @date     July, 2012
     *
     * @section  LICENSE
     *
     * <BSD boilerplate>
     *
     * @brief    Foo bar baz.
     */

Target form (v2 unified, matches src/libraries/mri/):

    //=============================================================================================================
    /**
     * SPDX-License-Identifier: BSD-3-Clause
     * Copyright (c) <years> MNE-CPP Authors
     *   <Name> <<email>>
     *   ...
     *
     * @file foo.h
     * @since <year>
     * @date  <Month Year>
     * @brief Foo bar baz.
     *
     * <preserved substantive body, if any>
     */

Authors / year range / @since / @date are derived from **path-only**
``git log`` (no ``--follow``) so they reflect the people who actually
edited *this* file, not rename ancestry of older sibling files.

The migrator is idempotent: re-running on an already-v2 file is a no-op.
It only rewrites the metadata; agents are expected to polish the
``@brief`` one-liner and the substantive body afterwards.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys

from .core import _unique_authors, _year_range_for

MONTHS = [
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December",
]
DIV = "//" + "=" * 109


def _git(args: list[str], fp: pathlib.Path) -> list[str]:
    abs_fp = fp.resolve()
    try:
        out = subprocess.run(
            ["git", "log", *args, "--", str(abs_fp)],
            cwd=abs_fp.parent,
            capture_output=True, text=True, check=False,
        ).stdout
    except FileNotFoundError:
        return []
    return [ln for ln in out.splitlines() if ln]


def git_authors(fp: pathlib.Path) -> list[tuple[str, str]]:
    raw = [tuple(ln.split("\t", 1)) for ln in _git(["--format=%an\t%ae"], fp)]
    raw = [(n, e) for n, e in raw if n and e]
    return _unique_authors(raw)


def git_year_span(fp: pathlib.Path) -> str:
    # Match the validator's `_year_range_for` semantics: earliest path-only
    # commit year .. CURRENT_YEAR (always extending to the current year so
    # the SPDX range covers the active copyright window).
    return _year_range_for(fp)


def git_since(fp: pathlib.Path) -> str:
    added = _git(["--format=%aI", "--diff-filter=A"], fp)
    if added:
        return added[-1][:4]
    any_ = _git(["--format=%aI"], fp)
    return any_[-1][:4] if any_ else "2026"


def git_date(fp: pathlib.Path) -> str:
    out = _git(["-1", "--format=%aI"], fp)
    if not out:
        return "May 2026"
    s = out[0]
    return f"{MONTHS[int(s[5:7]) - 1]} {s[:4]}"


def _parse_existing(txt: str) -> tuple[int, str | None, list[str]]:
    """Return (consumed_chars, brief, body_lines) for the leading legacy block.

    Matches an optional ``//===`` divider, a single ``/** ... */`` block,
    and an optional trailing ``//===`` divider. Returns (0, None, []) when
    no such block is present.
    """
    m = re.match(
        r"(?://=+[ \t]*\n)?/\*\*\n((?: \*[^\n]*\n)+) \*/\n(?://=+[ \t]*\n)?",
        txt,
    )
    if not m:
        return 0, None, []
    inner = [ln.rstrip("\n") for ln in m.group(1).splitlines()]
    brief: str | None = None
    body: list[str] = []
    # State: None = preamble; 'body' = collecting; 'skip' = inside LICENSE
    mode: str | None = None
    for ln in inner:
        s = ln.strip()
        # Drop legacy metadata fields entirely.
        if re.match(
            r"\*\s*@(file|author|since|date|version|note|copyright)\b", s
        ):
            continue
        sm = re.match(r"\*\s*@section\s+(\S+)\s*(.*)$", s)
        if sm:
            name = sm.group(1).upper()
            tail = sm.group(2).strip()
            if name == "LICENSE":
                mode = "skip"
                continue
            mode = "body"
            if tail:
                body.append(" * " + tail)
            continue
        bm = re.match(r"\*\s*@brief\s*(.*)$", s)
        if bm:
            brief = bm.group(1).strip() or None
            mode = "body"
            continue
        if mode == "skip":
            continue
        if mode == "body":
            body.append(ln)
    # Trim blank ` *` lines from both ends.
    while body and body[0].strip() == "*":
        body.pop(0)
    while body and body[-1].strip() == "*":
        body.pop()
    return m.end(), brief, body


def _render(
    fp: pathlib.Path, brief: str | None, body: list[str]
) -> str:
    authors = git_authors(fp)
    if not authors:
        authors = [("Christoph Dinh", "christoph.dinh@mne-cpp.org")]
    year_range = git_year_span(fp)
    since = git_since(fp)
    date_s = git_date(fp)
    lines: list[str] = [
        DIV,
        "/**",
        " * SPDX-License-Identifier: BSD-3-Clause",
        f" * Copyright (c) {year_range} MNE-CPP Authors",
    ]
    for name, email in authors:
        lines.append(f" *   {name} <{email}>")
    lines.extend(
        [
            " *",
            f" * @file {fp.name}",
            f" * @since {since}",
            f" * @date  {date_s}",
            f" * @brief {brief or 'TODO: describe this file in one substantive sentence.'}",
        ]
    )
    if body:
        lines.append(" *")
        lines.extend(body)
    lines.append(" */")
    return "\n".join(lines) + "\n"


_V2_HEAD_RE = re.compile(
    r"^(?://=+[ \t]*\n)?/\*\*\n \* SPDX-License-Identifier:"
)
_CMAKE_SPDX_RE = re.compile(r"^# SPDX-License-Identifier:", re.MULTILINE)


def _migrate_cmake(fp: pathlib.Path) -> bool:
    txt = fp.read_text()
    if _CMAKE_SPDX_RE.search(txt.split("\n\n", 1)[0] if "\n\n" in txt else txt):
        return False
    authors = git_authors(fp) or [
        ("Christoph Dinh", "christoph.dinh@mne-cpp.org")
    ]
    year_range = git_year_span(fp)
    lines = [
        "# SPDX-License-Identifier: BSD-3-Clause",
        f"# Copyright (c) {year_range} MNE-CPP Authors",
    ]
    for name, email in authors:
        lines.append(f"#   {name} <{email}>")
    block = "\n".join(lines) + "\n\n"
    fp.write_text(block + txt)
    return True


def migrate_file(fp: pathlib.Path) -> bool:
    if fp.name == "CMakeLists.txt" or fp.suffix == ".cmake":
        return _migrate_cmake(fp)
    txt = fp.read_text()
    if _V2_HEAD_RE.match(txt):
        return False
    end, brief, body = _parse_existing(txt)
    if end == 0:
        return False
    new = _render(fp, brief, body) + txt[end:]
    if new == txt:
        return False
    fp.write_text(new)
    return True


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument(
        "paths",
        nargs="+",
        type=pathlib.Path,
        help="Files or directories to migrate (recursive for dirs).",
    )
    args = ap.parse_args(argv)
    changed = 0
    scanned = 0
    for root in args.paths:
        if root.is_file():
            files = [root]
        else:
            files = sorted(
                list(root.rglob("*.h"))
                + list(root.rglob("*.hpp"))
                + list(root.rglob("*.cpp"))
                + list(root.rglob("CMakeLists.txt"))
                + list(root.rglob("*.cmake"))
            )
        for fp in files:
            scanned += 1
            if migrate_file(fp):
                changed += 1
                print(f"migrated: {fp}")
    print(f"\nmigrate_v2: {changed} changed, {scanned - changed} unchanged")
    return 0


if __name__ == "__main__":
    sys.exit(main())
