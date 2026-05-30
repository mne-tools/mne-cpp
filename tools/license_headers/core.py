# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

"""Shared logic for SPDX license-header migration and validation.

This module is stdlib-only by design (Python >= 3.10) so that the
``tools.license_headers`` package can be invoked from a clean Python 3.11
checkout in CI without any pip installs. The two CLIs that drive it live in
:mod:`tools.license_headers.migrate` and :mod:`tools.license_headers.validate`.

The migration rewrites the legacy mne-cpp BSD-3 license block (the
``@section LICENSE`` paragraph in every Doxygen file header, ~22 lines) into a
3-line SPDX block at the very top of the file, followed by per-author lines
sorted alphabetically by surname. The Doxygen ``@file/@author/@since/@date/
@brief`` block is preserved verbatim; only the ``@section LICENSE`` paragraph
is excised.

Year-range rule: floor 2010 for files with pre-2026 git history, single year
2026 for files born this cycle. Author list is sourced from
``git log --follow --format='%an <%ae>'``, deduped by lower-cased email, with
bot/no-reply addresses filtered out.
"""

from __future__ import annotations

import re
import subprocess
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable, Literal

CURRENT_YEAR = 2026
EPOCH_FLOOR_YEAR = 2010

CommentStyle = Literal["cpp", "hash"]

# Files we do not have a single-comment style for (or which look like text)
# return ``None`` from :func:`comment_style_for`.
_CPP_EXTS = {".h", ".hpp", ".cpp", ".cc", ".cxx", ".c", ".inc"}
_HASH_EXTS = {".py", ".sh", ".bash", ".cmake", ".yml", ".yaml"}
_SLASH_EXTS = {".ts", ".tsx", ".js", ".jsx", ".css"}
# .bat uses "REM" which is uncommon; treat as hash-style ('#' is invalid in
# batch but the spec includes .bat under hash-style scripts -- we emit '::'
# comments for batch since that's the de-facto modern convention).
_BAT_EXTS = {".bat"}

_BOT_EMAIL_RE = re.compile(
    r"(noreply|no-reply|bot@|github-actions|copilot|claude|dependabot)",
    re.IGNORECASE,
)

# Regex matching the legacy "@section LICENSE" paragraph inside a Doxygen
# /** ... */ block. Captures from the @section line through the
# "POSSIBILITY OF SUCH DAMAGE." closing line plus any trailing blank comment
# lines, so the deletion leaves a single blank ``*`` separator above the
# following @brief / @author / etc. directive.
_LEGACY_LICENSE_RE = re.compile(
    r"^[ \t]*\*[ \t]*@section\s+LICENSE\b[\s\S]*?"
    r"POSSIBILITY OF SUCH DAMAGE\.[ \t]*\n"
    r"(?:[ \t]*\*[ \t]*\n)*",
    re.MULTILINE,
)

# Hash-style legacy header (rare in mne-cpp, but handled symmetrically).
_LEGACY_LICENSE_HASH_RE = re.compile(
    r"^[ \t]*#[ \t]*LICENSE[ \t]*\n"
    r"[\s\S]*?POSSIBILITY OF SUCH DAMAGE\.[ \t]*\n"
    r"(?:[ \t]*#[ \t]*\n)*",
    re.MULTILINE,
)

_SPDX_FIRST_LINE_RE = re.compile(
    r"^(?://|#|::)\s*SPDX-License-Identifier:\s*BSD-3-Clause\s*$"
)
_SPDX_COPYRIGHT_RE = re.compile(
    # Accept both the new form `Copyright (c) <years>` and the legacy form
    # `Copyright (c) <years> MNE-CPP Authors` for backward compatibility,
    # so files migrated under v1 of the emitter don't churn on validation.
    r"^(?://|#|::)\s*Copyright \(c\)\s+(\d{4}(?:-\d{4})?)(?:\s+MNE-CPP Authors)?\s*$"
)
_SPDX_AUTHOR_RE = re.compile(
    r"^(?://|#|::)\s{2,}(?P<name>[^<]+?)\s+<(?P<email>[^>]+)>\s*$"
)

_GENERATED_MARKER_RE = re.compile(r"GENERATED|auto-generated", re.IGNORECASE)


# ---------------------------------------------------------------------------
# Dataclasses
# ---------------------------------------------------------------------------


@dataclass
class LegacyHeader:
    """Parsed view of the legacy ``@section LICENSE`` block inside a file."""

    authors: list[tuple[str, str]] = field(default_factory=list)
    since: str | None = None
    date: str | None = None
    brief: str | None = None
    legacy_text: str = ""
    header_line_range: tuple[int, int] = (0, 0)


# ---------------------------------------------------------------------------
# Comment style detection
# ---------------------------------------------------------------------------


def comment_style_for(path: Path) -> CommentStyle | None:
    """Return the comment style to use for *path*, or ``None`` if unsupported.

    Special-case ``CMakeLists.txt`` (no extension) and shebang-less .bat files.
    """
    name = path.name.lower()
    if name == "cmakelists.txt":
        return "hash"
    ext = path.suffix.lower()
    if ext in _CPP_EXTS or ext in _SLASH_EXTS:
        return "cpp"
    if ext in _HASH_EXTS:
        return "hash"
    if ext in _BAT_EXTS:
        # Treat .bat as hash-style; the migrator emits '::' lines via a tiny
        # post-process in :func:`build_spdx_block`.
        return "hash"
    return None


