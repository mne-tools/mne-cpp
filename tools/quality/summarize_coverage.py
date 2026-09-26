#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     August, 2026
#
#
# @brief    Normalise, filter and summarise an LCOV report under the coverage policy.
"""Normalise, filter and summarise an LCOV report under ``coverage_policy.json``.

Every number published for gate G2 passes through this one script (T0.2, T2.2):

* source paths become repository-relative, so the summary, the gate and Codecov
  name the same files;
* the exclusions in the policy are applied; an exclusion without a reason, or
  (with ``--repo-root``) one that matches no file, is an error;
* with ``--repo-root``, every tracked translation unit must be in the report,
  excluded, or free of executable code, and every reported file must be tracked;
* line and branch totals are summarised per scope and component;
* ``--lcov-out`` writes the filtered report that CI gates on and uploads, with
  line and branch records only (Codecov drops branches on lines where a mangled
  C++ function starts, so omitting ``FN`` keeps its line model identical);
* ``--codecov-report`` compares per-file totals with Codecov's report for the
  same upload, using Codecov's LCOV rules (branch counters decide a line, and a
  partially taken branch line is a hit, as ``partials_as_hits`` in codecov.yml).

Usage
-----
    python3 tools/quality/summarize_coverage.py coverage_raw.info --repo-root . \\
        --lcov-out coverage_filtered.info --json summary.json --markdown summary.md

    curl -s "https://api.codecov.io/api/v2/github/mne-tools/repos/mne-cpp/report/?sha=<sha>" -o codecov.json
    python3 tools/quality/summarize_coverage.py coverage_filtered.info --codecov-report codecov.json

Exit codes
----------
    0   report written; policy, completeness and reconciliation pass
    1   policy, completeness or reconciliation failure (outputs are still written)
    2   unreadable input
"""

from __future__ import annotations

import argparse
import fnmatch
import json
import re
import subprocess
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path, PurePosixPath
from typing import Callable

SCOPES = ("libraries", "applications", "tools")
POLICY_FILE = Path(__file__).resolve().parent / "coverage_policy.json"
TRANSLATION_UNIT_SUFFIXES = (".c", ".cc", ".cpp", ".cxx")
_SCOPE_MARKER = re.compile(r"(?:^|/)(src/(?:" + "|".join(SCOPES) + r")/.*)$")


class PolicyError(ValueError):
    """The coverage policy is malformed."""


@dataclass
class FileCoverage:
    """Per-line counters for one source file."""

    path: str
    lines: dict[int, int] = field(default_factory=dict)
    # (line, block, branch) -> times taken; None when the block never ran ("-").
    branches: dict[tuple[int, str, str], int | None] = field(default_factory=dict)

    @property
    def scope(self) -> str:
        return PurePosixPath(self.path).parts[1]

    @property
    def component(self) -> str:
        return "/".join(PurePosixPath(self.path).parts[:3])

    @property
    def lines_found(self) -> int:
        return len(self.lines)

    @property
    def lines_hit(self) -> int:
        return sum(1 for count in self.lines.values() if count > 0)

    @property
    def lines_missed(self) -> int:
        return self.lines_found - self.lines_hit

    @property
    def branches_found(self) -> int:
        return len(self.branches)

    @property
    def branches_hit(self) -> int:
        return sum(1 for taken in self.branches.values() if taken)


def normalise_path(value: str) -> str:
    """Repository-relative path, or ValueError when outside the measured scopes."""
    match = _SCOPE_MARKER.search(value.replace("\\", "/"))
    if match is None or len(PurePosixPath(match.group(1)).parts) < 4:
        raise ValueError(f"source is outside the measured production scope: {value}")
    return match.group(1)


