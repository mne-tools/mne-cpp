# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

"""CLI: migrate legacy mne-cpp license headers to the SPDX 3-line block.

Usage::

    python -m tools.license_headers.migrate [--dry-run] [--paths P ...]

When ``--paths`` is omitted, the full in-scope file set (see
``tools.license_headers.core.INCLUDE_GLOBS``) is processed. ``--dry-run``
prints unified diffs for files that *would* change without touching them.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from .core import (
    comment_style_for,
    diff_for,
    iter_in_scope,
    migrate_file,
    render_migrated,
    repo_root_from_here,
    resolve_paths,
)


def _parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(prog="tools.license_headers.migrate")
    p.add_argument("--dry-run", action="store_true", help="print diffs only")
    p.add_argument(
        "--paths",
        nargs="*",
        default=None,
        help="restrict to specific files / directories",
    )
    return p.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = _parse_args(argv)
    repo = repo_root_from_here()

    targets = resolve_paths(repo, args.paths) if args.paths else iter_in_scope(repo)

    scanned = 0
    would_change = 0
    unchanged = 0
    skipped = 0

    for path in targets:
        if comment_style_for(path) is None:
            skipped += 1
            continue
        scanned += 1
        try:
            original = path.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            skipped += 1
            continue
        new = render_migrated(path, original)
        if new is None or new == original:
            unchanged += 1
            continue
        would_change += 1
        if args.dry_run:
            sys.stdout.write(diff_for(path))
        else:
            migrate_file(path, dry_run=False)

    summary = (
        f"scanned={scanned} "
        f"{'would-change' if args.dry_run else 'changed'}={would_change} "
        f"unchanged={unchanged} skipped={skipped}"
    )
    print(summary, file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