def _comment_prefix(style: CommentStyle, path: Path | None = None) -> str:
    if style == "cpp":
        return "//"
    if path is not None and path.suffix.lower() == ".bat":
        return "::"
    return "#"


# ---------------------------------------------------------------------------
# SPDX block construction & detection
# ---------------------------------------------------------------------------


def _surname_key(author: tuple[str, str]) -> tuple[str, str]:
    name = author[0].strip()
    parts = name.split()
    surname = parts[-1] if parts else name
    return (surname.lower(), name.lower())


def build_spdx_block(
    authors: Iterable[tuple[str, str]],
    comment_style: CommentStyle,
    *,
    year_range: str = f"{CURRENT_YEAR}",
    path: Path | None = None,
) -> str:
    """Render the canonical 3+N-line SPDX block (terminated by ``\\n``).

    Authors are emitted in **chronological order** (oldest contributor
    first, most recent last) so the list reads as a project timeline.
    The caller is expected to feed the stream from ``_git_authors`` /
    ``authors_for`` which is already ordered that way.
    """
    prefix = _comment_prefix(comment_style, path)
    ordered_authors = _unique_authors(authors)
    lines = [
        f"{prefix} SPDX-License-Identifier: BSD-3-Clause",
        f"{prefix} Copyright (c) {year_range} MNE-CPP Authors",
    ]
    for name, email in ordered_authors:
        lines.append(f"{prefix}   {name} <{email}>")
    return "\n".join(lines) + "\n"


def _person_key(name: str) -> str:
    """Normalize a display name into a person-identity key.

    Collapses case, whitespace, punctuation, and short handle variants so that
    ``Lorenz Esch``, ``lorenz.esch``, and ``LorenzE`` map to the same person.
    The rule: lowercase, strip non-alphanumerics, then keep the longest
    contiguous alpha run -- this folds handles like ``juangpc`` and
    ``LorenzE`` onto the same key as their full name as long as the surname
    (``gpc``/``esch``) is the shared longest token. When that fails, we fall
    back to the raw lowercased name; the migration script's --strict mode can
    later flag any remaining duplicates for a manual override file.
    """
    raw = re.sub(r"[^a-z0-9]+", " ", name.lower()).strip()
    if not raw:
        return ""
    return raw


# Manual person aliases for cases the heuristic can't collapse on its own.
# Maps a normalized name fragment to a canonical (display name, preferred
# email) tuple. Extend as needed; one entry per real person.
_PERSON_ALIASES: dict[str, tuple[str, str]] = {
    "christoph dinh":  ("Christoph Dinh",  "christoph.dinh@mne-cpp.org"),
    "chdinh":          ("Christoph Dinh",  "christoph.dinh@mne-cpp.org"),
    "lorenz esch":     ("Lorenz Esch",     "lorenz.esch@tu-ilmenau.de"),
    "lorenze":         ("Lorenz Esch",     "lorenz.esch@tu-ilmenau.de"),
    "lorenzesch":      ("Lorenz Esch",     "lorenz.esch@tu-ilmenau.de"),
    "juan gpc":        ("Juan GPC",        "jgarciaprieto@mgh.harvard.edu"),
    "juangpc":         ("Juan GPC",        "jgarciaprieto@mgh.harvard.edu"),
    "gabriel motta":   ("Gabriel Motta",   "gabrielbenmotta@gmail.com"),
    "gabriel b motta": ("Gabriel Motta",   "gabrielbenmotta@gmail.com"),
    "gabrielbmotta":   ("Gabriel Motta",   "gabrielbenmotta@gmail.com"),
    "gbmotta":         ("Gabriel Motta",   "gabrielbenmotta@gmail.com"),
    "matti hamalainen":("Matti Hamalainen","msh@nmr.mgh.harvard.edu"),
    "rdoerfel":        ("Ruben Doerfel",   "doerfelruben@aol.com"),
    "ruben doerfel":   ("Ruben Doerfel",   "doerfelruben@aol.com"),
    "christof pieloth":("Christof Pieloth","pieloth@labp.htwk-leipzig.de"),
    "cpieloth":        ("Christof Pieloth","pieloth@labp.htwk-leipzig.de"),
    "daniel strohmeier": ("Daniel Strohmeier", "daniel.strohmeier@gmail.com"),
    "danielstrohmeier":  ("Daniel Strohmeier", "daniel.strohmeier@gmail.com"),
    "joewalter":         ("Daniel Strohmeier", "daniel.strohmeier@gmail.com"),
}


def _canonical_person(name: str, email: str) -> tuple[str, str]:
    """Resolve *(name, email)* to a canonical author identity via aliases.

    Falls through to the raw inputs (cleaned up) when no alias matches.
    """
    key = _person_key(name)
    if key in _PERSON_ALIASES:
        return _PERSON_ALIASES[key]
    # also try matching by the email local-part (e.g. "chdinh" in chdinh@...)
    local = email.split("@", 1)[0].lower()
    local_clean = re.sub(r"[^a-z]+", "", local)
    if local_clean in _PERSON_ALIASES:
        return _PERSON_ALIASES[local_clean]
    return (name.strip(), email.strip())