def parse_lcov(path: Path) -> list[FileCoverage]:
    """Parse ``DA`` and ``BRDA`` records; records for the same file are summed."""
    records: dict[str, FileCoverage] = {}
    current: FileCoverage | None = None

    for raw_line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        tag, _, content = raw_line.partition(":")
        if tag == "SF":
            name = normalise_path(content.strip())
            current = records.setdefault(name, FileCoverage(name))
        elif raw_line == "end_of_record":
            if current is None:
                raise ValueError(f"end_of_record without SF in {path}")
            current = None
        elif current is None:
            continue
        elif tag == "DA":
            number, count = content.split(",")[:2]
            line = int(number)
            current.lines[line] = current.lines.get(line, 0) + max(int(count), 0)
        elif tag == "BRDA":
            number, block, branch, taken = content.split(",")[:4]
            key = (int(number), block, branch)
            previous = current.branches.get(key)
            if taken == "-":
                current.branches.setdefault(key, None)
            else:
                current.branches[key] = (previous or 0) + int(taken)

    if current is not None:
        raise ValueError(f"unterminated LCOV record for {current.path}")
    if not records:
        raise ValueError(f"no source records found in {path}")
    return [records[name] for name in sorted(records)]


def load_exclusions(policy_path: Path) -> list[dict[str, str]]:
    """The policy's exclusions; every entry needs exactly a pattern and a reason."""
    policy = json.loads(policy_path.read_text(encoding="utf-8"))
    entries = policy.get("exclusions", [])
    if not isinstance(entries, list):
        raise PolicyError(f"{policy_path}: 'exclusions' must be a list")
    errors = []
    for index, entry in enumerate(entries):
        if not isinstance(entry, dict):
            errors.append(f"exclusion {index} is not an object")
            continue
        label = entry.get("pattern") or f"#{index}"
        if set(entry) != {"pattern", "reason"}:
            errors.append(f"exclusion {label}: expected keys pattern and reason, got {sorted(entry)}")
        for key in ("pattern", "reason"):
            if not isinstance(entry.get(key), str) or not entry[key].strip():
                errors.append(f"exclusion {label}: missing {key}")
    if errors:
        raise PolicyError(f"{policy_path}: " + "; ".join(errors))
    return entries


def matching_exclusion(path: str, exclusions: list[dict[str, str]]) -> dict[str, str] | None:
    return next((entry for entry in exclusions if fnmatch.fnmatchcase(path, entry["pattern"])), None)


def has_executable_code(source: str) -> bool:
    """False for a translation unit holding only comments, directives and using-directives."""
    source = re.sub(r"/\*.*?\*/", "", source, flags=re.DOTALL)
    source = re.sub(r"//[^\n]*", "", source)
    for line in source.splitlines():
        line = line.strip()
        if line and not line.startswith("#") and not re.fullmatch(r"using namespace [\w:]+;", line):
            return True
    return False


def tracked_sources(repo_root: Path) -> list[str]:
    result = subprocess.run(
        ["git", "-C", str(repo_root), "ls-files", "--", *(f"src/{scope}" for scope in SCOPES)],
        check=True, capture_output=True, text=True,
    )
    return sorted(result.stdout.split())


def check_completeness(
    reported: set[str],
    tracked: list[str],
    exclusions: list[dict[str, str]],
    read_text: Callable[[str], str],
) -> dict:
    """Account for every tracked translation unit and every reported file."""
    tracked_set = set(tracked)
    units = [path for path in tracked if path.endswith(TRANSLATION_UNIT_SUFFIXES)]
    excluded = [path for path in units if matching_exclusion(path, exclusions)]
    unaccounted = [path for path in units if path not in reported and path not in excluded]
    empty = [path for path in unaccounted if not has_executable_code(read_text(path))]
    return {
        "checked": True,
        "translation_units": len(units),
        "reported": sum(1 for path in units if path in reported),
        "excluded": len(excluded),
        "no_executable_code": empty,
        "missing": [path for path in unaccounted if path not in empty],
        "untracked": sorted(path for path in reported if path not in tracked_set),
    }


