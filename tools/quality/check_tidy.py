#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     October, 2026
#
#
# @brief    clang-tidy ratchet: findings per file and check may only shrink (T6.3).
"""Run the pinned clang-tidy and fail when any file gains findings of a check.

The checks are configured in ``.clang-tidy``.  ``tools/quality/tidy_baseline.json``
records the accepted findings as ``{file: {check: count}}``; a run fails when a
file has more findings of a check than recorded, so existing debt does not block
work but nothing new gets in.  Modules listed in ``"clean_modules"`` must stay at
zero.  ``--update`` rewrites the baseline after debt was paid off and refuses to
raise any count.

Suppressions need a reason: ``// NOLINT(check-name): why`` or
``// NOLINTNEXTLINE(check-name): why``.  A bare ``NOLINT`` or one without a
check name or reason fails the run.

Results are cached per translation unit, keyed by the clang-tidy version,
``.clang-tidy``, the compile command and the content of every file the unit
includes, so a rerun on the same tree gives the same answer without
re-analysing it.  Each unit gets a fixed timeout; a timeout is reported as a
failure, never as "no findings".

Usage
-----
    pip install clang-tidy==22.1.8
    python3 tools/quality/check_tidy.py -p build             # translation units changed since origin/staging
    python3 tools/quality/check_tidy.py -p build --all       # every in-scope translation unit
    python3 tools/quality/check_tidy.py -p build --all --update   # shrink the baseline

Exit codes
----------
    0   no new findings
    1   new findings, invalid suppressions or timeouts
    2   tool or setup error
"""

from __future__ import annotations

import argparse
import concurrent.futures
import hashlib
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
from collections import Counter
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
CLANG_TIDY_VERSION = "22.1.8"
BASELINE = REPO_ROOT / "tools" / "quality" / "tidy_baseline.json"
CONFIG = REPO_ROOT / ".clang-tidy"
SOURCE_EXTENSIONS = (".cpp", ".cc", ".cxx", ".c")
HEADER_EXTENSIONS = (".h", ".hh", ".hpp", ".hxx")
INCLUDE_PREFIX = "src/"
EXCLUDE_PREFIXES = ("src/external/",)
DEFAULT_TIMEOUT = 300
CACHE_DIR = Path(os.environ.get("MNE_TIDY_CACHE", REPO_ROOT / ".cache" / "clang-tidy"))

FINDING_RE = re.compile(r"^(?P<file>[^\n]+?):(?P<line>\d+):(?P<col>\d+): (?:warning|error): .*? \[(?P<checks>[\w.,-]+)\]$",
                        re.MULTILINE)
NOLINT_RE = re.compile(r"//\s*NOLINT(?:NEXTLINE|BEGIN|END)?\b(?P<rest>.*)$")
NOLINT_OK_RE = re.compile(r"^\((?P<checks>[\w.*,\s-]+)\)\s*:\s*\S")


def in_scope(path: str) -> bool:
    return path.startswith(INCLUDE_PREFIX) and not path.startswith(EXCLUDE_PREFIXES) and "_autogen/" not in path


def git(*args: str) -> str:
    return subprocess.run(["git", "-C", str(REPO_ROOT), *args], check=True, capture_output=True, text=True).stdout


def default_base() -> str:
    for ref in ("origin/staging", "staging"):
        try:
            return git("merge-base", "HEAD", ref).strip()
        except subprocess.CalledProcessError:
            continue
    return "HEAD"


def find_clang_tidy() -> str:
    path = shutil.which("clang-tidy")
    if path is None:
        raise FileNotFoundError(f"clang-tidy not found; install it with: pip install clang-tidy=={CLANG_TIDY_VERSION}")
    found = subprocess.run([path, "--version"], check=True, capture_output=True, text=True).stdout
    if f"version {CLANG_TIDY_VERSION}" not in found:
        raise RuntimeError(f"clang-tidy {CLANG_TIDY_VERSION} is required, found: {found.strip()} "
                           f"(pip install clang-tidy=={CLANG_TIDY_VERSION})")
    return path


def relative(path: str | Path) -> str:
    try:
        return Path(path).resolve().relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return Path(path).as_posix()


