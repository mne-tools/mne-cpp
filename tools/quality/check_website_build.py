#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     October, 2026
#
#
# @brief    Verify a built Docusaurus site has the base URL and version label of its channel (T4.3).
"""Check that a built site routes and labels the version it is deployed as.

The stable site is served from ``/`` and labelled ``vX.Y.Z`` (the project
version in ``src/CMakeLists.txt``); the dev site is served from ``/dev/`` and
labelled ``dev (latest)``.  A dev build with the stable base URL would overwrite
the stable root on deploy, so the check reads the generated HTML.

Usage
-----
    python3 tools/quality/check_website_build.py doc/website/build --channel dev
    python3 tools/quality/check_website_build.py doc/website/build --channel stable
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]


def project_version(repo_root: Path = REPO_ROOT) -> str:
    text = (repo_root / "src" / "CMakeLists.txt").read_text(encoding="utf-8")
    parts = [re.search(rf"set\(MNE_CPP_VERSION_{p}\s+(\d+)\)", text).group(1) for p in ("MAJOR", "MINOR", "PATCH")]
    return ".".join(parts)


def check(build: Path, channel: str, version: str) -> list[str]:
    base = "/dev/" if channel == "dev" else "/"
    label = "dev (latest)" if channel == "dev" else f"v{version}"
    problems = []
    index = build / "index.html"
    if not index.is_file():
        return [f"{index} is missing"]
    html = index.read_text(encoding="utf-8", errors="replace")
    assets = set(re.findall(r'(?:href|src)="(/[^"]*?/?)assets/', html))
    if assets != {base}:
        problems.append(f"assets are served from {sorted(assets) or 'nowhere'}, expected base URL '{base}'")
    if f">{label}<" not in html:
        problems.append(f"navbar version label '{label}' not found")
    if channel == "dev" and f">v{version}<" in html:
        problems.append(f"dev build is labelled as release v{version}")
    if f"v{version} (Stable)" not in html:
        problems.append(f"version menu does not offer 'v{version} (Stable)'")
    return problems


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("build", type=Path, help="Docusaurus build directory")
    parser.add_argument("--channel", choices=("stable", "dev"), required=True)
    args = parser.parse_args(argv)
    version = project_version()
    problems = check(args.build, args.channel, version)
    for problem in problems:
        print(f"FAIL: {problem}")
    if not problems:
        print(f"PASS: {args.channel} build serves from the right base URL and is labelled for v{version}.")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