def stale_exclusions(exclusions: list[dict[str, str]], paths: set[str]) -> list[str]:
    return [entry["pattern"] for entry in exclusions
            if not any(fnmatch.fnmatchcase(path, entry["pattern"]) for path in paths)]


def codecov_counts(item: FileCoverage) -> tuple[int, int]:
    """(lines, hits) as Codecov counts them with ``partials_as_hits``."""
    status = {line: count > 0 for line, count in item.lines.items()}
    taken: dict[int, bool] = {}
    for (line, _, _), count in item.branches.items():
        taken[line] = taken.get(line, False) or bool(count)
    status.update(taken)
    return len(status), sum(status.values())


def reconcile(files: list[FileCoverage], codecov: dict) -> list[str]:
    """Per-file differences between the local report and Codecov's."""
    theirs = {entry["name"]: entry["totals"] for entry in codecov["files"]}
    ours = {item.path: codecov_counts(item) for item in files}
    problems = []
    for path in sorted(set(ours) | set(theirs)):
        mine, other = ours.get(path), theirs.get(path)
        if other is None:
            if mine and mine[0]:
                problems.append(f"{path}: missing from Codecov")
        elif mine is None:
            problems.append(f"{path}: in Codecov but not in the report")
        elif (other["lines"], other["hits"]) != mine:
            problems.append(f"{path}: Codecov {other['hits']}/{other['lines']}, expected {mine[1]}/{mine[0]}")
    return problems


def _percent_of(part: int, whole: int) -> float | None:
    return round(100.0 * part / whole, 2) if whole else None


def _totals(files: list[FileCoverage]) -> dict[str, int | float | None]:
    lines_found = sum(item.lines_found for item in files)
    lines_hit = sum(item.lines_hit for item in files)
    branches_found = sum(item.branches_found for item in files)
    branches_hit = sum(item.branches_hit for item in files)
    return {
        "files": len(files),
        "lines_found": lines_found,
        "lines_hit": lines_hit,
        "lines_missed": lines_found - lines_hit,
        "line_percent": _percent_of(lines_hit, lines_found),
        "branches_found": branches_found,
        "branches_hit": branches_hit,
        "branches_missed": branches_found - branches_hit,
        "branch_percent": _percent_of(branches_hit, branches_found),
    }


def build_summary(
    files: list[FileCoverage],
    *,
    commit: str | None,
    source_url: str | None,
    top: int,
    exclusions: list[dict[str, str]] | None = None,
    excluded: list[FileCoverage] | None = None,
    tracked: list[str] | None = None,
    completeness: dict | None = None,
) -> dict:
    """Build the stable coverage-baseline document."""
    exclusions = exclusions or []
    excluded = excluded or []
    grouped: dict[str, list[FileCoverage]] = defaultdict(list)
    for item in files:
        grouped[item.component].append(item)

    exclusion_rows = []
    for entry in exclusions:
        hits = [item for item in excluded if matching_exclusion(item.path, exclusions) is entry]
        exclusion_rows.append({
            "pattern": entry["pattern"],
            "reason": entry["reason"],
            "reported_files": len(hits),
            "reported_lines": sum(item.lines_found for item in hits),
            "tracked_files": sum(1 for path in tracked or [] if fnmatch.fnmatchcase(path, entry["pattern"])),
        })

    codecov_lines = codecov_hits = 0
    for item in files:
        lines, hits = codecov_counts(item)
        codecov_lines += lines
        codecov_hits += hits

    priorities = sorted(files, key=lambda item: (-item.lines_missed, item.path))[:top]
    return {
        "schema_version": 2,
        "source": {"commit": commit, "coverage_url": source_url},
        "scope": [f"src/{scope}" for scope in SCOPES],
        "exclusions": exclusion_rows,
        "completeness": completeness or {"checked": False},
        "notes": [
            "Totals exclude only the files matched by the reasoned exclusions in tools/quality/coverage_policy.json.",
            "A null branch percentage means the LCOV input did not contain branch counters.",
            "codecov_model counts lines the way Codecov does with partials_as_hits: a line with branch counters "
            "is a hit when any of its branches was taken.",
        ],
        "totals": _totals(files),
        "codecov_model": {
            "lines": codecov_lines,
            "hits": codecov_hits,
            "line_percent": _percent_of(codecov_hits, codecov_lines),
        },
        "scopes": {scope: _totals([item for item in files if item.scope == scope]) for scope in SCOPES},
        "components": {name: _totals(grouped[name]) for name in sorted(grouped)},
        "priority_files": [
            {
                "path": item.path,
                "component": item.component,
                "lines_found": item.lines_found,
                "lines_hit": item.lines_hit,
                "lines_missed": item.lines_missed,
            }
            for item in priorities
        ],
        "files": [
            {
                "path": item.path,
                "scope": item.scope,
                "component": item.component,
                "lines_found": item.lines_found,
                "lines_hit": item.lines_hit,
                "lines_missed": item.lines_missed,
                "branches_found": item.branches_found,
                "branches_hit": item.branches_hit,
            }
            for item in files
        ],
    }