def _unique_authors(
    authors: Iterable[tuple[str, str]],
) -> list[tuple[str, str]]:
    """Collapse the (name, email) stream to one entry per real person.

    Dedup strategy (in order):
      1. Drop bot/no-reply addresses.
      2. Resolve each entry through :data:`_PERSON_ALIASES` to a canonical
         identity (so ``chdinh@nmr.mgh.harvard.edu`` and
         ``christoph.dinh@tu-ilmenau.de`` both fold to one Christoph Dinh).
      3. Within a person, keep the FIRST canonical entry seen -- which, when
         the caller feeds ``git log --follow`` output in reverse-chronological
         order, means the most recent email survives.
    """
    seen: dict[str, tuple[str, str]] = {}
    for name, email in authors:
        if not email.strip() or _BOT_EMAIL_RE.search(email):
            continue
        canon_name, canon_email = _canonical_person(name, email)
        key = _person_key(canon_name) or canon_email.lower()
        if key not in seen:
            seen[key] = (canon_name, canon_email)
    return list(seen.values())


def detect_spdx_block(text: str) -> tuple[int, str, list[tuple[str, str]]] | None:
    """If *text* starts with a valid SPDX block, return ``(end_offset, year_range, authors)``.

    Two on-disk forms are recognised:

    1. Legacy v1 form -- a run of bare ``//``/``#``/``::`` line comments at
       the very top of the file::

           // SPDX-License-Identifier: BSD-3-Clause
           // Copyright (c) 2026
           //   Name <email>

    2. Unified v2 form -- the SPDX metadata is the leading section of a
       single ``/** ... */`` Doxygen block, so the file opens with one
       comment style instead of two::

           /**
            * SPDX-License-Identifier: BSD-3-Clause
            * Copyright (c) 2026
            *   Name <email>
            *
            * @file ...

    The end offset is the index of the first character after the block
    (including the trailing newline). Returns ``None`` if neither form
    matches.
    """
    lines = text.splitlines(keepends=True)
    if len(lines) < 2:
        return None

    # v2: unified Doxygen-block form, optionally wrapped by a `//===` divider
    # line (matching the section-divider style used throughout the C++ files).
    divider_prefix = 0
    if re.match(r"^//=+\s*$", lines[0]):
        divider_prefix = 1

    if (
        len(lines) >= divider_prefix + 3
        and lines[divider_prefix].rstrip("\n") == "/**"
    ):
        inner = [ln.rstrip("\n") for ln in lines[divider_prefix + 1 :]]
        # Allow an arbitrary `* ` prefix in the inner lines.
        def _strip(s: str) -> str:
            m = re.match(r"^\s*\*\s?(.*)$", s)
            return m.group(1) if m else s
        if inner and _strip(inner[0]).startswith("SPDX-License-Identifier: BSD-3-Clause"):
            cm = re.match(r"^Copyright \(c\)\s+(\d{4}(?:-\d{4})?)(?:\s+MNE-CPP Authors)?\s*$", _strip(inner[1]))
            if cm:
                year_range = cm.group(1)
                authors: list[tuple[str, str]] = []
                # Canonical v3 form: a single ``@author`` Doxygen tag
                # introduces a semicolon-separated list whose entries
                # span one or more lines; continuation lines have no
                # ``@<tag>`` prefix but contain ``Name <email>;``.
                in_author_block = False
                _entry_re = re.compile(
                    r"(?P<name>[A-ZÄÖÜ][^<;]*?)\s*<(?P<email>[^>]+@[^>]+)>"
                )
                for ln in inner[2:]:
                    if ln.strip() == "*/":
                        break
                    body_line = _strip(ln)
                    if body_line.startswith("@author"):
                        in_author_block = True
                        rest = body_line[len("@author"):]
                    elif body_line.startswith("@"):
                        in_author_block = False
                        continue
                    elif in_author_block:
                        rest = body_line
                    else:
                        continue
                    for em in _entry_re.finditer(rest):
                        authors.append(
                            (em.group("name").strip(),
                             em.group("email").strip())
                        )
                # Only accept the v3 block if we found at least one author;
                # otherwise treat as a regular Doxygen file header.
                if authors:
                    # Advance past the rest of the Doxygen block (up to and
                    # including ` */`) and an optional trailing `//===` divider
                    # so re-running the migrator is idempotent.
                    close_idx = 2
                    while close_idx < len(inner) and inner[close_idx].strip() != "*/":
                        close_idx += 1
                    if close_idx >= len(inner):
                        return None
                    end_line_in_lines = divider_prefix + 1 + close_idx + 1
                    if (
                        end_line_in_lines < len(lines)
                        and re.match(r"^//=+\s*$", lines[end_line_in_lines])
                    ):
                        end_line_in_lines += 1
                    end_offset = sum(len(lines[i]) for i in range(end_line_in_lines))
                    return end_offset, year_range, authors

    # v1: legacy bare line-comment form.
    if not _SPDX_FIRST_LINE_RE.match(lines[0].rstrip("\n")):
        return None
    m = _SPDX_COPYRIGHT_RE.match(lines[1].rstrip("\n"))
    if not m:
        return None
    year_range = m.group(1)
    authors = []
    idx = 2
    while idx < len(lines):
        a = _SPDX_AUTHOR_RE.match(lines[idx].rstrip("\n"))
        if not a:
            break
        authors.append((a.group("name").strip(), a.group("email").strip()))
        idx += 1
    end_offset = sum(len(line) for line in lines[:idx])
    return end_offset, year_range, authors


