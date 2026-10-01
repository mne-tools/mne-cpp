#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     October, 2026
#
#
# @brief    Fail when in-scope C/C++ files are not formatted with the pinned clang-format (T6.2).
"""Check (or fix) the formatting of the C/C++ sources against ``.clang-format``.

The whole tree was reformatted once with the pinned clang-format, so every
in-scope file must stay formatted as a whole.  CI checks all files
(``--all``); locally the default is the files changed since the merge base
with ``origin/staging``, which is faster and gives the same verdict for a
branch that started from a formatted tree.

The in-scope set is the C/C++ sources under ``src/`` minus ``src/external``.
Local and CI runs use the same file filter and the same clang-format version.

Usage
-----
    pip install clang-format==21.1.8
    python3 tools/quality/check_format.py                  # changed files vs origin/staging
    python3 tools/quality/check_format.py --base HEAD~1    # files changed in one commit
    python3 tools/quality/check_format.py --all            # every in-scope file (CI)
    python3 tools/quality/check_format.py --fix            # reformat the selected files in place

Exit codes
----------
    0   selected files are formatted
    1   files need formatting (offending files and lines are listed)
    2   tool or git error
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
CLANG_FORMAT_VERSION = "21.1.8"
EXTENSIONS = ("c", "cc", "cpp", "cxx", "h", "hh", "hpp", "hxx")
INCLUDE_PREFIX = "src/"
EXCLUDE_PREFIXES = ("src/external/",)
BATCH = 200
MAX_REPORTED_LINES = 40


def in_scope(path: str) -> bool:
    return (path.startswith(INCLUDE_PREFIX) and not path.startswith(EXCLUDE_PREFIXES)
            and path.rsplit(".", 1)[-1] in EXTENSIONS)


def git(*args: str) -> str:
    return subprocess.run(["git", "-C", str(REPO_ROOT), *args], check=True, capture_output=True, text=True).stdout


def default_base() -> str:
    for ref in ("origin/staging", "staging"):
        try:
            return git("merge-base", "HEAD", ref).strip()
        except subprocess.CalledProcessError:
            continue
    return "HEAD"


def changed_files(base: str) -> list[str]:
    """In-scope files that differ from *base* in the working tree (added, copied, modified, renamed)."""
    names = git("diff", "--name-only", "--diff-filter=ACMR", base, "--").split()
    return sorted(name for name in names if in_scope(name))


def all_files() -> list[str]:
    """Every tracked in-scope file."""
    return sorted(name for name in git("ls-files", "--", INCLUDE_PREFIX).split() if in_scope(name))


def find_clang_format() -> str:
    path = shutil.which("clang-format")
    if path is None:
        raise FileNotFoundError(f"clang-format not found; install it with: pip install clang-format=={CLANG_FORMAT_VERSION}")
    found = subprocess.run([path, "--version"], check=True, capture_output=True, text=True).stdout
    if f"version {CLANG_FORMAT_VERSION}" not in found:
        raise RuntimeError(f"clang-format {CLANG_FORMAT_VERSION} is required, found: {found.strip()} "
                           f"(pip install clang-format=={CLANG_FORMAT_VERSION})")
    return path


def run_clang_format(binary: str, files: list[str], fix: bool) -> tuple[list[str], list[str]]:
    """Return (unformatted files, diagnostic lines); with *fix* the files are rewritten instead."""
    mode = ["-i"] if fix else ["--dry-run", "-Werror"]
    bad: list[str] = []
    report: list[str] = []
    for start in range(0, len(files), BATCH):
        run = subprocess.run([binary, *mode, *files[start:start + BATCH]], cwd=REPO_ROOT,
                             capture_output=True, text=True)
        for line in run.stderr.splitlines():
            if ": error: code should be clang-formatted" in line:
                name = line.split(":", 1)[0]
                if name not in bad:
                    bad.append(name)
                report.append(line.split(" [-W", 1)[0])
            elif "error:" in line:
                raise RuntimeError(line)
        if run.returncode != 0 and not bad:
            raise RuntimeError(run.stderr.strip() or f"clang-format exited with {run.returncode}")
    return bad, report


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    scope = parser.add_mutually_exclusive_group()
    scope.add_argument("--base", help="ref to diff against (default: merge base with origin/staging)")
    scope.add_argument("--all", action="store_true", help="select every in-scope file")
    parser.add_argument("--fix", action="store_true", help="reformat the selected files in place")
    parser.add_argument("--list", action="store_true", help="print the selected files and exit")
    args = parser.parse_args(argv)

    try:
        if args.all:
            label, scope_flag, files = "all in-scope files", "--all", all_files()
        else:
            base = args.base or default_base()
            label, scope_flag, files = f"changed since {base[:12]}", f"--base {base}", changed_files(base)
    except subprocess.CalledProcessError as exc:
        print(f"error: git failed: {exc.stderr.strip()}", file=sys.stderr)
        return 2
    if args.list:
        print("\n".join(files))
        return 0
    if not files:
        print(f"PASS: no in-scope C/C++ files ({label}).")
        return 0

    try:
        binary = find_clang_format()
        bad, report = run_clang_format(binary, files, args.fix)
    except (FileNotFoundError, RuntimeError, subprocess.CalledProcessError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    if args.fix:
        print(f"formatted {len(files)} file(s) ({label}).")
        return 0
    if not bad:
        print(f"PASS: {len(files)} file(s) formatted with clang-format {CLANG_FORMAT_VERSION} ({label}).")
        return 0
    print("\n".join(report[:MAX_REPORTED_LINES]))
    if len(report) > MAX_REPORTED_LINES:
        print(f"... {len(report) - MAX_REPORTED_LINES} more")
    print(f"FAIL: {len(bad)} of {len(files)} file(s) need formatting:")
    print("\n".join(f"  {name}" for name in bad))
    print(f"Fix with: python3 tools/quality/check_format.py --fix {scope_flag}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
