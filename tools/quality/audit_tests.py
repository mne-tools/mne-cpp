#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     September, 2026
#
#
# @brief    Inventory every test: registration, metadata, requirements, and CI execution.
"""Produce the v2.4.0 test inventory (package T0.1).

``validate_test_inventory.py`` answers "is anything wrong?".  This tool answers
"what is there?": one record per ``src/testframes/test_*`` directory with how it
is registered, which CMake conditions can remove it, its labels and timeout,
what it needs at run time, whether CI excludes it on a platform, and whether it
actually ran in the pinned CI evidence.

CI execution is read from ``doc/release/v2.4.0/ci-test-results.json``, a compact
record imported once from the JUnit artifacts of a named workflow run.  Keeping
that record in the tree makes the inventory reproducible offline and byte-stable:
the same checkout always produces the same report.

Usage
-----
    # regenerate the committed report
    python3 tools/quality/audit_tests.py --json doc/release/v2.4.0/test-inventory.json \\
        --markdown doc/release/v2.4.0/test-inventory.md

    # refresh the pinned CI evidence from a workflow run's JUnit artifacts
    gh run download <run-id> --repo mne-tools/mne-cpp -p 'test-results-*' -D /tmp/junit
    python3 tools/quality/audit_tests.py \\
        --import-junit ubuntu-24.04=/tmp/junit/test-results-ubuntu-24.04/test-results.xml \\
        --import-junit macos-26=/tmp/junit/test-results-macos-26/test-results.xml \\
        --import-junit windows-2025-vs2026=/tmp/junit/test-results-windows-2025-vs2026/test-results.xml \\
        --run-url https://github.com/mne-tools/mne-cpp/actions/runs/<run-id> --commit <sha>
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))
import validate_test_inventory as vti  # noqa: E402

REPO_ROOT = vti.REPO_ROOT
TESTFRAMES_DIR = vti.TESTFRAMES_DIR
CI_EVIDENCE = REPO_ROOT / "doc" / "release" / "v2.4.0" / "ci-test-results.json"
TEST_WORKFLOW = REPO_ROOT / ".github" / "workflows" / "_reusable-tests.yml"
RETRY_SCAN_GLOBS = ("scripts/test/*", ".github/workflows/*.yml")

_IF_RE = re.compile(r"^\s*(if|elseif)\s*\((.*)\)\s*$", re.IGNORECASE)
_ELSE_RE = re.compile(r"^\s*else\s*\(.*\)\s*$", re.IGNORECASE)
_ENDIF_RE = re.compile(r"^\s*endif\s*\(.*\)\s*$", re.IGNORECASE)
_SUBDIR_RE = re.compile(r"^\s*add_subdirectory\s*\(\s*([A-Za-z0-9_./+-]+)", re.IGNORECASE)
_EXCLUDE_RE = re.compile(r"(?:^|\s)-E\s+\"([^\"]+)\"")
_OS_CONDITION_RE = re.compile(r"matrix\.os\s*==\s*'([^']+)'")
_RETRY_RE = re.compile(r"re-?run(?:ning)? failed|--repeat\s+until-pass|--rerun-failed|passed on retry", re.IGNORECASE)

# Lexical signals in a test's sources.  They classify, they do not prove; each
# record lists the signals so a reviewer can check the classification.
_REQUIREMENT_SIGNALS = {
    "test-data": re.compile(r"mne-cpp-test-data"),
    "sample-data": re.compile(r"MNE_DATA|mne_data|MNE-sample-data"),
    "python": re.compile(r"PythonRunner|MNE_REQUIRE_PYTHON|\"python3?\"|import mne"),
    "freesurfer": re.compile(r"FREESURFER_HOME|mri_convert|mri_watershed"),
}
_GUI_RE = re.compile(r"\bQTEST_MAIN\s*\(")
_QSKIP_RE = re.compile(r"\bQSKIP\s*\(")
_PROCESS_RE = re.compile(r"\bQProcess\b")


def conditional_subdirectories(cmake: Path) -> dict[str, list[str]]:
    """Map each add_subdirectory() target to the if() conditions enclosing it."""
    result: dict[str, list[str]] = {}
    stack: list[str] = []
    for line in vti.strip_cmake_comments(cmake.read_text(encoding="utf-8")).splitlines():
        if match := _IF_RE.match(line):
            condition = " ".join(match.group(2).split())
            if match.group(1).lower() == "elseif":
                previous = stack.pop() if stack else ""
                condition = f"NOT ({previous}) AND ({condition})"
            stack.append(condition)
        elif _ELSE_RE.match(line):
            previous = stack.pop() if stack else ""
            stack.append(f"NOT ({previous})")
        elif _ENDIF_RE.match(line):
            if stack:
                stack.pop()
        elif match := _SUBDIR_RE.match(line):
            result[match.group(1)] = list(stack)
    return result


def platform_exclusions(workflow: Path) -> dict[str, list[str]]:
    """Map test name to the CI platforms whose ctest invocation excludes it with -E."""
    excluded: dict[str, list[str]] = {}
    if not workflow.is_file():
        return excluded
    for step in re.split(r"\n\s*- name:", workflow.read_text(encoding="utf-8")):
        os_match = _OS_CONDITION_RE.search(step)
        for pattern in _EXCLUDE_RE.findall(step):
            for name in pattern.split("|"):
                excluded.setdefault(name.strip(), []).append(os_match.group(1) if os_match else "all")
    return {name: sorted(set(platforms)) for name, platforms in excluded.items()}


def retry_paths(repo_root: Path) -> list[dict[str, Any]]:
    """Places that turn a failed test into a pass by running it again."""
    hits: list[dict[str, Any]] = []
    for pattern in RETRY_SCAN_GLOBS:
        for path in sorted(repo_root.glob(pattern)):
            if not path.is_file():
                continue
            for number, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
                if _RETRY_RE.search(line):
                    hits.append({"path": path.relative_to(repo_root).as_posix(), "line": number,
                                 "text": line.strip()[:120]})
    return hits


def source_signals(directory: Path) -> dict[str, Any]:
    text = "\n".join(
        path.read_text(encoding="utf-8", errors="replace")
        for path in sorted(directory.rglob("*"))
        if path.suffix in {".cpp", ".h", ".hpp"}
    )
    return {
        "requirements": sorted(name for name, regex in _REQUIREMENT_SIGNALS.items() if regex.search(text)),
        "gui": bool(_GUI_RE.search(text)),
        "qskip_sites": len(_QSKIP_RE.findall(text)),
        "spawns_processes": bool(_PROCESS_RE.search(text)),
    }


def parse_junit(path: Path) -> dict[str, Any]:
    root = ET.parse(path).getroot()
    results: dict[str, dict[str, Any]] = {}
    for case in root.iter("testcase"):
        name = case.get("name") or ""
        status = case.get("status") or "run"
        if case.find("failure") is not None or case.find("error") is not None:
            status = "fail"
        elif case.find("skipped") is not None:
            status = "skipped"
        results[name] = {"status": status, "seconds": round(float(case.get("time") or 0.0), 1)}
    statuses = [value["status"] for value in results.values()]
    return {
        "tests": len(results),
        "failures": statuses.count("fail"),
        "skipped": sum(1 for s in statuses if s not in {"run", "fail"}),
        "results": dict(sorted(results.items())),
    }


def import_junit(sources: list[str], run_url: str | None, commit: str | None) -> dict[str, Any]:
    platforms: dict[str, Any] = {}
    for item in sources:
        platform, _, path = item.partition("=")
        if not platform or not path:
            raise ValueError(f"--import-junit expects PLATFORM=PATH, got '{item}'")
        platforms[platform] = parse_junit(Path(path))
    return {
        "schema_version": 1,
        "source": {"run_url": run_url, "commit": commit},
        "platforms": dict(sorted(platforms.items())),
    }


def load_ci_evidence(path: Path) -> dict[str, Any] | None:
    return json.loads(path.read_text(encoding="utf-8")) if path.is_file() else None


def registry_references(registry_path: Path) -> dict[str, list[str]]:
    if not registry_path.is_file():
        return {}
    registry = json.loads(registry_path.read_text(encoding="utf-8"))
    references: dict[str, list[str]] = {}
    for record in registry.get("classes", []):
        if record.get("test"):
            references.setdefault(record["test"], []).append(record.get("name", ""))
    return {name: sorted(classes) for name, classes in references.items()}


def build_inventory(repo_root: Path = REPO_ROOT, ci_evidence: dict[str, Any] | None = None) -> dict[str, Any]:
    testframes = repo_root / "src" / "testframes"
    policy = vti.load_policy(vti.POLICY_FILE)
    entries, missing_directories = vti.collect_inventory(testframes, testframes / "CMakeLists.txt")

    leaf_conditions = conditional_subdirectories(testframes / "CMakeLists.txt")
    root_conditions = {
        Path(target).name: conditions
        for target, conditions in conditional_subdirectories(repo_root / "CMakeLists.txt").items()
        if target.startswith("src/testframes/")
    }
    excluded = platform_exclusions(repo_root / ".github" / "workflows" / "_reusable-tests.yml")
    references = registry_references(repo_root / "doc" / "api_registry.json")
    platforms = (ci_evidence or {}).get("platforms", {})

    records: list[dict[str, Any]] = []
    for name, entry in sorted(entries.items()):
        if entry.registered:
            via, conditions = "src/testframes/CMakeLists.txt", leaf_conditions.get(name, [])
        elif name in root_conditions:
            via, conditions = "CMakeLists.txt", root_conditions[name]
        else:
            via, conditions = None, []
        guard = policy.get("conditional", {}).get(name) if entry.guarded_registration else None
        if via is None:
            state = "unregistered"
        elif conditions or guard:
            state = "conditional"
        else:
            state = "unconditional"

        ci = {
            platform: data["results"][name]["status"] if name in data["results"] else "absent"
            for platform, data in sorted(platforms.items())
        }
        records.append({
            "name": name,
            "cmake": entry.cmake_path,
            "registration": {
                "state": state,
                "via": via,
                "conditions": conditions,
                "early_return_guard": guard,
                "unregistered_reason": policy.get("unregistered", {}).get(name) if via is None else None,
            },
            "add_test_names": [n.replace("${PROJECT_NAME}", entry.name) for n in entry.add_test_names],
            "labels": entry.labels,
            "timeout": entry.timeout,
            **source_signals(testframes / entry.directory),
            "ci_excluded_on": excluded.get(name, []),
            "ci": ci,
            "registry_classes": references.get(name, []),
        })

    add_test_owner: dict[str, list[str]] = {}
    for record in records:
        for test_name in record["add_test_names"]:
            add_test_owner.setdefault(test_name, []).append(record["name"])
    duplicates = {name: owners for name, owners in sorted(add_test_owner.items()) if len(owners) > 1}

    known = {record["name"] for record in records}
    summary: dict[str, Any] = {
        "directories": len(records),
        "registration": {
            state: sum(1 for r in records if r["registration"]["state"] == state)
            for state in ("unconditional", "conditional", "unregistered")
        },
        "missing_directories": missing_directories,
        "duplicate_test_names": duplicates,
        "without_labels": sum(1 for r in records if r["registration"]["via"] and not r["labels"]),
        "without_timeout": sum(1 for r in records if r["registration"]["via"] and r["timeout"] is None),
        "requirements": {
            key: sum(1 for r in records if key in r["requirements"]) for key in sorted(_REQUIREMENT_SIGNALS)
        },
        "gui": sum(1 for r in records if r["gui"]),
        "with_qskip": sum(1 for r in records if r["qskip_sites"]),
        "ci_excluded": {r["name"]: r["ci_excluded_on"] for r in records if r["ci_excluded_on"]},
        "retry_paths": retry_paths(repo_root),
        "ci": {},
    }
    for platform, data in sorted(platforms.items()):
        registered = [r for r in records if r["registration"]["via"]]
        summary["ci"][platform] = {
            "executed": data["tests"],
            "failures": data["failures"],
            "skipped": data["skipped"],
            "registered_not_executed": sorted(r["name"] for r in registered if r["ci"][platform] == "absent"),
            "executed_not_in_inventory": sorted(set(data["results"]) - known),
        }

    return {
        "schema_version": 1,
        "generated_by": "tools/quality/audit_tests.py",
        "ci_source": (ci_evidence or {}).get("source"),
        "summary": summary,
        "tests": records,
    }


def render_markdown(report: dict[str, Any]) -> str:
    summary = report["summary"]
    registration = summary["registration"]
    source = report["ci_source"] or {}
    lines = [
        "# MNE-CPP v2.4.0 test inventory",
        "",
        "Generated by `tools/quality/audit_tests.py`. Do not edit by hand.",
        "",
        f"CI evidence: {source.get('run_url') or 'none recorded'} (commit `{source.get('commit') or 'n/a'}`)",
        "",
        "## Registration",
        "",
        f"- Test directories: {summary['directories']}",
        f"- Registered unconditionally: {registration['unconditional']}",
        f"- Registered behind a CMake condition or early `return()`: {registration['conditional']}",
        f"- Not registered: {registration['unregistered']}",
        f"- Registered without `LABELS`: {summary['without_labels']}",
        f"- Registered without explicit `TIMEOUT`: {summary['without_timeout']}",
        f"- Duplicate `add_test` names: {len(summary['duplicate_test_names'])}",
        "",
        "## CI execution",
        "",
        "| Platform | Executed | Failures | Skipped | Registered but not executed |",
        "|---|---:|---:|---:|---|",
    ]
    for platform, data in summary["ci"].items():
        missing = ", ".join(f"`{n}`" for n in data["registered_not_executed"]) or "-"
        lines.append(f"| {platform} | {data['executed']} | {data['failures']} | {data['skipped']} | {missing} |")
    lines += [
        "",
        "## Run-time requirements (lexical signals)",
        "",
        "| Requirement | Tests |",
        "|---|---:|",
    ]
    lines += [f"| {key} | {count} |" for key, count in summary["requirements"].items()]
    lines += [
        f"| GUI (`QTEST_MAIN`) | {summary['gui']} |",
        f"| Contains `QSKIP` | {summary['with_qskip']} |",
        "",
        "## Conditional and unregistered tests",
        "",
        "| Test | State | Condition |",
        "|---|---|---|",
    ]
    for record in report["tests"]:
        registration_info = record["registration"]
        if registration_info["state"] == "unconditional":
            continue
        reason = (
            registration_info["unregistered_reason"]
            or registration_info["early_return_guard"]
            or " AND ".join(registration_info["conditions"])
        )
        lines.append(f"| `{record['name']}` | {registration_info['state']} | {reason} |")
    lines += ["", "## Platform exclusions in CI", ""]
    lines += [f"- `{name}`: {', '.join(platforms)}" for name, platforms in summary["ci_excluded"].items()] or ["- none"]
    lines += ["", "## Retry paths that can turn a failure into a pass", ""]
    lines += [f"- `{hit['path']}:{hit['line']}` {hit['text']}" for hit in summary["retry_paths"]] or ["- none"]
    return "\n".join(lines) + "\n"


def dump_json(data: dict[str, Any]) -> str:
    return json.dumps(data, indent=2, sort_keys=False) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--json", type=Path, dest="json_path", help="write the JSON inventory")
    parser.add_argument("--markdown", type=Path, help="write the Markdown summary")
    parser.add_argument("--ci-evidence", type=Path, default=CI_EVIDENCE, help="pinned CI results (default: %(default)s)")
    parser.add_argument("--import-junit", action="append", default=[], metavar="PLATFORM=PATH",
                        help="import JUnit results into --ci-evidence instead of auditing")
    parser.add_argument("--run-url", help="workflow run the imported JUnit files came from")
    parser.add_argument("--commit", help="commit the imported JUnit files were produced from")
    args = parser.parse_args(argv)

    if args.import_junit:
        evidence = import_junit(args.import_junit, args.run_url, args.commit)
        args.ci_evidence.parent.mkdir(parents=True, exist_ok=True)
        args.ci_evidence.write_text(dump_json(evidence), encoding="utf-8")
        print(f"wrote {args.ci_evidence} ({', '.join(evidence['platforms'])})")
        return 0

    report = build_inventory(REPO_ROOT, load_ci_evidence(args.ci_evidence))
    if args.json_path:
        args.json_path.write_text(dump_json(report), encoding="utf-8")
    if args.markdown:
        args.markdown.write_text(render_markdown(report), encoding="utf-8")
    if not args.json_path and not args.markdown:
        print(render_markdown(report), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