def load_database(build_dir: Path) -> dict[str, dict]:
    """In-scope translation units of the compile database, keyed by repository-relative path."""
    database = build_dir / "compile_commands.json"
    if not database.is_file():
        raise FileNotFoundError(f"{database} not found; configure with CMAKE_EXPORT_COMPILE_COMMANDS=ON")
    units = {}
    for entry in json.loads(database.read_text(encoding="utf-8")):
        path = relative(Path(entry["directory"], entry["file"]))
        if in_scope(path) and path.endswith(SOURCE_EXTENSIONS):
            units[path] = entry
    return units


def changed_units(base: str, units: dict[str, dict]) -> list[str]:
    """Changed sources, plus the sources whose basename matches a changed header."""
    changed = [name for name in git("diff", "--name-only", "--diff-filter=ACMR", base, "--").split() if in_scope(name)]
    selected = {name for name in changed if name in units}
    headers = {Path(name).stem for name in changed if name.endswith(HEADER_EXTENSIONS)}
    selected |= {unit for unit in units if Path(unit).stem in headers}
    return sorted(selected)


def suppression_problems(paths: list[str]) -> list[str]:
    problems = []
    for path in paths:
        file = REPO_ROOT / path
        if not file.is_file():
            continue
        for number, line in enumerate(file.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
            match = NOLINT_RE.search(line)
            if match and not NOLINT_OK_RE.match(match.group("rest").strip()):
                problems.append(f"{path}:{number}: suppression needs a check name and a reason: "
                                f"// NOLINT(check-name): why")
    return problems


def unit_dependencies(entry: dict) -> list[str]:
    """Files the unit includes, from the compiler's dependency output, for the cache key."""
    arguments = entry.get("arguments") or shlex.split(entry["command"])
    arguments = [a for a in arguments if a not in ("-c",)]
    if "-o" in arguments:
        index = arguments.index("-o")
        del arguments[index:index + 2]
    run = subprocess.run(arguments + ["-M", "-MF", "-"], cwd=entry["directory"], capture_output=True, text=True)
    if run.returncode != 0:
        return []
    return sorted({token for token in run.stdout.replace("\\\n", " ").split()[1:]})


def cache_key(tool_version: str, entry: dict, extra: list[str]) -> str:
    digest = hashlib.sha256()
    digest.update(tool_version.encode())
    digest.update(CONFIG.read_bytes())
    digest.update(json.dumps([entry.get("arguments") or entry["command"], extra]).encode())
    for dependency in unit_dependencies(entry) or [str(Path(entry["directory"], entry["file"]))]:
        path = Path(entry["directory"], dependency)
        digest.update(dependency.encode())
        digest.update(path.read_bytes() if path.is_file() else b"-")
    return digest.hexdigest()


Finding = tuple[str, int, int, str]


def run_unit(binary: str, build_dir: Path, path: str, entry: dict, extra: list[str], timeout: int,
             tool_version: str, use_cache: bool) -> tuple[str, list[Finding], str | None]:
    """Return (path, [(file, line, column, check)], error)."""
    key = cache_key(tool_version, entry, extra) if use_cache else ""
    cached = CACHE_DIR / f"{key}.json"
    if use_cache and cached.is_file():
        data = json.loads(cached.read_text(encoding="utf-8"))
        return path, [tuple(item) for item in data], None
    command = [binary, "-p", str(build_dir), "--quiet", *[f"--extra-arg={arg}" for arg in extra], str(REPO_ROOT / path)]
    try:
        run = subprocess.run(command, cwd=REPO_ROOT, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return path, [], f"timed out after {timeout} s"
    findings = []
    for match in FINDING_RE.finditer(run.stdout):
        file = relative(match.group("file"))
        if not in_scope(file):
            continue
        for check in match.group("checks").split(","):
            if check == "clang-diagnostic-error":
                return path, [], f"does not compile under clang-tidy: {match.group(0)[:300]}"
            findings.append((file, int(match.group("line")), int(match.group("col")), check))
    if use_cache:
        CACHE_DIR.mkdir(parents=True, exist_ok=True)
        cached.write_text(json.dumps(findings), encoding="utf-8")
    return path, findings, None


def count(findings: list[Finding]) -> dict[str, dict[str, int]]:
    """{file: {check: n}}; a header finding reported by several units is one location, counted once."""
    result: dict[str, Counter] = {}
    for file, _line, _column, check in set(findings):
        result.setdefault(file, Counter())[check] += 1
    return {file: dict(sorted(checks.items())) for file, checks in sorted(result.items())}


def compare(observed: dict[str, dict[str, int]], baseline: dict, scope: set[str]) -> list[str]:
    """New findings in files of *scope* relative to the baseline."""
    accepted = baseline.get("findings", {})
    clean = tuple(baseline.get("clean_modules", []))
    problems = []
    for file in sorted(scope):
        for check, n in observed.get(file, {}).items():
            allowed = 0 if file.startswith(clean) else accepted.get(file, {}).get(check, 0)
            if n > allowed:
                problems.append(f"{file}: {check}: {n} finding(s), {allowed} accepted")
    return problems


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-p", "--build-dir", type=Path, required=True, help="build directory with compile_commands.json")
    scope = parser.add_mutually_exclusive_group()
    scope.add_argument("--base", help="ref to diff against (default: merge base with origin/staging)")
    scope.add_argument("--all", action="store_true", help="check every in-scope translation unit")
    parser.add_argument("--update", action="store_true", help="shrink the baseline to the observed findings (needs --all)")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    parser.add_argument("--timeout", type=int, default=DEFAULT_TIMEOUT, help="seconds per translation unit")
    parser.add_argument("--no-cache", action="store_true")
    parser.add_argument("--extra-arg", action="append", default=[], help="passed to clang-tidy, e.g. -isysroot...")
    args = parser.parse_args(argv)
    if args.update and not args.all:
        parser.error("--update needs --all")

    try:
        binary = find_clang_tidy()
        tool_version = subprocess.run([binary, "--version"], capture_output=True, text=True).stdout
        units = load_database(args.build_dir.resolve())
        selected = sorted(units) if args.all else changed_units(args.base or default_base(), units)
        baseline = json.loads(BASELINE.read_text(encoding="utf-8"))
    except (FileNotFoundError, RuntimeError, subprocess.CalledProcessError, json.JSONDecodeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    stems = {Path(unit).stem for unit in selected}
    tracked = [p for p in git("ls-files", INCLUDE_PREFIX).split()
               if in_scope(p) and p.endswith(SOURCE_EXTENSIONS + HEADER_EXTENSIONS)]
    problems = suppression_problems([p for p in tracked if args.all or Path(p).stem in stems])
    if not selected:
        print("PASS: no in-scope translation units to check.")
        return 1 if problems else 0

    all_findings: list[Finding] = []
    errors = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(args.jobs, 1)) as pool:
        futures = [pool.submit(run_unit, binary, args.build_dir.resolve(), path, units[path], args.extra_arg,
                               args.timeout, tool_version, not args.no_cache) for path in selected]
        for future in concurrent.futures.as_completed(futures):
            path, unit_findings, error = future.result()
            if error:
                errors.append(f"{path}: {error}")
            all_findings.extend(unit_findings)
    observed = count(all_findings)

    scope_files = set(observed) | (set(baseline.get("findings", {})) if args.all else set(selected))
    problems += compare(observed, baseline, scope_files)
    problems += errors

    if args.update:
        old = baseline.get("findings", {})
        raised = compare(observed, baseline, set(observed)) if old else []
        if raised or errors:
            print("\n".join(raised + errors))
            print("FAIL: --update only shrinks the baseline; fix the new findings first.")
            return 1
        baseline["findings"] = {f: c for f, c in observed.items() if c}
        BASELINE.write_text(json.dumps(baseline, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        before = sum(sum(c.values()) for c in old.values())
        after = sum(sum(c.values()) for c in baseline["findings"].values())
        print(f"baseline updated: {before} -> {after} findings")
        return 0

    total = sum(sum(c.values()) for c in observed.values())
    if problems:
        print("\n".join(problems))
        print(f"FAIL: {len(problems)} problem(s) in {len(selected)} translation unit(s) ({total} findings observed).")
        return 1
    print(f"PASS: {len(selected)} translation unit(s), {total} finding(s), none new.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
