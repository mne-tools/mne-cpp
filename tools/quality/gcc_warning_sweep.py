#!/usr/bin/env python3
# =============================================================================================================
#
# @file     gcc_warning_sweep.py
# @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
# @since    2.4.0
# @date     July, 2026
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#
# @brief    Reproduce the Linux GCC warning gate locally, on every translation unit.
#
# =============================================================================================================

"""Compile every translation unit with GCC and report the diagnostics.

Release gate G5 requires a warning-free build on the whole compiler matrix, but
several GCC diagnostics have no Apple clang equivalent -- ``-Wdeprecated-copy``,
``-Wmaybe-uninitialized``, ``-Wformat-zero-length`` and
``-Wrange-loop-construct`` among them. A change that is clean on a macOS
workstation can therefore still fail the Ubuntu job, and each discovery costs a
full CI cycle.

This script closes that gap: it replays the project's own
``compile_commands.json`` through ``g++ -fsyntax-only`` and prints every
diagnostic in MNE-CPP's own sources, so the complete list is available in one
local pass.

Usage::

    python3 tools/quality/gcc_warning_sweep.py                      # sweep everything
    python3 tools/quality/gcc_warning_sweep.py --filter fiff        # only matching paths
    python3 tools/quality/gcc_warning_sweep.py --check              # exit 1 if anything is reported

Requires a configured build directory (for ``compile_commands.json``) and a GCC
that understands the project's C++ standard; ``brew install gcc`` provides one
on macOS.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]

# Qt ships as macOS frameworks, whose headers live in
# <Qt>/lib/QtCore.framework/Headers rather than <include>/QtCore. GCC does not
# understand -iframework, so `#include <QtCore/qglobal.h>` fails to resolve. A
# directory of symlinks named after each module reproduces the include layout
# GCC expects.
SHIM_DIR = Path(os.environ.get('TMPDIR', '/tmp')) / 'mnecpp-qt-include-shim'

# GCC notes an ARM/x86 calling-convention change introduced in GCC 10.1 for
# by-value aggregates. It reports no defect and is irrelevant to a build that
# uses a single compiler, but it is emitted for most Eigen and Qt templates.
BASE_FLAGS = ['-fsyntax-only', '-Wno-psabi']

# Flags accepted by clang but not by GCC.
CLANG_ONLY_PREFIXES = ('-Wno-variadic-macro-arguments-omitted',)

DIAGNOSTIC_RE = re.compile(r'^(?P<path>[^:\s][^:]*):(?P<line>\d+):(?P<col>\d+):\s+'
                           r'(?P<severity>warning|error):\s+(?P<message>.*)$')
WARNING_FLAG_RE = re.compile(r'\[(-W[a-z0-9=+-]+)\]')

# Diagnostics from these locations are not ours to fix.
EXCLUDED_PATH_PARTS = ('/external/', '.framework/', str(SHIM_DIR))
EXCLUDED_PATH_MARKERS = ('autogen',)


def find_compiler(explicit: str | None) -> str:
    """Return a usable g++, preferring an explicit choice."""
    if explicit:
        if not shutil.which(explicit):
            sys.exit(f'error: compiler {explicit!r} not found on PATH')
        return explicit
    for candidate in ('g++-15', 'g++-14', 'g++-13', 'g++'):
        path = shutil.which(candidate)
        if not path:
            continue
        # Apple aliases g++ to clang++; that defeats the purpose of the sweep.
        version = subprocess.run([candidate, '--version'], capture_output=True, text=True)
        if 'clang' in version.stdout.lower():
            continue
        return candidate
    sys.exit('error: no GNU g++ found. Install one (e.g. `brew install gcc`) '
             'or pass --compiler.')


def build_include_shim(qt_lib_dir: Path) -> None:
    SHIM_DIR.mkdir(parents=True, exist_ok=True)
    for framework in qt_lib_dir.glob('*.framework'):
        headers = framework / 'Headers'
        if not headers.is_dir():
            continue
        link = SHIM_DIR / framework.stem
        if link.is_symlink() or link.exists():
            continue
        link.symlink_to(headers)


def locate_qt_lib_dir(build_dir: Path) -> Path | None:
    cache = build_dir / 'CMakeCache.txt'
    if cache.exists():
        for line in cache.read_text(encoding='utf-8', errors='replace').splitlines():
            if line.startswith('Qt6_DIR:'):
                # <qt>/lib/cmake/Qt6 -> <qt>/lib
                qt6_dir = Path(line.split('=', 1)[1].strip())
                return qt6_dir.parent.parent
    fallback = REPO_ROOT / 'src/external/qt/dynamic/lib'
    return fallback if fallback.is_dir() else None


def rewrite_command(command: str, compiler: str) -> tuple[list[str], str] | None:
    """Turn a compile_commands.json entry into a GCC syntax-only invocation."""
    tokens = shlex.split(command)
    rewritten: list[str] = [compiler]
    source: str | None = None
    skip_next = False

    for token in tokens[1:]:
        if skip_next:
            skip_next = False
            continue
        if token == '-o':
            skip_next = True
            continue
        if token == '-c':
            continue
        if token == '-iframework':
            skip_next = True
            continue
        if token.startswith(CLANG_ONLY_PREFIXES):
            continue
        if token.endswith(('.cpp', '.cc', '.cxx')):
            source = token
            continue
        rewritten.append(token)

    if source is None:
        return None

    rewritten += ['-isystem', str(SHIM_DIR)]
    rewritten += BASE_FLAGS
    rewritten.append(source)
    return rewritten, source


def is_ours(path_text: str) -> Path | None:
    if any(part in path_text for part in EXCLUDED_PATH_PARTS):
        return None
    if any(marker in path_text for marker in EXCLUDED_PATH_MARKERS):
        return None
    try:
        resolved = Path(path_text).resolve()
        return resolved.relative_to(REPO_ROOT)
    except (ValueError, OSError):
        return None


def sweep_entry(entry: dict, compiler: str) -> list[str]:
    rewritten = rewrite_command(entry['command'], compiler)
    if rewritten is None:
        return []
    argv, _ = rewritten

    result = subprocess.run(argv, cwd=entry['directory'], capture_output=True, text=True)

    findings: list[str] = []
    for line in result.stderr.splitlines():
        match = DIAGNOSTIC_RE.match(line.strip())
        if not match:
            continue
        relative = is_ours(match.group('path'))
        if relative is None:
            continue
        flag_match = WARNING_FLAG_RE.search(match.group('message'))
        flag = flag_match.group(1) if flag_match else ''
        findings.append('\t'.join([
            f"{relative}:{match.group('line')}:{match.group('col')}",
            match.group('severity'),
            flag,
            match.group('message'),
        ]))
    return findings


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--build-dir', default='build/developer-dynamic',
                        help='configured build directory holding compile_commands.json')
    parser.add_argument('--compiler', default=None,
                        help='g++ to use (default: first GNU g++ found)')
    parser.add_argument('--filter', default=None,
                        help='only sweep sources whose path contains this substring')
    parser.add_argument('--jobs', type=int, default=os.cpu_count() or 8)
    parser.add_argument('--check', action='store_true',
                        help='exit non-zero when any diagnostic is reported')
    args = parser.parse_args()

    build_dir = (REPO_ROOT / args.build_dir) if not Path(args.build_dir).is_absolute() \
        else Path(args.build_dir)
    database = build_dir / 'compile_commands.json'
    if not database.exists():
        sys.exit(f'error: {database} not found. Configure the build first, e.g.\n'
                 f'  cmake -B {args.build_dir} -S . -DBUILD_TESTS=ON')

    compiler = find_compiler(args.compiler)

    qt_lib_dir = locate_qt_lib_dir(build_dir)
    if qt_lib_dir is not None:
        build_include_shim(qt_lib_dir)

    entries = []
    seen: set[str] = set()
    for entry in json.loads(database.read_text(encoding='utf-8')):
        if entry['file'] in seen:
            continue
        if args.filter and args.filter not in entry['file']:
            continue
        seen.add(entry['file'])
        entries.append(entry)

    print(f'sweeping {len(entries)} translation units with {compiler}', file=sys.stderr)

    findings: list[str] = []
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for index, result in enumerate(pool.map(lambda e: sweep_entry(e, compiler), entries), 1):
            findings.extend(result)
            if index % 100 == 0:
                print(f'  {index}/{len(entries)}', file=sys.stderr)

    unique = sorted(set(findings))
    for finding in unique:
        print(finding)

    if unique:
        print(f'\n{len(unique)} unique diagnostics', file=sys.stderr)
        counts: dict[str, int] = {}
        for finding in unique:
            flag = finding.split('\t')[2] or '(no flag)'
            counts[flag] = counts.get(flag, 0) + 1
        for flag, count in sorted(counts.items(), key=lambda kv: -kv[1]):
            print(f'  {count:4d}  {flag}', file=sys.stderr)
    else:
        print('no diagnostics', file=sys.stderr)

    return 1 if (args.check and unique) else 0


if __name__ == '__main__':
    sys.exit(main())