# ---------------------------------------------------------------------------
# Legacy header parsing
# ---------------------------------------------------------------------------


_AUTHOR_FIELD_RE = re.compile(
    r"^\s*\*\s*@author\s+(?P<body>.+?)(?:\n(?=\s*\*\s*@)|\Z)",
    re.MULTILINE | re.DOTALL,
)
_AUTHOR_ENTRY_RE = re.compile(
    r"(?P<name>[^<;\n]+?)\s*<(?P<email>[^>\s]+)>"
)
_SINCE_RE = re.compile(r"^\s*\*\s*@since\s+(?P<v>\S+)", re.MULTILINE)
_DATE_RE = re.compile(r"^\s*\*\s*@date\s+(?P<d>.+?)\s*$", re.MULTILINE)
_BRIEF_RE = re.compile(r"^\s*\*\s*@brief\s+(?P<b>.+?)\s*$", re.MULTILINE)


def parse_legacy_header(text: str) -> LegacyHeader | None:
    """Best-effort parse of the legacy mne-cpp file header.

    Returns ``None`` if no Doxygen ``/** ... */`` file block is found in the
    first 200 lines. The line range returned is the (start, end) line numbers
    (1-based, inclusive) of the legacy LICENSE paragraph itself, not the full
    Doxygen comment.
    """
    head = "".join(text.splitlines(keepends=True)[:200])
    doxy_match = re.search(r"/\*\*[\s\S]*?\*/", head)
    if not doxy_match:
        return None
    block = doxy_match.group(0)

    header = LegacyHeader()
    license_m = _LEGACY_LICENSE_RE.search(block)
    if license_m:
        # Translate offsets within the doxy block to 1-based line numbers
        # within the source file.
        prefix_text = text[: doxy_match.start() + license_m.start()]
        start_line = prefix_text.count("\n") + 1
        end_line = start_line + license_m.group(0).count("\n") - 1
        header.legacy_text = license_m.group(0)
        header.header_line_range = (start_line, end_line)

    author_m = _AUTHOR_FIELD_RE.search(block)
    if author_m:
        body = author_m.group("body")
        for entry in _AUTHOR_ENTRY_RE.finditer(body):
            header.authors.append(
                (entry.group("name").strip().rstrip(";,"), entry.group("email").strip())
            )
    s = _SINCE_RE.search(block)
    if s:
        header.since = s.group("v").strip()
    d = _DATE_RE.search(block)
    if d:
        header.date = d.group("d").strip()
    b = _BRIEF_RE.search(block)
    if b:
        header.brief = b.group("b").strip()
    return header


# ---------------------------------------------------------------------------
# Git interaction
# ---------------------------------------------------------------------------


# ---------------------------------------------------------------------------
# Explicit project-rename map
#
# git's ``--follow`` rename detection is content-similarity based and
# produces extensive false positives on mne-cpp: brand-new files (e.g.
# the ``mna/`` or ``mri/`` libraries created in 2026) get attributed to
# 2012-era contributors merely because their boilerplate Doxygen headers
# look similar to the headers of unrelated 2012 files. We therefore do
# *not* use ``--follow``; instead we maintain a small hand-curated map
# of the project's real historical paths so that a current file like
# ``src/libraries/connectivity/connectivity_global.h`` also sees the
# history of ``src/libraries/conn/conn_global.h`` (the Mar-Jun 2026
# interlude) and (further back)
# ``libraries/connectivity/connectivity_global.h``.
#
# Format: a list of ``(prefix_old, prefix_new, basename_substitutions)``
# entries describing each path rewrite that ever happened in the repo,
# in any order. ``basename_substitutions`` is an iterable of
# ``(old_substring, new_substring)`` tuples applied to the part of the
# path *after* the prefix; pass an empty tuple if only the directory
# changed.
# ---------------------------------------------------------------------------

