#!/usr/bin/env python3
# =============================================================================================================
#
# @file     summarize_coverage.py
# @author   MNE-CPP maintainers
# @since    2.4.0
# @date     August, 2026
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#
# @brief    Summarize an LCOV report into a reproducible coverage baseline.
#
# =============================================================================================================
"""Create deterministic JSON and Markdown summaries from an LCOV report."""

from __future__ import annotations

import argparse
import json
from collections import defaultdict
from dataclasses import asdict, dataclass
from pathlib import Path, PurePosixPath

SCOPES = ("libraries", "applications", "tools")
GENERATED_MARKERS = ("_autogen/", "/moc_", "/qrc_", "/ui_")


@dataclass
class FileCoverage:
    """Coverage counters for one source file."""

    path: str
    scope: str
    component: str
    lines_found: int = 0
    lines_hit: int = 0
    branches_found: int = 0
    branches_hit: int = 0
    generated_candidate: bool = False

    @property
    def lines_missed(self) -> int:
        return self.lines_found - self.lines_hit


def _normalise_path(value: str) -> str:
    value = value.replace("\\", "/")
    marker = value.find("src/")
    return value[marker:] if marker >= 0 else value


def _new_record(path: str) -> FileCoverage:
    normalised = _normalise_path(path)
    parts = PurePosixPath(normalised).parts
    if len(parts) < 3 or parts[0] != "src" or parts[1] not in SCOPES:
        raise ValueError(f"source is outside the measured production scope: {normalised}")
    component = "/".join(parts[:3])
    generated = any(marker in f"/{normalised}" for marker in GENERATED_MARKERS)
    return FileCoverage(normalised, parts[1], component, generated_candidate=generated)


def parse_lcov(path: Path) -> list[FileCoverage]:
    """Parse file-level line and branch counters from an LCOV tracefile."""
    records: dict[str, FileCoverage] = {}
    current: FileCoverage | None = None

    for raw_line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if raw_line.startswith("SF:"):
            current = _new_record(raw_line[3:])
        elif current is not None and raw_line.startswith("LF:"):
            current.lines_found = int(raw_line[3:])
        elif current is not None and raw_line.startswith("LH:"):
            current.lines_hit = int(raw_line[3:])
        elif current is not None and raw_line.startswith("BRF:"):
            current.branches_found = int(raw_line[4:])
        elif current is not None and raw_line.startswith("BRH:"):
            current.branches_hit = int(raw_line[4:])
        elif raw_line == "end_of_record" and current is not None:
            previous = records.get(current.path)
            if previous is None:
                records[current.path] = current
            else:
                previous.lines_found = max(previous.lines_found, current.lines_found)
                previous.lines_hit = max(previous.lines_hit, current.lines_hit)
                previous.branches_found = max(previous.branches_found, current.branches_found)
                previous.branches_hit = max(previous.branches_hit, current.branches_hit)
            current = None

    if current is not None:
        raise ValueError(f"unterminated LCOV record for {current.path}")
    if not records:
        raise ValueError(f"no source records found in {path}")
    return [records[name] for name in sorted(records)]


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
        "line_percent": round(100.0 * lines_hit / lines_found, 2) if lines_found else None,
        "branches_found": branches_found,
        "branches_hit": branches_hit,
        "branches_missed": branches_found - branches_hit,
        "branch_percent": round(100.0 * branches_hit / branches_found, 2)
        if branches_found
        else None,
    }


def build_summary(
    files: list[FileCoverage], *, commit: str | None, source_url: str | None, top: int
) -> dict:
    """Build the stable coverage-baseline document."""
    grouped: dict[str, list[FileCoverage]] = defaultdict(list)
    for item in files:
        grouped[item.component].append(item)

    maintained = [item for item in files if not item.generated_candidate]
    generated = [item for item in files if item.generated_candidate]
    priorities = sorted(maintained, key=lambda item: (-item.lines_missed, item.path))[:top]
    return {
        "schema_version": 1,
        "source": {"commit": commit, "coverage_url": source_url},
        "scope": [f"src/{scope}" for scope in SCOPES],
        "exclusions": [],
        "notes": [
            "Generated-code candidates remain in measured totals and are reported separately.",
            "A null branch percentage means the LCOV input did not contain branch counters.",
        ],
        "totals": _totals(files),
        "maintained_totals": _totals(maintained),
        "generated_candidate_totals": _totals(generated),
        "scopes": {
            scope: _totals([item for item in files if item.scope == scope]) for scope in SCOPES
        },
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
            asdict(item) | {"lines_missed": item.lines_missed}
            for item in files
        ],
    }


def _percent(value: float | None) -> str:
    return "not reported" if value is None else f"{value:.2f}%"


def render_markdown(summary: dict) -> str:
    """Render the human-readable companion to the JSON baseline."""
    totals = summary["totals"]
    generated = summary["generated_candidate_totals"]
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
        f"- Generated-code candidates: {generated['files']:,} files and {generated['lines_found']:,} lines",
        "",
        "Generated-code candidates are included in the measured totals above. No exclusions are applied.",
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
    lines += [
        "",
        "## Priority files",
        "",
        "Generated-code candidates are omitted from this ranking but remain in all totals.",
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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("lcov", type=Path, help="LCOV tracefile to summarize")
    parser.add_argument("--commit", help="commit represented by the tracefile")
    parser.add_argument("--source-url", help="published workflow or Codecov URL")
    parser.add_argument("--json", type=Path, dest="json_path", help="write the JSON baseline")
    parser.add_argument("--markdown", type=Path, help="write the Markdown baseline")
    parser.add_argument("--top", type=int, default=25, help="number of priority files to report")
    args = parser.parse_args()

    if args.top < 1:
        parser.error("--top must be at least 1")
    summary = build_summary(
        parse_lcov(args.lcov), commit=args.commit, source_url=args.source_url, top=args.top
    )
    if args.json_path:
        args.json_path.parent.mkdir(parents=True, exist_ok=True)
        args.json_path.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    if args.markdown:
        args.markdown.parent.mkdir(parents=True, exist_ok=True)
        args.markdown.write_text(render_markdown(summary), encoding="utf-8")
    if not args.json_path and not args.markdown:
        print(render_markdown(summary), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())