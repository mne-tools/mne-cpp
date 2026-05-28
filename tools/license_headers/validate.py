# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

"""CLI: validate that in-scope source files carry a canonical SPDX header.

Exits 0 when every in-scope file under the working tree has a valid 3-line
SPDX block at the very top. In ``--strict`` mode the validator additionally
checks that the author list matches ``git log --follow`` and that the
copyright year range matches the file's true age.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from .core import (
    iter_in_scope,
    repo_root_from_here,
    resolve_paths,
    validate_file,
)


def _parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(prog="tools.license_headers.validate")
    p.add_argument(
        "--strict",
        action="store_true",
        help="also fail if author list / year range drift from git history",
    )
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

    failures: list[str] = []
    for path in targets:
        failures.extend(validate_file(path, strict=args.strict))

    if failures:
        for f in failures:
            print(f, file=sys.stderr)
        print(
            f"\nvalidate: {len(failures)} file(s) failed "
            f"(scanned {len(targets)})",
            file=sys.stderr,
        )
        return 1
    print(f"validate: OK ({len(targets)} files)", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