_PROJECT_RENAMES: tuple[tuple[str, str, tuple[tuple[str, str], ...]], ...] = (
    # Oct 2022, commit 41c19654d4: pure tree move libraries/ -> src/libraries/
    ("src/libraries/", "libraries/", ()),
    # Sep 2017: pure tree move MNE/ -> libraries/ ("Changed folder name
    # MNE to libraries"). Cascades with the rule above so today's
    # src/libraries/<lib>/<file> also picks up the MNE/<lib>/<file>
    # history from 2012-2017.
    ("libraries/", "MNE/", ()),
    # Oct 2012: per-library ``include/`` subfolders were flattened
    # ("flattened structure for easy access"). Only fiff and mne had
    # this layer; record each so MNE/fiff/<file> also resolves to
    # MNE/fiff/include/<file> and similarly for mne.
    ("MNE/fiff/", "MNE/fiff/include/", ()),
    ("MNE/mne/", "MNE/mne/include/", ()),
    # Jun 2026: conn/ -> connectivity/ plus the ``conn_global.{h,cpp}`` ->
    # ``connectivity_global.{h,cpp}`` basename rename, restoring the
    # original name to match mne-python's ``mne_connectivity``. Applies to
    # the current src/libraries/connectivity/ layout (and cascades to the
    # pre-Oct-2022 libraries/ layout and the Mar 2026 conn/ interlude).
    (
        "src/libraries/connectivity/",
        "src/libraries/conn/",
        (("conn_global.", "connectivity_global."),),
    ),
    (
        "libraries/connectivity/",
        "libraries/conn/",
        (("conn_global.", "connectivity_global."),),
    ),
    # Mar 2026, commit b3dfbd1a5f: connectivity/ -> conn/ plus the
    # ``connectivity_global.{h,cpp}`` -> ``conn_global.{h,cpp}`` basename
    # rename. Applies to the (now historical) src/libraries/conn/ layout
    # and (via cascade) to the pre-Oct-2022 layout.
    (
        "src/libraries/conn/",
        "src/libraries/connectivity/",
        (("connectivity_global.", "conn_global."),),
    ),
    (
        "libraries/conn/",
        "libraries/connectivity/",
        (("connectivity_global.", "conn_global."),),
    ),
)


def _historical_paths(path: Path) -> list[Path]:
    """Return *path* plus all of its known pre-rename historical paths.

    The current path is always first; historical paths follow in arbitrary
    order. The resulting list is fed to ``git log`` so we can union the
    commit histories across renames *without* relying on git's fragile
    content-similarity rename detection.
    """
    # Work in repo-relative posix form because the rename rules below are
    # all expressed that way.
    try:
        rel = path.relative_to(_repo_root()).as_posix()
    except ValueError:
        rel = path.as_posix()
    candidates: list[str] = [rel]
    queue: list[str] = [rel]
    seen: set[str] = {rel}
    while queue:
        cur = queue.pop()
        for prefix_new, prefix_old, subs in _PROJECT_RENAMES:
            # Skip rules whose ``prefix_old`` is itself a refinement of
            # ``prefix_new`` (e.g. MNE/fiff/ -> MNE/fiff/include/) once we
            # are already living under ``prefix_old``; otherwise the BFS
            # would keep inserting another ``include/`` segment forever.
            if prefix_old.startswith(prefix_new) and cur.startswith(prefix_old):
                continue
            if cur.startswith(prefix_new):
                tail = cur[len(prefix_new):]
                for old, new in subs:
                    tail = tail.replace(new, old)
                older = prefix_old + tail
                if older not in seen:
                    seen.add(older)
                    candidates.append(older)
                    queue.append(older)
    root = _repo_root()
    return [root / c for c in candidates]


def _repo_root() -> Path:
    global _REPO_ROOT_CACHE
    if _REPO_ROOT_CACHE is None:
        try:
            out = subprocess.check_output(
                ["git", "rev-parse", "--show-toplevel"],
                stderr=subprocess.DEVNULL,
                text=True,
            ).strip()
            _REPO_ROOT_CACHE = Path(out)
        except (subprocess.CalledProcessError, FileNotFoundError):
            _REPO_ROOT_CACHE = Path.cwd()
    return _REPO_ROOT_CACHE


_REPO_ROOT_CACHE: Path | None = None


def _git_authors(path: Path) -> list[tuple[str, str]]:
    """Return chronological author list for *path*, including pre-rename history.

    Renames are followed via the explicit :data:`_PROJECT_RENAMES` map
    (see comment block above) instead of ``git log --follow``, which is
    content-similarity based and produces false attributions to unrelated
    2012-era contributors for brand-new 2026 files.
    """
    rows: list[tuple[int, str, str]] = []
    seen: set[tuple[int, str]] = set()
    for hist in _historical_paths(path):
        try:
            out = subprocess.check_output(
                [
                    "git", "log",
                    "--format=%at%x09%an%x09%ae",
                    "--", str(hist),
                ],
                stderr=subprocess.DEVNULL,
                text=True,
            )
        except (subprocess.CalledProcessError, FileNotFoundError):
            continue
        for line in out.splitlines():
            parts = line.split("\t")
            if len(parts) != 3:
                continue
            ts_str, name, email = parts
            try:
                ts = int(ts_str)
            except ValueError:
                continue
            key = (ts, email.lower())
            if key in seen:
                continue
            seen.add(key)
            rows.append((ts, name.strip(), email.strip()))
    rows.sort(key=lambda r: r[0])
    return [(n, e) for _, n, e in rows]


def _git_earliest_year(path: Path) -> int | None:
    """Return the year of the earliest commit touching *path* or any
    of its known pre-rename historical paths."""
    earliest: int | None = None
    for hist in _historical_paths(path):
        try:
            out = subprocess.check_output(
                [
                    "git", "log",
                    "--format=%ad", "--date=format:%Y",
                    "--", str(hist),
                ],
                stderr=subprocess.DEVNULL,
                text=True,
            )
        except (subprocess.CalledProcessError, FileNotFoundError):
            continue
        for y in out.split():
            if y.isdigit():
                yi = int(y)
                if earliest is None or yi < earliest:
                    earliest = yi
    return earliest


_MONTH_NAMES = (
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December",
)


