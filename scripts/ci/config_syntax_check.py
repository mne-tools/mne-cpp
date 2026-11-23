#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Syntax-check the code paths of other build configurations with the native compiler.

Code under ``#ifdef WASMBUILD`` is compiled by the WebAssembly CI job only, code under
``#ifdef STATICBUILD`` by the static release builds only. For each of these macros, this
script takes every translation unit of a configured native (dynamic) build that mentions
it, adds ``-D<MACRO> -fsyntax-only`` to its compile command and runs it, so such code is
checked in seconds instead of in a full Emscripten or static build.

Usage: config_syntax_check.py [--build build/developer-dynamic] [-j N]

@since 3.0.0
"""

from __future__ import annotations

import argparse
import json
import os
import shlex
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
SCOPES = ("src/apps", "src/tools", "src/libs")
MACROS = ("WASMBUILD", "STATICBUILD")


def guarded_units(macro: str) -> set[Path]:
    """The sources that hold code only compiled with @p macro defined."""
    out = subprocess.run(["git", "grep", "-l", "-w", macro, "--", *SCOPES],
                         cwd=REPO_ROOT, capture_output=True, text=True, check=False).stdout
    return {(REPO_ROOT / line).resolve() for line in out.split() if line.endswith(".cpp")}


def syntax_command(entry: dict, macro: str) -> list[str]:
    """The entry's compile command, checking syntax only with @p macro defined."""
    args = entry["arguments"] if "arguments" in entry else shlex.split(entry["command"])
    out: list[str] = []
    skip = False
    for arg in args:
        if skip:
            skip = False
        elif arg in ("-o", "-MF", "-MT", "-MQ"):
            skip = True
        elif arg not in ("-c", "-MD", "-MMD"):
            out.append(arg)
    return [out[0], f"-D{macro}", "-fsyntax-only", *out[1:]]


def check(job: tuple[dict, str]) -> tuple[str, str, int, str]:
    entry, macro = job
    proc = subprocess.run(syntax_command(entry, macro), cwd=entry["directory"], capture_output=True, text=True)
    return macro, entry["file"], proc.returncode, proc.stderr


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--build", default=str(REPO_ROOT / "build" / "developer-dynamic"))
    parser.add_argument("-j", type=int, default=os.cpu_count() or 4)
    args = parser.parse_args()
    database = Path(args.build) / "compile_commands.json"
    if not database.is_file():
        print(f"error: {database} not found; configure the build first", file=sys.stderr)
        return 2
    entries = json.loads(database.read_text())
    jobs = []
    for macro in MACROS:
        units = guarded_units(macro)
        jobs += [(e, macro) for e in entries if Path(e["directory"], e["file"]).resolve() in units]
    with ThreadPoolExecutor(args.j) as pool:
        results = list(pool.map(check, jobs))
    failed = [(macro, file, err) for macro, file, code, err in results if code]
    for macro, file, err in failed:
        print(f"== {os.path.relpath(file, REPO_ROOT)} (-D{macro})")
        print("\n".join(line for line in err.splitlines() if "error:" in line)[:4000])
    print(f"config syntax: {len(results)} units, {len(failed)} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
