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
    """Render the canonical 3+N-line SPDX block (terminated by ``\\n``)."""
    prefix = _comment_prefix(comment_style, path)
    sorted_authors = sorted(_unique_authors(authors), key=_surname_key)
    lines = [
        f"{prefix} SPDX-License-Identifier: BSD-3-Clause",
        f"{prefix} Copyright (c) {year_range}",
    ]
    for name, email in sorted_authors:
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
    "gabrielbmotta":   ("Gabriel Motta",   "gabrielbenmotta@gmail.com"),
    "matti hamalainen":("Matti Hamalainen","msh@nmr.mgh.harvard.edu"),
    "rdoerfel":        ("Ruben Doerfel",   "doerfelruben@aol.com"),
    "ruben doerfel":   ("Ruben Doerfel",   "doerfelruben@aol.com"),
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
            cm = re.match(r"^Copyright \(c\)\s+(\d{4}(?:-\d{4})?)\s*$", _strip(inner[1]))
            if cm:
                year_range = cm.group(1)
                authors: list[tuple[str, str]] = []
                idx = 2
                while idx < len(inner):
                    stripped = _strip(inner[idx])
                    am = re.match(r"^\s{2,}(?P<name>[^<]+?)\s+<(?P<email>[^>]+)>\s*$", stripped)
                    if not am:
                        break
                    authors.append((am.group("name").strip(), am.group("email").strip()))
                    idx += 1
                # Only accept the v2 block if we found at least one author;
                # otherwise treat as a regular Doxygen file header.
                if authors:
                    # Advance past the rest of the Doxygen block (up to and
                    # including ` */`) and an optional trailing `//===` divider
                    # so re-running the migrator is idempotent.
                    close_idx = idx
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


def _git_authors(path: Path) -> list[tuple[str, str]]:
    # Path-only history (no --follow): we want the people who actually edited
    # *this* file, not the rename-ancestry authors of older sibling files.
    try:
        out = subprocess.check_output(
            ["git", "log", "--format=%an <%ae>", "--", str(path)],
            cwd=path.parent if path.parent.exists() else None,
            stderr=subprocess.DEVNULL,
            text=True,
        )
    except (subprocess.CalledProcessError, FileNotFoundError):
        return []
    result: list[tuple[str, str]] = []
    for line in out.splitlines():
        m = re.match(r"^(?P<name>.+?)\s+<(?P<email>[^>]+)>\s*$", line)
        if m:
            result.append((m.group("name").strip(), m.group("email").strip()))
    return result


def _git_earliest_year(path: Path) -> int | None:
    # Path-only history (no --follow): the year range tracks edits to this
    # path, not the content lineage of files it inherited from.
    try:
        out = subprocess.check_output(
            [
                "git",
                "log",
                "--format=%ad",
                "--date=format:%Y",
                "--",
                str(path),
            ],
            cwd=path.parent if path.parent.exists() else None,
            stderr=subprocess.DEVNULL,
            text=True,
        )
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None
    years = [int(y) for y in out.split() if y.isdigit()]
    return min(years) if years else None


def _year_range_for(path: Path) -> str:
    earliest = _git_earliest_year(path)
    if earliest is None or earliest >= CURRENT_YEAR:
        return f"{CURRENT_YEAR}"
    floored = max(earliest, EPOCH_FLOOR_YEAR)
    if floored >= CURRENT_YEAR:
        return f"{CURRENT_YEAR}"
    return f"{floored}-{CURRENT_YEAR}"


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
    _end, year_range, authors = detected
    if not authors:
        return [f"{path}: SPDX block has no author lines"]
    if strict:
        expected = _unique_authors(authors_for(path))
        expected_set = {e.lower() for _, e in expected}
        actual_set = {e.lower() for _, e in authors}
        missing = expected_set - actual_set
        extra = actual_set - expected_set
        if missing or extra:
            return [
                f"{path}: SPDX author list out of sync with git log "
                f"(missing={sorted(missing)}, extra={sorted(extra)})"
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