def _git_creation_month_year(path: Path) -> str | None:
    """Return ``"Month YYYY"`` of the earliest commit touching *path* or
    any of its known pre-rename historical paths.
    """
    earliest: tuple[int, int] | None = None
    for hist in _historical_paths(path):
        try:
            out = subprocess.check_output(
                [
                    "git", "log",
                    "--format=%ad", "--date=format:%Y-%m",
                    "--", str(hist),
                ],
                stderr=subprocess.DEVNULL,
                text=True,
            )
        except (subprocess.CalledProcessError, FileNotFoundError):
            continue
        for entry in out.splitlines():
            m = re.match(r"^(\d{4})-(\d{2})$", entry.strip())
            if not m:
                continue
            year = int(m.group(1))
            month = int(m.group(2))
            if not (1 <= month <= 12):
                continue
            cand = (year, month)
            if earliest is None or cand < earliest:
                earliest = cand
    if earliest is None:
        return None
    year, month = earliest
    return f"{_MONTH_NAMES[month - 1]} {year}"


def _year_range_for(path: Path) -> str:
    earliest = _git_earliest_year(path)
    if earliest is None or earliest >= CURRENT_YEAR:
        return f"{CURRENT_YEAR}"
    floored = max(earliest, EPOCH_FLOOR_YEAR)
    if floored >= CURRENT_YEAR:
        return f"{CURRENT_YEAR}"
    return f"{floored}-{CURRENT_YEAR}"


# ---------------------------------------------------------------------------
# Git version-tag lookup (for @since derivation)
# ---------------------------------------------------------------------------


_VERSION_TAG_RE = re.compile(r"^v?(\d+\.\d+\.\d+)$")
_VERSION_TAGS: list[tuple[str, str]] | None = None  # sorted [(YYYY-MM-DD, X.Y.Z)]


def _load_version_tags() -> list[tuple[str, str]]:
    """Return cached list of ``(YYYY-MM-DD, version)`` for every
    ``vX.Y.Z`` / ``X.Y.Z`` git tag, sorted by tag creation date."""
    global _VERSION_TAGS
    if _VERSION_TAGS is not None:
        return _VERSION_TAGS
    out = ""
    try:
        out = subprocess.check_output(
            ["git", "tag", "--sort=creatordate",
             "--format=%(creatordate:short) %(refname:short)"],
            stderr=subprocess.DEVNULL, text=True,
        )
    except (subprocess.CalledProcessError, FileNotFoundError):
        _VERSION_TAGS = []
        return _VERSION_TAGS
    parsed: list[tuple[str, str]] = []
    for line in out.splitlines():
        try:
            date_str, tag = line.split(None, 1)
        except ValueError:
            continue
        m = _VERSION_TAG_RE.match(tag.strip())
        if not m:
            continue
        parsed.append((date_str, m.group(1)))
    _VERSION_TAGS = parsed
    return _VERSION_TAGS


def _git_creation_iso(path: Path) -> str | None:
    """Return ``YYYY-MM-DD`` of the earliest commit touching *path* or
    any of its historical paths."""
    earliest: str | None = None
    for hist in _historical_paths(path):
        try:
            out = subprocess.check_output(
                ["git", "log", "--format=%ad",
                 "--date=format:%Y-%m-%d", "--", str(hist)],
                stderr=subprocess.DEVNULL, text=True,
            )
        except (subprocess.CalledProcessError, FileNotFoundError):
            continue
        for entry in out.splitlines():
            s = entry.strip()
            if len(s) == 10 and s[4] == "-" and s[7] == "-":
                if earliest is None or s < earliest:
                    earliest = s
    return earliest


def _first_version_for(path: Path, fallback: str = "0.1.0") -> str:
    """Return the closest version tag whose creation date is at or
    after *path*'s first commit. Files older than every tagged release
    pick the earliest tag (e.g. ``0.1.0``); files newer than every tag
    pick the most recent tag. Returns *fallback* when the repository
    has no version tags at all."""
    tags = _load_version_tags()
    if not tags:
        return fallback
    iso = _git_creation_iso(path)
    if iso is None:
        return tags[-1][1]
    for tag_date, version in tags:
        if tag_date >= iso:
            return version
    # File post-dates every release tag -- attribute to the latest tag.
    return tags[-1][1]


def authors_for(
    path: Path, legacy: LegacyHeader | None = None
) -> list[tuple[str, str]]:
    """Combined author list: git history first, then legacy header fallback."""
    authors = list(_git_authors(path))
    if legacy:
        authors.extend(legacy.authors)
    if not authors:
        # Best fallback for brand-new files staged but never committed.
        authors = [("Christoph Dinh", "christoph.dinh@mne-cpp.org")]
    return _unique_authors(authors)


# ---------------------------------------------------------------------------
# Generated-file detection
# ---------------------------------------------------------------------------


def is_generated(text: str) -> bool:
    head = "\n".join(text.splitlines()[:10])
    return bool(_GENERATED_MARKER_RE.search(head))


# ---------------------------------------------------------------------------
# Validation
# ---------------------------------------------------------------------------


