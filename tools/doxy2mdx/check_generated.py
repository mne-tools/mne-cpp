#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     October, 2026
#
#
# @brief    Fail when the committed API pages or sidebar differ from a fresh doxy2mdx run (T4.2).
"""Fail when the committed API pages or sidebar differ from what doxy2mdx generates.

doxy2mdx runs in strict mode into a temporary directory, so the working tree is
never touched, and the result is compared byte for byte with the committed
``doc/website/docs/api`` and ``doc/website/sidebars.api.generated.ts``.  Files
that the generator does not write (hand-written pages listed in KEEP) are
ignored; every other committed file must be produced, and every produced file
must be committed.

Usage
-----
    python3 tools/doxy2mdx/check_generated.py --xml-dir doc/xml_out/xml

Exit codes
----------
    0   committed output is current
    1   stale, missing or orphaned pages
    2   doxy2mdx failed
"""

from __future__ import annotations

import argparse
import filecmp
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
API_DIR = Path("doc/website/docs/api")
SIDEBAR = Path("doc/website/sidebars.api.generated.ts")
KEEP = {"index.mdx"}


def compare(generated: Path, committed: Path) -> dict[str, list[str]]:
    """Relative paths that differ, are missing from the commit, or are not generated."""
    gen = {p.relative_to(generated).as_posix() for p in generated.rglob("*") if p.is_file()}
    com = {p.relative_to(committed).as_posix() for p in committed.rglob("*") if p.is_file()} - KEEP
    return {
        "stale": sorted(rel for rel in gen & com if not filecmp.cmp(generated / rel, committed / rel, shallow=False)),
        "missing": sorted(gen - com),
        "orphaned": sorted(com - gen),
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--xml-dir", type=Path, default=Path("doc/xml_out/xml"))
    parser.add_argument("--repo-root", type=Path, default=REPO_ROOT)
    args = parser.parse_args(argv)
    root = args.repo_root.resolve()

    with tempfile.TemporaryDirectory() as tmp:
        out, sidebar = Path(tmp) / "api", Path(tmp) / SIDEBAR.name
        run = subprocess.run(
            [sys.executable, str(root / "tools/doxy2mdx/doxy2mdx.py"), "--xml-dir", str(args.xml_dir.resolve()),
             "--out-dir", str(out), "--registry", str(root / "doc/api_registry.json"), "--repo-root", str(root),
             "--generate-sidebars", "--sidebar-out", str(sidebar), "--strict"],
            capture_output=True, text=True, check=False)
        if run.returncode != 0:
            print(run.stdout + run.stderr)
            print(f"FAIL: doxy2mdx --strict exited {run.returncode}")
            return 2
        result = compare(out, root / API_DIR)
        if not filecmp.cmp(sidebar, root / SIDEBAR, shallow=False):
            result["stale"].append(f"../../{SIDEBAR.name}")

    total = sum(len(paths) for paths in result.values())
    for kind, paths in result.items():
        for rel in paths:
            print(f"{kind.upper()}: {API_DIR.as_posix()}/{rel}")
    if total:
        print(f"FAIL: {total} generated file(s) out of date. Regenerate with doc/build-api-docs.sh --no-site "
              "and commit doc/website/docs/api and doc/website/sidebars.api.generated.ts.")
        return 1
    print("PASS: committed API pages and sidebar match doxy2mdx output.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
