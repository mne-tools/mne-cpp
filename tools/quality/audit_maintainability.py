#!/usr/bin/env python3
# =============================================================================================================
#
# @file     audit_maintainability.py
# @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
# @since    2.4.0
# @date     September, 2026
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#
# @brief    Count maintainability debt per file so it can be ratcheted and assigned.
#
# =============================================================================================================
"""Maintainability baseline for gate G5 (package T0.5).

Counts lexical debt candidates in maintained C++ under ``src/libraries``,
``src/applications`` and ``src/tools``:

* ``null_macro``        uses of ``NULL`` instead of ``nullptr``
* ``typedef_struct``    C-style ``typedef struct``
* ``c_style_cast``      casts to a builtin/pointer type written as ``(T)expr``
* ``raw_new``           non-Qt ``new T`` not immediately handed to a smart pointer or a parent
* ``qt_new_unparented`` ``new QXxx`` with no parent on the same line (often reparented later by a layout)
* ``raw_delete``        explicit ``delete`` / ``delete[]``
* ``console_io``        ``printf``/``fprintf``/``std::cout``/``std::cerr`` (T6.6 decides keep-or-convert)
* ``numeric_define``    ``#define NAME <number>`` constants that could be ``constexpr``
* ``oversized_unit``    files longer than the policy threshold

Comments and string literals are stripped before matching, so a ``NULL`` in a
doc comment or a ``"printf"`` in a message is not counted.  Every metric lists
the files that contribute to it, which is what T6.4-T6.8 need to cut
path-disjoint tranches.  Compiler warnings are not counted here: the build is
already ``-Werror`` (T6.1), so the warning count is zero by construction.

Usage
-----
    python3 tools/quality/audit_maintainability.py
    python3 tools/quality/audit_maintainability.py --json doc/release/v2.4.0/maintainability-baseline.json \\
        --markdown doc/release/v2.4.0/maintainability-baseline.md
    # fail if any metric exceeds the committed baseline (the ratchet)
    python3 tools/quality/audit_maintainability.py --check doc/release/v2.4.0/maintainability-baseline.json
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[2]
POLICY_FILE = Path(__file__).resolve().parent / "maintainability_policy.json"

_BUILTIN = r"(?:unsigned\s+|signed\s+|const\s+)?(?:int|float|double|char|long|short|bool|size_t|u?int(?:8|16|32|64)_t|q(?:u)?int(?:8|16|32|64)|qreal|void)"
PATTERNS: dict[str, re.Pattern[str]] = {
    "null_macro": re.compile(r"\bNULL\b"),
    "typedef_struct": re.compile(r"\btypedef\s+struct\b"),
    "c_style_cast": re.compile(rf"(?<![\w\]\)>])\(\s*{_BUILTIN}(?:\s*\*+)?\s*\)\s*(?=[\w(\-])"),
    "raw_new": re.compile(r"\bnew\s+(?!\(std::nothrow\))[A-Za-z_][\w:]*(?:<[^;]*?>)?\s*[\[(;{]"),
    "raw_delete": re.compile(r"\bdelete\s*(?:\[\s*\])?\s*[A-Za-z_(*]"),
    "console_io": re.compile(r"(?<![\w:.>])(?:std::)?(?:printf|fprintf|puts)\s*\(|\bstd::(?:cout|cerr)\b"),
    "numeric_define": re.compile(r"^\s*#\s*define\s+[A-Z_][A-Z0-9_]*\s+\(?-?\d", re.MULTILINE),
}
# A `new` whose result lands in a smart pointer (including the project's X::SPtr/UPtr typedefs) is owned.
_OWNED_NEW = re.compile(
    r"(?:make_(?:unique|shared)|(?:Q|std::)(?:Shared|Scoped|Unique)?(?:Pointer|_ptr|Ptr)\s*<[^;]*>\s*\w*\s*[({=]"
    r"|\b(?:Const)?[SU]Ptr\s*(?:\w+\s*)?[({=]|\.reset\s*\(|::create\s*\()\s*[^;]*$"
)
# Qt objects are usually handed to a parent or layout after construction, which a lexical scan cannot see.
_QT_NEW = re.compile(r"\bnew\s+Q[A-Z]\w*")


def strip_comments_and_strings(text: str) -> str:
    """Blank out comments and literals, keeping line structure and preprocessor lines."""
    out: list[str] = []
    index, length = 0, len(text)
    while index < length:
        char = text[index]
        pair = text[index:index + 2]
        if pair == "//":
            end = text.find("\n", index)
            index = length if end < 0 else end
        elif pair == "/*":
            end = text.find("*/", index + 2)
            end = length if end < 0 else end + 2
            out.append("".join("\n" if c == "\n" else " " for c in text[index:end]))
            index = end
        elif char in "\"'":
            if char == "'" and index > 0 and text[index - 1].isalnum():
                out.append(char)  # C++14 digit separator
                index += 1
                continue
            end = index + 1
            while end < length and text[end] != char and text[end] != "\n":
                end += 2 if text[end] == "\\" else 1
            out.append(char + " " * max(end - index - 1, 0) + char)
            index = end + 1
        else:
            out.append(char)
            index += 1
    return "".join(out)


def count_file(text: str) -> dict[str, int]:
    code = strip_comments_and_strings(text)
    counts = {name: len(pattern.findall(code)) for name, pattern in PATTERNS.items()}
    owned = qt = 0
    for line in code.splitlines():
        for match in PATTERNS["raw_new"].finditer(line):
            if _OWNED_NEW.search(line[:match.start()] + "("):
                owned += 1
            elif re.search(r"^\s*this\s*\)|[(,]\s*this\s*\)|\bparent\w*\s*\)", line[match.end():]):
                owned += 1  # Qt parent-child ownership
            elif _QT_NEW.match(line, match.start()):
                qt += 1
    counts["raw_new"] -= owned + qt
    counts["qt_new_unparented"] = qt
    return counts


def scan(repo_root: Path, policy: dict[str, Any]) -> dict[str, dict[str, int]]:
    extensions = set(policy["extensions"])
    excluded = tuple(policy.get("exclude_path_fragments", {}))
    files: dict[str, dict[str, int]] = {}
    for root in policy["scan_roots"]:
        for path in sorted((repo_root / root).rglob("*")):
            if path.suffix not in extensions or not path.is_file():
                continue
            relative = path.relative_to(repo_root).as_posix()
            if any(fragment in f"/{relative}" for fragment in excluded):
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            counts = count_file(text)
            counts["lines"] = text.count("\n") + 1
            files[relative] = counts
    return files


def build_report(repo_root: Path = REPO_ROOT, policy: dict[str, Any] | None = None) -> dict[str, Any]:
    policy = policy or json.loads(POLICY_FILE.read_text(encoding="utf-8"))
    suppressions = {k: v for k, v in policy.get("suppressions", {}).items() if not k.startswith("$")}
    threshold = int(policy["oversized_lines"])
    files = scan(repo_root, policy)

    metrics: dict[str, Any] = {}
    for name in [*PATTERNS, "qt_new_unparented", "oversized_unit"]:
        suppressed = suppressions.get(name, {})
        per_file: dict[str, int] = {}
        for path, counts in files.items():
            if path in suppressed:
                continue
            value = (1 if counts["lines"] > threshold else 0) if name == "oversized_unit" else counts[name]
            if value:
                per_file[path] = counts["lines"] if name == "oversized_unit" else value
        by_scope: dict[str, int] = {}
        for path, value in per_file.items():
            scope = path.split("/")[1]
            by_scope[scope] = by_scope.get(scope, 0) + (1 if name == "oversized_unit" else value)
        metrics[name] = {
            "total": len(per_file) if name == "oversized_unit" else sum(per_file.values()),
            "files": len(per_file),
            "by_scope": dict(sorted(by_scope.items())),
            "suppressed": sorted(suppressed),
            "top": dict(sorted(per_file.items(), key=lambda item: (-item[1], item[0]))[:15]),
            "all_files": dict(sorted(per_file.items())),
        }
    return {
        "schema_version": 1,
        "generated_by": "tools/quality/audit_maintainability.py",
        "policy": "tools/quality/maintainability_policy.json",
        "scanned_files": len(files),
        "scanned_lines": sum(c["lines"] for c in files.values()),
        "oversized_threshold": threshold,
        "metrics": metrics,
    }


def check(report: dict[str, Any], baseline: dict[str, Any]) -> list[str]:
    """The ratchet: no metric may grow, in total or in any single file."""
    failures: list[str] = []
    for name, metric in report["metrics"].items():
        previous = baseline.get("metrics", {}).get(name)
        if previous is None:
            continue
        if metric["total"] > previous["total"]:
            failures.append(f"{name}: {metric['total']} > baseline {previous['total']}")
        for path, value in metric["all_files"].items():
            if value > previous["all_files"].get(path, 0) and name != "oversized_unit":
                failures.append(f"{name}: {path} has {value}, baseline {previous['all_files'].get(path, 0)}")
            elif name == "oversized_unit" and path not in previous["all_files"]:
                failures.append(f"{name}: {path} newly exceeds {report['oversized_threshold']} lines")
    return failures


def render_markdown(report: dict[str, Any]) -> str:
    lines = [
        "# MNE-CPP v2.4.0 maintainability baseline",
        "",
        "Generated by `tools/quality/audit_maintainability.py`. Do not edit by hand.",
        "Counts are lexical candidates with comments and string literals removed; they rank work, they do not prove defects.",
        "",
        f"Scanned {report['scanned_files']:,} files / {report['scanned_lines']:,} lines in "
        "`src/libraries`, `src/applications`, `src/tools`. Compiler warnings: 0 (the build is `-Werror`, T6.1).",
        "",
        "| Metric | Total | Files | libraries | applications | tools |",
        "|---|---:|---:|---:|---:|---:|",
    ]
    for name, metric in report["metrics"].items():
        scope = metric["by_scope"]
        lines.append(
            f"| `{name}` | {metric['total']:,} | {metric['files']:,} | {scope.get('libraries', 0):,} "
            f"| {scope.get('applications', 0):,} | {scope.get('tools', 0):,} |"
        )
    for name, metric in report["metrics"].items():
        if not metric["top"]:
            continue
        unit = "lines" if name == "oversized_unit" else "count"
        lines += ["", f"## `{name}` — top files", "", f"| {unit} | File |", "|---:|---|"]
        lines += [f"| {value:,} | `{path}` |" for path, value in metric["top"].items()]
    return "\n".join(lines) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--json", type=Path, dest="json_path", help="write the JSON report")
    parser.add_argument("--markdown", type=Path, help="write the Markdown summary")
    parser.add_argument("--check", type=Path, metavar="BASELINE", help="fail if any metric grew beyond BASELINE")
    args = parser.parse_args(argv)

    report = build_report()
    if args.check:
        failures = check(report, json.loads(args.check.read_text(encoding="utf-8")))
        for failure in failures:
            print(f"ERROR {failure}")
        print("OK: no maintainability metric grew." if not failures else f"{len(failures)} regression(s)")
        return 1 if failures else 0
    if args.json_path:
        args.json_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if args.markdown:
        args.markdown.write_text(render_markdown(report), encoding="utf-8")
    if not args.json_path and not args.markdown:
        print(render_markdown(report), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