def validate_file(path: Path, *, strict: bool = False) -> list[str]:
    """Return a list of human-readable errors for *path* (empty == OK)."""
    style = comment_style_for(path)
    if style is None:
        return []
    try:
        text = path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        return [f"{path}: cannot read ({exc})"]
    if not text.strip():
        return []
    if is_generated(text):
        return []

    detected = detect_spdx_block(text)
    if detected is None:
        return [f"{path}: missing SPDX header block"]
    end, year_range, authors = detected
    if not authors:
        return [f"{path}: SPDX block has no author lines"]
    # Enforce the canonical v3 format: exactly one ``@author`` Doxygen
    # tag, with the rest of the contributors listed as semicolon-
    # separated continuation lines aligned under the first name.
    block = text[:end]
    author_tag_lines = [
        ln for ln in block.splitlines() if re.match(r"^\s*\*\s*@author\b", ln)
    ]
    if len(author_tag_lines) != 1:
        return [
            f"{path}: SPDX header must use exactly one ``@author`` tag "
            f"(found {len(author_tag_lines)})"
        ]
    if len(authors) > 1:
        # Validate continuation prefix and ``;`` separator.
        cont_re = re.compile(r"^ \*           [A-ZÄÖÜ][^<;]*<[^>]+@[^>]+>;?\s*$")
        for ln in block.splitlines():
            if re.match(r"^\s*\*\s+\S.*<[^>]+@[^>]+>;?\s*$", ln) and not (
                re.match(r"^\s*\*\s*@author\b", ln) or cont_re.match(ln)
            ):
                return [
                    f"{path}: author continuation line must match "
                    f"`` *           Name <email>[;]`` (got: {ln!r})"
                ]
    if strict:
        expected = _unique_authors(authors_for(path))
        expected_emails = [e.lower() for _, e in expected]
        actual_emails = [e.lower() for _, e in authors]
        if expected_emails != actual_emails:
            return [
                f"{path}: SPDX author list out of sync with git log "
                f"(expected {expected_emails}, got {actual_emails})"
            ]
        expected_year = _year_range_for(path)
        if year_range != expected_year:
            return [
                f"{path}: SPDX year range {year_range!r} != expected {expected_year!r}"
            ]
    return []


# ---------------------------------------------------------------------------
# Migration
# ---------------------------------------------------------------------------


def _strip_legacy_license_paragraph(text: str, style: CommentStyle) -> str:
    if style == "cpp":
        return _LEGACY_LICENSE_RE.sub("", text, count=1)
    return _LEGACY_LICENSE_HASH_RE.sub("", text, count=1)


def render_migrated(path: Path, text: str) -> str | None:
    """Return the migrated file content, or ``None`` if no change is needed."""
    style = comment_style_for(path)
    if style is None or is_generated(text) or not text.strip():
        return None

    if detect_spdx_block(text) is not None:
        # Already migrated; preserve the file verbatim (idempotent no-op).
        return None

    legacy = parse_legacy_header(text)
    body = _strip_legacy_license_paragraph(text, style)
    authors = authors_for(path, legacy)
    year_range = _year_range_for(path)
    spdx = build_spdx_block(
        authors, style, year_range=year_range, path=path
    )

    # Leave a single blank line between the SPDX block and the existing
    # content so the Doxygen divider (or shebang) starts on its own line.
    if not body.startswith("\n"):
        spdx = spdx + "\n"
    return spdx + body


def migrate_file(path: Path, *, dry_run: bool = False) -> bool:
    """Rewrite *path* in place. Returns True iff content changed."""
    try:
        original = path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError):
        return False
    new = render_migrated(path, original)
    if new is None or new == original:
        return False
    if not dry_run:
        path.write_text(new, encoding="utf-8")
    return True


def diff_for(path: Path) -> str:
    """Return a unified diff for the would-be migration of *path*."""
    import difflib

    try:
        original = path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError):
        return ""
    new = render_migrated(path, original)
    if new is None or new == original:
        return ""
    diff = difflib.unified_diff(
        original.splitlines(keepends=True),
        new.splitlines(keepends=True),
        fromfile=f"a/{path}",
        tofile=f"b/{path}",
    )
    return "".join(diff)


# ---------------------------------------------------------------------------
# File-set walking
# ---------------------------------------------------------------------------

INCLUDE_GLOBS: tuple[str, ...] = (
    "src/**/*.h",
    "src/**/*.hpp",
    "src/**/*.cpp",
    "src/**/*.cc",
    "src/**/*.cxx",
    "src/**/*.c",
    "src/**/*.inc",
    "src/**/*.py",
    "tools/**/*.py",
    "scripts/**/*.py",
    "scripts/**/*.sh",
    "scripts/**/*.bat",
    "scripts/**/*.cmake",
    "**/CMakeLists.txt",
    ".github/workflows/*.yml",
    ".github/workflows/*.yaml",
    "doc/website/src/**/*.ts",
    "doc/website/src/**/*.tsx",
    "doc/website/src/**/*.js",
    "doc/website/src/**/*.jsx",
    "doc/website/src/**/*.css",
)

EXCLUDE_PATTERNS: tuple[str, ...] = (
    "src/external/*",
    "*/build/*",
    "*/build-*/*",
    "build/*",
    "build-*/*",
    "build_*/*",
    "*/.docusaurus/*",
    "*/node_modules/*",
    "*/moc_*",
    "*/ui_*",
    "*/qrc_*",
    "*/doxygen-awesome*",
    "*/katex*",
    "doc/website/.docusaurus/*",
    "doc/website/node_modules/*",
    "doc/website/build/*",
)


