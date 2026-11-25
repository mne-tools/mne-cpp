#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     September, 2026
#
#
# @brief    Enforce the CI coverage floor and regression tolerance from one policy.
"""Check line coverage from an LCOV file against ``coverage_policy.json``.

The workflow used to hard-code a 20% floor and a 2.0-point regression allowance
in shell, neither of which matched release gate G2.  The numbers now live in one
policy file, the CI log prints exactly what is enforced, and the script can be
tested like any other code.

The libraries listed under ``library_floors`` have their own floors, so a weak
library on the signal path cannot hide behind the global figure.

The previous run's coverage is read from ``--baseline`` (restored from the
Actions cache).  Only a passing run writes the current value back, so a failing
run cannot lower the bar for the next one.

Usage
-----
    python3 scripts/quality/check_coverage_gate.py coverage_filtered.info \\
        --baseline .coverage-baseline

Exit codes
----------
    0   floor and regression check pass
    1   coverage below the floor or dropped more than the tolerance
    2   unreadable input or policy
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

POLICY_FILE = Path(__file__).resolve().parent / "coverage_policy.json"


def line_coverage(lcov: Path) -> tuple[int, int]:
    """(hit, found) over every ``DA:`` record, the same count the workflow used."""
    hit = found = 0
    for file_hit, file_found in file_coverage(lcov).values():
        hit += file_hit
        found += file_found
    return hit, found


def file_coverage(lcov: Path) -> dict[str, tuple[int, int]]:
    """(hit, found) of every source file (``SF:``) of the report."""
    files: dict[str, list[int]] = {}
    counts = files.setdefault("", [0, 0])
    with lcov.open(encoding="utf-8", errors="replace") as handle:
        for line in handle:
            if line.startswith("SF:"):
                counts = files.setdefault(line[3:].strip(), [0, 0])
            elif line.startswith("DA:"):
                counts[1] += 1
                fields = line[3:].strip().split(",")
                if len(fields) >= 2 and fields[1].lstrip("-").isdigit() and int(fields[1]) > 0:
                    counts[0] += 1
    return {name: (hit, found) for name, (hit, found) in files.items() if found}


def library_coverage(files: dict[str, tuple[int, int]], prefixes: list[str]) -> dict[str, tuple[int, int]]:
    """(hit, found) summed over the files under each path prefix; reports may hold relative or absolute paths."""
    result = {}
    for prefix in prefixes:
        needle = "/" + prefix.strip("/") + "/"
        matched = [counts for name, counts in files.items() if needle in "/" + name.replace("\\", "/")]
        result[prefix] = (sum(h for h, _ in matched), sum(f for _, f in matched))
    return result


def library_floors(policy: dict) -> dict[str, float]:
    return {prefix: float(floor) for prefix, floor in policy.get("library_floors", {}).items() if not prefix.startswith("$")}


def load_policy(path: Path) -> dict:
    policy = json.loads(path.read_text(encoding="utf-8"))
    for key in ("line_floor_percent", "max_regression_points"):
        if key not in policy:
            raise ValueError(f"{path}: missing '{key}'")
    return policy


def evaluate(current: float, previous: float | None, policy: dict) -> list[str]:
    failures = []
    floor = float(policy["line_floor_percent"])
    tolerance = float(policy["max_regression_points"])
    if current < floor:
        failures.append(f"line coverage {current:.2f}% is below the floor of {floor:.2f}%")
    if previous is not None and previous - current > tolerance + 1e-9:
        failures.append(
            f"line coverage dropped {previous - current:.2f} points ({previous:.2f}% -> {current:.2f}%), "
            f"more than the allowed {tolerance:.2f}"
        )
    return failures


def evaluate_libraries(libraries: dict[str, tuple[int, int]], policy: dict) -> list[str]:
    failures = []
    for prefix, floor in library_floors(policy).items():
        hit, found = libraries.get(prefix, (0, 0))
        if found == 0:
            failures.append(f"{prefix} has a coverage floor but no lines in the report")
        elif hit * 100.0 / found < floor:
            failures.append(f"{prefix} line coverage {hit * 100.0 / found:.2f}% is below its floor of {floor:.2f}%")
    return failures


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("lcov", type=Path, help="filtered LCOV report")
    parser.add_argument("--baseline", type=Path, help="previous coverage value; rewritten with the current one")
    parser.add_argument("--policy", type=Path, default=POLICY_FILE)
    args = parser.parse_args(argv)

    try:
        policy = load_policy(args.policy)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    try:
        files = file_coverage(args.lcov)
    except OSError as exc:
        print(f"error: cannot read {args.lcov}: {exc}", file=sys.stderr)
        return 2
    hit = sum(h for h, _ in files.values())
    found = sum(f for _, f in files.values())
    if found == 0:
        print(f"error: {args.lcov} has no line records", file=sys.stderr)
        return 2
    current = round(hit * 100.0 / found, 2)

    previous = None
    if args.baseline and args.baseline.is_file():
        try:
            previous = float(args.baseline.read_text(encoding="utf-8").strip())
        except ValueError:
            print(f"warning: ignoring unreadable baseline {args.baseline}", file=sys.stderr)

    print(f"Line coverage        : {current:.2f}% ({hit:,} / {found:,} lines)")
    print(f"Floor                : {float(policy['line_floor_percent']):.2f}%")
    print(f"Regression tolerance : {float(policy['max_regression_points']):.2f} points")
    print(f"Previous baseline    : {'none' if previous is None else f'{previous:.2f}%'}")

    libraries = library_coverage(files, list(library_floors(policy)))
    for prefix, floor in library_floors(policy).items():
        lib_hit, lib_found = libraries[prefix]
        measured = f"{lib_hit * 100.0 / lib_found:.2f}%" if lib_found else "no lines"
        print(f"Floor {prefix:<20}: {floor:.2f}% (measured {measured})")

    failures = evaluate(current, previous, policy) + evaluate_libraries(libraries, policy)
    for failure in failures:
        print(f"FAIL: {failure}")

    if args.baseline and not failures:
        args.baseline.write_text(f"{current:.2f}\n", encoding="utf-8")

    if failures:
        return 1
    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
