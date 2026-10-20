#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Release invariants checked before anything is tagged or published (v2.4.0 T10.3).

Every file that carries the release version must agree with ``src/CMakeLists.txt``,
``CHANGELOG.md`` must have a section for it, the tag must not exist yet, and the
working tree must be clean and at the commit the release candidate verified::

    python3 tools/release/preflight.py [--commit SHA] [--remote origin]

Nothing is modified, so the check can be rerun at any point of a release.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]

# file -> pattern whose first group is the version that file carries
VERSIONED_FILES = {
    "README.md": r"badge/version-([0-9.]+)-",
    "CITATION.cff": r"^version:\s*([0-9.]+)\s*$",
    "doc/website/docusaurus.config.ts": r"const stableVersion = '([^']+)'",
    "doc/Doxyfile": r'^PROJECT_NUMBER\s*=\s*"([^"]+)"',
    "doc/Doxyfile.internal": r'^PROJECT_NUMBER\s*=\s*"([^"]+)"',
}


def project_version(root: Path) -> str:
    text = (root / "src" / "CMakeLists.txt").read_text(encoding="utf-8")
    return ".".join(re.search(rf"set\(MNE_CPP_VERSION_{part}\s+(\d+)\)", text).group(1)
                    for part in ("MAJOR", "MINOR", "PATCH"))


def file_problems(root: Path, version: str) -> list[str]:
    problems = []
    for name, pattern in VERSIONED_FILES.items():
        match = re.search(pattern, (root / name).read_text(encoding="utf-8"), re.M)
        found = match.group(1) if match else "nothing"
        if found != version:
            problems.append(f"{name} carries {found}, expected {version}")
    citation = (root / "CITATION.cff").read_text(encoding="utf-8")
    if not re.search(r'^date-released:\s*"\d{4}-\d{2}-\d{2}"\s*$', citation, re.M):
        problems.append("CITATION.cff has no date-released in YYYY-MM-DD form")
    changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")
    if not re.search(rf"^## \[{re.escape(version)}\]", changelog, re.M):
        problems.append(f"CHANGELOG.md has no '## [{version}]' section")
    return problems


def _git(root: Path, *args: str) -> str:
    return subprocess.run(["git", *args], cwd=root, capture_output=True, text=True, check=True).stdout


def git_problems(root: Path, version: str, commit: str | None, remote: str | None) -> list[str]:
    problems = []
    if _git(root, "status", "--porcelain", "--untracked-files=no").strip():
        problems.append("working tree has uncommitted changes")
    head = _git(root, "rev-parse", "HEAD").strip()
    if commit and not head.startswith(commit):
        problems.append(f"HEAD is {head[:12]}, not the verified commit {commit}")
    tag = f"v{version}"
    if _git(root, "tag", "--list", tag).strip():
        problems.append(f"tag {tag} already exists locally")
    if remote and _git(root, "ls-remote", "--tags", remote, f"refs/tags/{tag}").strip():
        problems.append(f"tag {tag} already exists on {remote}")
    return problems


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--commit", help="the commit the release candidate verified")
    parser.add_argument("--remote", help="also refuse a tag that exists on this remote")
    parser.add_argument("--repo-root", type=Path, default=REPO_ROOT)
    args = parser.parse_args(argv)
    version = project_version(args.repo_root)
    problems = file_problems(args.repo_root, version) + git_problems(args.repo_root, version, args.commit, args.remote)
    for problem in problems:
        print(f"FAIL: {problem}")
    if not problems:
        print(f"PASS: v{version} is consistent and not yet tagged.")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