def write_lcov(files: list[FileCoverage], path: Path) -> None:
    out = []
    for item in files:
        out += ["TN:", f"SF:{item.path}"]
        for (line, block, branch), taken in sorted(item.branches.items()):
            out.append(f"BRDA:{line},{block},{branch},{'-' if taken is None else taken}")
        out += [f"BRF:{item.branches_found}", f"BRH:{item.branches_hit}"]
        out += [f"DA:{line},{count}" for line, count in sorted(item.lines.items())]
        out += [f"LF:{item.lines_found}", f"LH:{item.lines_hit}", "end_of_record"]
    path.write_text("\n".join(out) + "\n", encoding="utf-8")


def _percent(value: float | None) -> str:
    return "not reported" if value is None else f"{value:.2f}%"


def render_markdown(summary: dict) -> str:
    """Render the human-readable companion to the JSON baseline."""
    totals = summary["totals"]
    model = summary["codecov_model"]
    lines = [
        "# MNE-CPP v2.4.0 coverage baseline",
        "",
        f"Commit: `{summary['source']['commit'] or 'unspecified'}`",
        f"Coverage source: {summary['source']['coverage_url'] or 'local LCOV input'}",
        "",
        "## Totals",
        "",
        f"- Lines: {totals['lines_hit']:,} / {totals['lines_found']:,} ({_percent(totals['line_percent'])})",
        f"- Branches: {totals['branches_hit']:,} / {totals['branches_found']:,} ({_percent(totals['branch_percent'])})",
        f"- Files: {totals['files']:,}",
        f"- Codecov model (partials as hits): {model['hits']:,} / {model['lines']:,} ({_percent(model['line_percent'])})",
        "",
        "## Scope",
        "",
        "| Scope | Lines | Coverage | Branches | Branch coverage |",
        "|---|---:|---:|---:|---:|",
    ]
    for name, values in summary["scopes"].items():
        lines.append(
            f"| `src/{name}` | {values['lines_hit']:,} / {values['lines_found']:,} "
            f"| {_percent(values['line_percent'])} | {values['branches_hit']:,} / "
            f"{values['branches_found']:,} | {_percent(values['branch_percent'])} |"
        )
    lines += ["", "## Exclusions", ""]
    if summary["exclusions"]:
        lines += ["| Pattern | Reported files | Reported lines | Reason |", "|---|---:|---:|---|"]
        for row in summary["exclusions"]:
            lines.append(f"| `{row['pattern']}` | {row['reported_files']:,} | {row['reported_lines']:,} | {row['reason']} |")
    else:
        lines.append("None.")
    completeness = summary["completeness"]
    lines += ["", "## Completeness", ""]
    if completeness["checked"]:
        lines.append(
            f"{completeness['reported']:,} of {completeness['translation_units']:,} tracked translation units "
            f"are in the report and {completeness['excluded']:,} are excluded. "
            f"{len(completeness['no_executable_code'])} contain no executable code; "
            f"{len(completeness['missing'])} are missing; {len(completeness['untracked'])} reported files are untracked."
        )
        for key in ("missing", "untracked", "no_executable_code"):
            for path in completeness[key]:
                lines.append(f"- {key.replace('_', ' ')}: `{path}`")
    else:
        lines.append("Not checked (no `--repo-root`).")
    lines += [
        "",
        "## Priority files",
        "",
        "| Uncovered lines | Covered / found | File |",
        "|---:|---:|---|",
    ]
    for item in summary["priority_files"]:
        lines.append(
            f"| {item['lines_missed']:,} | {item['lines_hit']:,} / {item['lines_found']:,} "
            f"| `{item['path']}` |"
        )
    return "\n".join(lines) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("lcov", type=Path, help="LCOV tracefile to summarize")
    parser.add_argument("--policy", type=Path, default=POLICY_FILE, help="coverage policy with the exclusions")
    parser.add_argument("--repo-root", type=Path, help="checkout to check completeness and stale exclusions against")
    parser.add_argument("--lcov-out", type=Path, help="write the normalised, filtered LCOV report")
    parser.add_argument("--codecov-report", type=Path, help="Codecov API report JSON to reconcile against")
    parser.add_argument("--commit", help="commit represented by the tracefile")
    parser.add_argument("--source-url", help="published workflow or Codecov URL")
    parser.add_argument("--json", type=Path, dest="json_path", help="write the JSON baseline")
    parser.add_argument("--markdown", type=Path, help="write the Markdown baseline")
    parser.add_argument("--top", type=int, default=25, help="number of priority files to report")
    args = parser.parse_args(argv)

    if args.top < 1:
        parser.error("--top must be at least 1")
    try:
        exclusions = load_exclusions(args.policy)
    except PolicyError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    try:
        records = parse_lcov(args.lcov)
        codecov = json.loads(args.codecov_report.read_text(encoding="utf-8")) if args.codecov_report else None
    except (OSError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    files = [item for item in records if not matching_exclusion(item.path, exclusions)]
    excluded = [item for item in records if matching_exclusion(item.path, exclusions)]
    failures = []
    tracked = completeness = None
    if args.repo_root:
        tracked = tracked_sources(args.repo_root)
        completeness = check_completeness(
            {item.path for item in files}, tracked, exclusions,
            lambda path: (args.repo_root / path).read_text(encoding="utf-8", errors="replace"),
        )
        failures += [f"tracked translation unit missing from the report: {path}" for path in completeness["missing"]]
        failures += [f"reported file is not tracked in the repository: {path}" for path in completeness["untracked"]]
        failures += [f"exclusion matches no file: {pattern}"
                     for pattern in stale_exclusions(exclusions, set(tracked) | {item.path for item in records})]

    summary = build_summary(
        files, commit=args.commit, source_url=args.source_url, top=args.top,
        exclusions=exclusions, excluded=excluded, tracked=tracked, completeness=completeness,
    )
    if args.lcov_out:
        write_lcov(files, args.lcov_out)
    if args.json_path:
        args.json_path.parent.mkdir(parents=True, exist_ok=True)
        args.json_path.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    if args.markdown:
        args.markdown.parent.mkdir(parents=True, exist_ok=True)
        args.markdown.write_text(render_markdown(summary), encoding="utf-8")
    if codecov is not None:
        problems = reconcile(files, codecov)
        model, theirs = summary["codecov_model"], codecov["totals"]
        print(f"Local model : {model['hits']:,} / {model['lines']:,} lines ({_percent(model['line_percent'])})")
        print(f"Codecov     : {theirs['hits']:,} / {theirs['lines']:,} lines ({theirs['coverage']}%)")
        failures += problems
    if not any((args.lcov_out, args.json_path, args.markdown, codecov is not None)):
        print(render_markdown(summary), end="")

    for failure in failures:
        print(f"FAIL: {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
