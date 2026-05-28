"""One-shot migration to the v3 unified SPDX+Doxygen header form.

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

Target form (v3):

    //=============================================================================================================
    /**
     * SPDX-License-Identifier: BSD-3-Clause
     * Copyright (c) <years> MNE-CPP Authors
     *
     * @file     foo.h
     * @author   <Name> <<email>>
     * @author   <Name> <<email>>
     * @since    <closest-version-tag>          (e.g. 0.1.0)
     * @date     <Month YYYY>                   (first commit)
     * @brief    Foo bar baz.
     *
     * <preserved substantive body, if any>
     */

Differences from v2:
  * Authors moved from bare ``*   Name <email>`` lines under the
    Copyright header back into proper Doxygen ``@author`` tags.
  * ``@since`` is now the closest git version tag at or after the
    file's first commit (e.g. 0.1.0, 2.0.0) instead of the year.
  * ``@date`` reintroduced to record the month/year of the file's
    first appearance in git history.

Authors / year range / @since / @date are derived from **path-only**
``git log`` (no ``--follow``) so they reflect the people who actually
edited *this* file, not rename ancestry of older sibling files.

The migrator is idempotent: re-running on an already-v3 file is a no-op.
It only rewrites the metadata; agents are expected to polish the
``@brief`` one-liner and the substantive body afterwards.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys

from .core import (
    _first_version_for,
    _git_authors,
    _git_creation_month_year,
    _year_range_for,
)

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
    # _git_authors already runs `git log --follow --reverse`, so the
    # raw list is chronological. We dedup via _unique_authors inside
    # build_spdx_block; here we just return the raw stream.
    return _git_authors(fp)


def git_year_span(fp: pathlib.Path) -> str:
    return _year_range_for(fp)


def git_since_version(fp: pathlib.Path) -> str:
    """Closest version tag at or after the file's first commit."""
    return _first_version_for(fp)


def git_first_date(fp: pathlib.Path) -> str:
    """Month + year of the file's first commit (e.g. ``July 2012``)."""
    return _git_creation_month_year(fp) or "May 2026"


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
    from .core import _unique_authors
    authors = _unique_authors(git_authors(fp))
    if not authors:
        authors = [("Christoph Dinh", "christoph.dinh@mne-cpp.org")]
    year_range = git_year_span(fp)
    since = git_since_version(fp)
    date = git_first_date(fp)
    lines: list[str] = [
        DIV,
        "/**",
        " * SPDX-License-Identifier: BSD-3-Clause",
        f" * Copyright (c) {year_range} MNE-CPP Authors",
        " *",
        f" * @file     {fp.name}",
    ]
    for name, email in authors:
        lines.append(f" * @author   {name} <{email}>")
    lines.extend(
        [
            f" * @since    {since}",
            f" * @date     {date}",
            f" * @brief    {brief or 'TODO: describe this file in one substantive sentence.'}",
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
    from .core import _unique_authors
    txt = fp.read_text()
    if _CMAKE_SPDX_RE.search(txt.split("\n\n", 1)[0] if "\n\n" in txt else txt):
        return False
    authors = _unique_authors(git_authors(fp)) or [
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


def _parse_v2_existing(
    txt: str,
) -> tuple[int, str | None, list[str]] | None:
    """Parse an existing v2 unified Doxygen block.

    Returns ``(consumed_chars, brief, body_lines)`` where ``body_lines``
    are the substantive content lines (verbatim, including the leading
    `` * `` prefix) that follow the ``@brief`` line, with leading/trailing
    blank ``*`` separators trimmed. Returns ``None`` if the file does not
    begin with a v2 unified block.
    """
    m = re.match(
        r"(?P<head>(?://=+[ \t]*\n)?/\*\*\n)"
        r"(?P<inner>(?: \*[^\n]*\n)+)"
        r" \*/\n(?://=+[ \t]*\n)?",
        txt,
    )
    if not m:
        return None
    inner_lines = m.group("inner").splitlines()
    # Must begin with the SPDX line to be a v2 block.
    if not re.match(
        r" \* SPDX-License-Identifier: BSD-3-Clause\s*$", inner_lines[0]
    ):
        return None
    brief: str | None = None
    body: list[str] = []
    saw_brief = False
    for ln in inner_lines:
        s = ln.strip()
        # Discard the SPDX / Copyright / author / @file / @since / @date
        # metadata lines -- they are regenerated.
        if not saw_brief:
            if re.match(
                r"\*\s*(SPDX-License-Identifier|Copyright)\b", s
            ):
                continue
            if re.match(r"\*\s+\S.+<[^>]+>\s*$", s):
                # author line `` *   Name <email>``
                continue
            if re.match(r"\*\s*@(file|since|date|author|version)\b", s):
                continue
            bm = re.match(r"\*\s*@brief\s*(.*)$", s)
            if bm:
                brief = bm.group(1).strip() or None
                saw_brief = True
                continue
            # Blank `` *`` separator between metadata and brief -- skip.
            if s == "*":
                continue
            # Anything else before @brief is unexpected; bail out so we
            # don't munge a hand-edited header.
            return None
        else:
            body.append(ln)
    while body and body[0].strip() == "*":
        body.pop(0)
    while body and body[-1].strip() == "*":
        body.pop()
    return m.end(), brief, body


def rebuild_file(fp: pathlib.Path) -> bool:
    """Re-render an already-v2/v3 file's header with fresh git data.

    Preserves the ``@brief`` text and substantive body verbatim while
    regenerating the SPDX line, copyright year range, chronological
    author list, ``@file``, ``@since`` (closest version tag) and
    ``@date`` (first-commit month/year) fields.
    """
    if fp.name == "CMakeLists.txt" or fp.suffix == ".cmake":
        return _rebuild_cmake(fp)
    txt = fp.read_text()
    parsed = _parse_v2_existing(txt)
    if parsed is None:
        return False
    end, brief, body = parsed
    new = _render(fp, brief, body) + txt[end:]
    if new == txt:
        return False
    fp.write_text(new)
    return True


_CMAKE_V2_BLOCK_RE = re.compile(
    r"\A(# SPDX-License-Identifier: BSD-3-Clause\n"
    r"# Copyright \(c\) [^\n]+\n"
    r"(?:#   [^\n]+\n)+\n)"
)


def _rebuild_cmake(fp: pathlib.Path) -> bool:
    from .core import _unique_authors
    txt = fp.read_text()
    m = _CMAKE_V2_BLOCK_RE.match(txt)
    if not m:
        return False
    authors = _unique_authors(git_authors(fp)) or [
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
    new = block + txt[m.end():]
    if new == txt:
        return False
    fp.write_text(new)
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
    ap.add_argument(
        "--rebuild",
        action="store_true",
        help=(
            "Re-render existing v2 headers from fresh git data "
            "(refresh authors, year range, @since; drop @date)."
        ),
    )
    args = ap.parse_args(argv)
    op = rebuild_file if args.rebuild else migrate_file
    label = "rebuilt" if args.rebuild else "migrated"
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
            if op(fp):
                changed += 1
                print(f"{label}: {fp}")
    print(f"\nmigrate_v2: {changed} changed, {scanned - changed} unchanged")
    return 0


if __name__ == "__main__":
    sys.exit(main())