def _is_excluded(rel: Path) -> bool:
    import fnmatch

    s = rel.as_posix()
    for pat in EXCLUDE_PATTERNS:
        if fnmatch.fnmatch(s, pat):
            return True
    return False


_PRUNE_DIRS = {
    "build",
    "node_modules",
    ".docusaurus",
    ".git",
    ".github_cache",
    "external",  # under src/external/
    "Testing",
    "doxygen-awesome",
    "katex",
}

_INCLUDE_EXT_TO_STYLE = {
    ".h": "cpp", ".hpp": "cpp", ".cpp": "cpp", ".cc": "cpp",
    ".cxx": "cpp", ".c": "cpp", ".inc": "cpp",
    ".py": "hash", ".sh": "hash", ".bash": "hash",
    ".cmake": "hash", ".yml": "hash", ".yaml": "hash",
    ".ts": "cpp", ".tsx": "cpp", ".js": "cpp", ".jsx": "cpp", ".css": "cpp",
    ".bat": "hash",
}


def _is_included(rel: Path) -> bool:
    """Decide whether *rel* (a path relative to repo root) is in scope."""
    s = rel.as_posix()
    parts = rel.parts
    if not parts:
        return False
    top = parts[0]

    name = rel.name
    if name == "CMakeLists.txt":
        return True

    suffix = rel.suffix.lower()
    if suffix not in _INCLUDE_EXT_TO_STYLE:
        return False

    if top == "src":
        return True
    if top == "tools" and suffix == ".py":
        return True
    if top == "scripts" and suffix in {".py", ".sh", ".bash", ".cmake", ".bat"}:
        return True
    if top == ".github" and len(parts) >= 2 and parts[1] == "workflows" and suffix in {".yml", ".yaml"}:
        return True
    if top == "doc" and len(parts) >= 3 and parts[1] == "website" and parts[2] == "src":
        return True
    return False


def iter_in_scope(repo_root: Path) -> list[Path]:
    """Enumerate all in-scope files under *repo_root*, deduped and sorted.

    Uses :func:`os.walk` with directory pruning so vendored / generated
    subtrees (``build/``, ``node_modules/``, ``src/external/`` …) never get
    descended into.
    """
    import os

    out: list[Path] = []
    repo_str = str(repo_root)
    for dirpath, dirnames, filenames in os.walk(repo_str, topdown=True):
        # Prune excluded directories in place.
        rel_dir = Path(dirpath).relative_to(repo_root)
        rel_parts = rel_dir.parts
        # Inside doc/website we only want src/, prune build/.docusaurus/etc.
        prune: list[str] = []
        for d in list(dirnames):
            if d in _PRUNE_DIRS or d.startswith("build-") or d.startswith("build_"):
                prune.append(d)
                continue
            # Prune the giant src/external/skigen tree explicitly.
            if rel_parts == ("src",) and d == "external":
                prune.append(d)
                continue
            # Inside doc/website prune everything except src/.
            if rel_parts == ("doc", "website") and d != "src":
                prune.append(d)
                continue
        for d in prune:
            dirnames.remove(d)

        for fn in filenames:
            if fn.startswith("moc_") or fn.startswith("ui_") or fn.startswith("qrc_"):
                continue
            rel = rel_dir / fn
            if _is_included(rel):
                out.append(repo_root / rel)
    return sorted(out)


def resolve_paths(repo_root: Path, raw: list[str] | None) -> list[Path]:
    """Resolve user-supplied --paths against the in-scope filter.

    If *raw* is falsy, returns the full in-scope set. Each entry may be a file
    or a directory; directories expand to their in-scope contents. Explicit
    file arguments bypass the expensive global walk so the CLI stays snappy
    on single-file invocations.
    """
    if not raw:
        return iter_in_scope(repo_root)
    result: list[Path] = []
    need_full_walk = any(
        (repo_root / item).is_dir() if not Path(item).is_absolute() else Path(item).is_dir()
        for item in raw
    )
    in_scope = set(iter_in_scope(repo_root)) if need_full_walk else set()
    for item in raw:
        p = (repo_root / item).resolve() if not Path(item).is_absolute() else Path(item)
        if p.is_dir():
            result.extend(sorted(q for q in in_scope if p in q.parents))
        elif p.is_file():
            rel = p.relative_to(repo_root) if repo_root in p.parents else None
            if rel is not None and _is_excluded(rel):
                continue
            if comment_style_for(p) is not None:
                result.append(p)
        else:
            continue
    # Dedupe preserving order
    seen2: set[Path] = set()
    deduped: list[Path] = []
    for p in result:
        if p not in seen2:
            seen2.add(p)
            deduped.append(p)
    return deduped


def repo_root_from_here() -> Path:
    """Best-effort: walk up from this file until a ``.git`` directory is found."""
    here = Path(__file__).resolve()
    for parent in [here, *here.parents]:
        if (parent / ".git").exists():
            return parent
    return Path.cwd()


def now_year() -> int:
    return datetime.now(timezone.utc).year
