#!/usr/bin/env python3
# =============================================================================================================
#
# @file     validate_test_inventory.py
# @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
# @since    2.4.0
# @date     July, 2026
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#
# @brief    Validate the invariants that keep the test suite honest.
#
# =============================================================================================================
"""Validate the test inventory under ``src/testframes``.

A test that builds but never runs is worse than a missing test: it looks like
coverage in the repository and produces none in CI.  That is not hypothetical.
Thirty-four tests spent months in exactly that state because each of them
overrode ``RUNTIME_OUTPUT_DIRECTORY`` to a path the CI runner did not glob, so
the binaries were compiled, linked, and then never launched.  Nothing failed,
because nothing ran.

This validator encodes the invariants that would have caught it, plus the
neighbouring ones that keep the inventory trustworthy as it grows:

* every ``test_*`` directory is either registered or explicitly excused,
* every registered directory actually calls ``add_test()``,
* nobody re-introduces a per-test output-directory override,
* test names are unique and match their directory,
* labels come from the agreed vocabulary and timeouts stay inside sane bounds,
* ``doc/api_registry.json`` test references resolve to a test that really runs,
* quarantined tests carry an owner, an issue, and an expiry date.

Labels and timeouts are ratcheted rather than demanded outright: the tree does
not carry them yet (that lands with the CTest registration helper), so the
policy records how many tests still lack them and the count is only allowed to
fall.  Unknown labels and out-of-range timeouts are hard errors from day one.

Usage
-----
    python3 tools/quality/validate_test_inventory.py

    # cross-check the static inventory against what CTest would actually run
    ctest --test-dir build --show-only=json-v1 > ctest.json
    python3 tools/quality/validate_test_inventory.py --ctest-json ctest.json

    # machine-readable output for CI annotations
    python3 tools/quality/validate_test_inventory.py --format json

    # after removing the last unlabelled test, tighten the ratchet
    python3 tools/quality/validate_test_inventory.py --update-ratchet

Exit codes
----------
    0   no errors (warnings may still be present)
    1   at least one validation error
    2   the tree or the policy file could not be read
"""

from __future__ import annotations

import argparse
import datetime as _datetime
import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

# --------------------------------------------------------------------------------------------------------
# Repository layout
# --------------------------------------------------------------------------------------------------------

REPO_ROOT = Path(__file__).resolve().parents[2]
TESTFRAMES_DIR = REPO_ROOT / "src" / "testframes"
ROOT_TEST_CMAKE = TESTFRAMES_DIR / "CMakeLists.txt"
API_REGISTRY = REPO_ROOT / "doc" / "api_registry.json"
POLICY_FILE = Path(__file__).resolve().parent / "test_inventory_policy.json"

TEST_DIR_PREFIX = "test_"


# --------------------------------------------------------------------------------------------------------
# CMake parsing
#
# This is deliberately a lexical parse, not an evaluation.  Running CMake would
# require a configured build tree with Qt present, which is exactly the thing a
# lint step must not depend on.  The cost is that generator expressions and
# conditional blocks are read as text; the checks below are written so that
# reading them as text still gives the right answer.
# --------------------------------------------------------------------------------------------------------

_ADD_SUBDIRECTORY_RE = re.compile(r"^[ \t]*add_subdirectory[ \t]*\([ \t]*([A-Za-z0-9_./+-]+)", re.MULTILINE)
_PROJECT_RE = re.compile(r"^[ \t]*project[ \t]*\([ \t]*([A-Za-z0-9_.+-]+)", re.MULTILINE | re.IGNORECASE)
# Commands are routinely spread over several lines, so these must not be anchored
# to a single one.
_ADD_TEST_CALL_RE = re.compile(r"\badd_test\s*\(", re.IGNORECASE)
_ADD_TEST_NAME_RE = re.compile(r"\badd_test\s*\(\s*NAME\s+([^\s)]+)", re.IGNORECASE)
_SET_TESTS_PROPERTIES_RE = re.compile(r"set_tests_properties\s*\((.*?)\)", re.IGNORECASE | re.DOTALL)
_LABELS_RE = re.compile(r"\bLABELS\s+(\"[^\"]*\"|[^\s)]+)", re.IGNORECASE)
_TIMEOUT_RE = re.compile(r"\bTIMEOUT\s+([0-9]+)", re.IGNORECASE)
_OUTPUT_DIR_RE = re.compile(r"\bRUNTIME_OUTPUT_DIRECTORY(?:_[A-Z]+)?\b")


def strip_cmake_comments(text: str) -> str:
    """Remove ``#`` comments while leaving ``#`` inside quoted strings alone."""
    out: list[str] = []
    for line in text.splitlines():
        in_quotes = False
        cut = len(line)
        index = 0
        while index < len(line):
            char = line[index]
            if char == "\\" and in_quotes:
                index += 2
                continue
            if char == '"':
                in_quotes = not in_quotes
            elif char == "#" and not in_quotes:
                cut = index
                break
            index += 1
        out.append(line[:cut])
    return "\n".join(out)


@dataclass
class TestEntry:
    """Everything the validator knows about one leaf test directory."""

    name: str
    directory: str
    registered: bool = False
    has_add_test: bool = False
    add_test_names: list[str] = field(default_factory=list)
    labels: list[str] = field(default_factory=list)
    timeout: int | None = None
    overrides_output_dir: bool = False

    @property
    def cmake_path(self) -> str:
        return f"src/testframes/{self.directory}/CMakeLists.txt"


def parse_registered_directories(root_cmake: Path) -> list[str]:
    text = strip_cmake_comments(root_cmake.read_text(encoding="utf-8"))
    return _ADD_SUBDIRECTORY_RE.findall(text)


def parse_leaf(directory: Path) -> TestEntry:
    entry = TestEntry(name=directory.name, directory=directory.name)
    cmake = directory / "CMakeLists.txt"
    if not cmake.is_file():
        return entry

    text = strip_cmake_comments(cmake.read_text(encoding="utf-8"))

    project_match = _PROJECT_RE.search(text)
    if project_match:
        entry.name = project_match.group(1)

    entry.add_test_names = [n.strip('"') for n in _ADD_TEST_NAME_RE.findall(text)]
    entry.has_add_test = bool(_ADD_TEST_CALL_RE.search(text))
    entry.overrides_output_dir = bool(_OUTPUT_DIR_RE.search(text))

    labels: list[str] = []
    timeout: int | None = None
    for block in _SET_TESTS_PROPERTIES_RE.findall(text):
        label_match = _LABELS_RE.search(block)
        if label_match:
            raw = label_match.group(1).strip('"')
            labels.extend(token for token in re.split(r"[;\s]+", raw) if token)
        timeout_match = _TIMEOUT_RE.search(block)
        if timeout_match:
            value = int(timeout_match.group(1))
            timeout = value if timeout is None else max(timeout, value)

    entry.labels = sorted(set(labels))
    entry.timeout = timeout
    return entry


def collect_inventory(testframes_dir: Path, root_cmake: Path) -> tuple[dict[str, TestEntry], list[str]]:
    """Return the parsed leaf tests plus the directories the root file names but does not have."""
    registered = parse_registered_directories(root_cmake)
    registered_set = set(registered)
    missing_directories = [d for d in registered if not (testframes_dir / d / "CMakeLists.txt").is_file()]

    entries: dict[str, TestEntry] = {}
    for child in sorted(testframes_dir.iterdir()):
        if not child.is_dir() or not child.name.startswith(TEST_DIR_PREFIX):
            continue
        entry = parse_leaf(child)
        entry.registered = child.name in registered_set
        entries[child.name] = entry

    return entries, missing_directories


# --------------------------------------------------------------------------------------------------------
# Findings
# --------------------------------------------------------------------------------------------------------


@dataclass
class Finding:
    code: str
    severity: str  # "error" or "warning"
    location: str
    message: str

    def render(self) -> str:
        return f"{self.severity.upper():7} {self.code}  {self.location}\n         {self.message}"

    def as_dict(self) -> dict[str, str]:
        return {
            "code": self.code,
            "severity": self.severity,
            "location": self.location,
            "message": self.message,
        }


def display_path(path: Path) -> str:
    """Repository-relative when possible, absolute otherwise (fixtures live in a temp dir)."""
    try:
        return path.resolve().relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return path.as_posix()


def error(code: str, location: str, message: str) -> Finding:
    return Finding(code=code, severity="error", location=location, message=message)


def warning(code: str, location: str, message: str) -> Finding:
    return Finding(code=code, severity="warning", location=location, message=message)


# --------------------------------------------------------------------------------------------------------
# Checks
# --------------------------------------------------------------------------------------------------------


def check_registration(
    entries: dict[str, TestEntry],
    missing_directories: list[str],
    policy: dict[str, Any],
) -> list[Finding]:
    """Every test directory is registered, or excused in writing."""
    findings: list[Finding] = []
    excused: dict[str, str] = policy.get("unregistered", {})

    for directory in missing_directories:
        findings.append(
            error(
                "TI001",
                "src/testframes/CMakeLists.txt",
                f"add_subdirectory({directory}) names a directory with no CMakeLists.txt.",
            )
        )

    for name, entry in entries.items():
        if entry.registered:
            if name in excused:
                findings.append(
                    error(
                        "TI002",
                        entry.cmake_path,
                        f"'{name}' is registered but the policy still lists it as deliberately "
                        "unregistered; remove the entry from 'unregistered'.",
                    )
                )
            continue

        reason = excused.get(name)
        if reason:
            continue
        findings.append(
            error(
                "TI003",
                entry.cmake_path,
                f"'{name}' is not added via add_subdirectory() in src/testframes/CMakeLists.txt. "
                "Register it, or record a reason under 'unregistered' in the policy file.",
            )
        )

    for name in excused:
        if name not in entries:
            findings.append(
                error(
                    "TI004",
                    display_path(POLICY_FILE),
                    f"'unregistered' names '{name}', which no longer exists under src/testframes.",
                )
            )

    return findings


def check_add_test(entries: dict[str, TestEntry]) -> list[Finding]:
    """A registered directory that never calls add_test() builds a binary nobody runs."""
    findings: list[Finding] = []
    for name, entry in sorted(entries.items()):
        if not entry.registered:
            continue
        if not entry.has_add_test:
            findings.append(
                error(
                    "TI010",
                    entry.cmake_path,
                    f"'{name}' is registered but never calls add_test(); the binary would be built "
                    "and never executed.",
                )
            )
    return findings


def check_output_directory_overrides(entries: dict[str, TestEntry]) -> list[Finding]:
    """The regression guard for the bug that motivated this validator."""
    findings: list[Finding] = []
    for name, entry in sorted(entries.items()):
        if entry.overrides_output_dir:
            findings.append(
                error(
                    "TI011",
                    entry.cmake_path,
                    f"'{name}' sets RUNTIME_OUTPUT_DIRECTORY. Leaf tests must inherit the location "
                    "set in src/testframes/CMakeLists.txt; overriding it has previously hidden tests "
                    "from CI and from the coverage numbers.",
                )
            )
    return findings


def check_names(entries: dict[str, TestEntry]) -> list[Finding]:
    """Names are unique, and a directory is named after the test it holds."""
    findings: list[Finding] = []
    seen: dict[str, str] = {}
    for directory, entry in sorted(entries.items()):
        if not entry.registered:
            continue
        if entry.name != directory:
            findings.append(
                error(
                    "TI020",
                    entry.cmake_path,
                    f"project() is '{entry.name}' but the directory is '{directory}'; they must match "
                    "so that a test can be found from its name.",
                )
            )
        previous = seen.get(entry.name)
        if previous is not None:
            findings.append(
                error(
                    "TI021",
                    entry.cmake_path,
                    f"test name '{entry.name}' is already used by src/testframes/{previous}.",
                )
            )
        else:
            seen[entry.name] = directory
    return findings


def check_labels(entries: dict[str, TestEntry], policy: dict[str, Any]) -> list[Finding]:
    """Unknown labels are always fatal; missing labels are ratcheted."""
    findings: list[Finding] = []
    allowed = set(policy.get("allowed_labels", []))

    for _, entry in sorted(entries.items()):
        if not entry.registered:
            continue
        unknown = [label for label in entry.labels if label not in allowed]
        if unknown:
            findings.append(
                error(
                    "TI030",
                    entry.cmake_path,
                    f"unknown label(s) {', '.join(sorted(unknown))}. Allowed labels: "
                    f"{', '.join(sorted(allowed))}.",
                )
            )
    return findings


def check_timeouts(entries: dict[str, TestEntry], policy: dict[str, Any]) -> list[Finding]:
    """A timeout outside the agreed band is either useless or a hidden hang."""
    findings: list[Finding] = []
    bounds = policy.get("timeout_seconds", {})
    minimum = int(bounds.get("min", 1))
    maximum = int(bounds.get("max", 3600))

    for _, entry in sorted(entries.items()):
        if not entry.registered or entry.timeout is None:
            continue
        if not (minimum <= entry.timeout <= maximum):
            findings.append(
                error(
                    "TI040",
                    entry.cmake_path,
                    f"TIMEOUT {entry.timeout}s is outside the allowed range "
                    f"{minimum}-{maximum}s. Adjust the test or raise the bound in the policy "
                    "with a reason.",
                )
            )
    return findings


def check_ratchets(entries: dict[str, TestEntry], policy: dict[str, Any]) -> tuple[list[Finding], dict[str, int]]:
    """Missing labels and timeouts may only ever become rarer."""
    findings: list[Finding] = []
    registered = [e for e in entries.values() if e.registered]
    actual = {
        "max_tests_without_labels": sum(1 for e in registered if not e.labels),
        "max_tests_without_timeout": sum(1 for e in registered if e.timeout is None),
    }
    ratchet = policy.get("ratchet", {})
    labels_text = {
        "max_tests_without_labels": "tests without a LABELS property",
        "max_tests_without_timeout": "tests without an explicit TIMEOUT",
    }
    policy_location = display_path(POLICY_FILE)

    for key, count in actual.items():
        allowed = ratchet.get(key)
        if allowed is None:
            findings.append(
                error("TI050", policy_location, f"policy is missing ratchet key '{key}'."))
            continue
        if count > allowed:
            findings.append(
                error(
                    "TI051",
                    policy_location,
                    f"{count} {labels_text[key]}, but the ratchet allows at most {allowed}. "
                    "New tests must carry the metadata; the ratchet only moves down.",
                )
            )
        elif count < allowed:
            findings.append(
                warning(
                    "TI052",
                    policy_location,
                    f"{count} {labels_text[key]}, below the recorded limit of {allowed}. "
                    f"Tighten it: set '{key}' to {count} (or run with --update-ratchet).",
                )
            )

    return findings, actual


def check_registry_references(entries: dict[str, TestEntry], registry_path: Path) -> list[Finding]:
    """A registry entry may only point at a test that is registered and runs."""
    findings: list[Finding] = []
    if not registry_path.is_file():
        return [warning("TI060", display_path(registry_path), "API registry not found; reference check skipped.")]

    try:
        registry = json.loads(registry_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        return [error("TI061", display_path(registry_path), f"invalid JSON: {exc}")]

    runnable = {
        name for name, entry in entries.items() if entry.registered and entry.has_add_test
    }

    referenced: dict[str, int] = {}

    def walk(node: Any) -> None:
        if isinstance(node, dict):
            value = node.get("test")
            if isinstance(value, str) and value:
                referenced[value] = referenced.get(value, 0) + 1
            for child in node.values():
                walk(child)
        elif isinstance(node, list):
            for child in node:
                walk(child)

    walk(registry)

    location = display_path(registry_path)
    for name in sorted(referenced):
        if name in runnable:
            continue
        entry = entries.get(name)
        if entry is None:
            detail = "no such directory under src/testframes."
        elif not entry.registered:
            detail = "the directory exists but is not registered in src/testframes/CMakeLists.txt."
        else:
            detail = "the directory is registered but never calls add_test()."
        findings.append(
            error(
                "TI062",
                location,
                f"{referenced[name]} entr{'y' if referenced[name] == 1 else 'ies'} reference test "
                f"'{name}', which does not resolve to a running test: {detail}",
            )
        )
    return findings


def check_quarantine(entries: dict[str, TestEntry], policy: dict[str, Any], today: _datetime.date) -> list[Finding]:
    """A quarantined test is a debt with a name on it and a date attached."""
    findings: list[Finding] = []
    location = display_path(POLICY_FILE)
    required = ("test", "owner", "issue", "expires", "reason")

    quarantine = policy.get("quarantine", [])
    if not isinstance(quarantine, list):
        return [error("TI070", location, "'quarantine' must be a list.")]

    seen: set[str] = set()
    for index, item in enumerate(quarantine):
        label = f"quarantine[{index}]"
        if not isinstance(item, dict):
            findings.append(error("TI070", location, f"{label} must be an object."))
            continue

        missing = [key for key in required if not item.get(key)]
        if missing:
            findings.append(
                error(
                    "TI071",
                    location,
                    f"{label} is missing required field(s): {', '.join(missing)}. "
                    "A quarantine needs an owner, an issue, and an expiry date.",
                )
            )
            continue

        name = str(item["test"])
        label = f"quarantine entry for '{name}'"

        if name in seen:
            findings.append(error("TI072", location, f"{label} is listed more than once."))
        seen.add(name)

        if name not in entries:
            findings.append(
                error("TI073", location, f"{label} names a test that does not exist under src/testframes.")
            )

        try:
            expires = _datetime.date.fromisoformat(str(item["expires"]))
        except ValueError:
            findings.append(
                error("TI074", location, f"{label} has expiry '{item['expires']}'; expected YYYY-MM-DD.")
            )
            continue

        if expires < today:
            findings.append(
                error(
                    "TI075",
                    location,
                    f"{label} expired on {expires.isoformat()}. Fix the test or renegotiate the "
                    f"quarantine with {item['owner']} ({item['issue']}).",
                )
            )
        elif (expires - today).days <= 14:
            findings.append(
                warning(
                    "TI076",
                    location,
                    f"{label} expires on {expires.isoformat()}, in {(expires - today).days} day(s).",
                )
            )

    return findings


def check_ctest_json(entries: dict[str, TestEntry], ctest_json: Path) -> list[Finding]:
    """Cross-check the static inventory against what CTest would really run."""
    findings: list[Finding] = []
    try:
        data = json.loads(ctest_json.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return [error("TI080", display_path(ctest_json), f"could not read CTest inventory: {exc}")]

    location = display_path(ctest_json)
    ctest_names = {test.get("name") for test in data.get("tests", []) if test.get("name")}
    static_names = {name for name, entry in entries.items() if entry.registered and entry.has_add_test}

    for name in sorted(static_names - ctest_names):
        findings.append(
            error(
                "TI081",
                f"src/testframes/{name}/CMakeLists.txt",
                f"'{name}' registers a test but does not appear in the configured CTest inventory. "
                "It is most likely disabled by a condition that no longer holds.",
            )
        )

    for name in sorted(ctest_names - static_names):
        findings.append(
            warning(
                "TI082",
                location,
                f"CTest knows test '{name}', which the static inventory did not find under "
                "src/testframes.",
            )
        )

    return findings


# --------------------------------------------------------------------------------------------------------
# Policy handling
# --------------------------------------------------------------------------------------------------------


def load_policy(path: Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        print(f"error: policy file not found: {path}", file=sys.stderr)
        raise SystemExit(2)
    except json.JSONDecodeError as exc:
        print(f"error: policy file is not valid JSON: {path}: {exc}", file=sys.stderr)
        raise SystemExit(2)


def update_ratchet(path: Path, policy: dict[str, Any], actual: dict[str, int]) -> bool:
    ratchet = policy.setdefault("ratchet", {})
    changed = False
    for key, count in actual.items():
        if ratchet.get(key) != count:
            ratchet[key] = count
            changed = True
    if changed:
        path.write_text(json.dumps(policy, indent=2) + "\n", encoding="utf-8")
    return changed


# --------------------------------------------------------------------------------------------------------
# Entry point
# --------------------------------------------------------------------------------------------------------


def run(args: argparse.Namespace) -> int:
    testframes_dir = Path(args.testframes).resolve()
    root_cmake = testframes_dir / "CMakeLists.txt"
    if not root_cmake.is_file():
        print(f"error: no CMakeLists.txt in {testframes_dir}", file=sys.stderr)
        return 2

    policy = load_policy(Path(args.policy).resolve())
    entries, missing_directories = collect_inventory(testframes_dir, root_cmake)

    findings: list[Finding] = []
    findings += check_registration(entries, missing_directories, policy)
    findings += check_add_test(entries)
    findings += check_output_directory_overrides(entries)
    findings += check_names(entries)
    findings += check_labels(entries, policy)
    findings += check_timeouts(entries, policy)

    ratchet_findings, actual = check_ratchets(entries, policy)
    findings += ratchet_findings

    findings += check_registry_references(entries, Path(args.api_registry).resolve())
    findings += check_quarantine(entries, policy, _datetime.date.today())

    if args.ctest_json:
        findings += check_ctest_json(entries, Path(args.ctest_json).resolve())

    if args.update_ratchet:
        if update_ratchet(Path(args.policy).resolve(), policy, actual):
            print(f"ratchet updated: {actual}")
        else:
            print("ratchet already tight")
        findings = [f for f in findings if f.code not in {"TI051", "TI052"}]

    errors = [f for f in findings if f.severity == "error"]
    warnings = [f for f in findings if f.severity == "warning"]
    registered = sum(1 for e in entries.values() if e.registered)

    if args.format == "json":
        print(
            json.dumps(
                {
                    "summary": {
                        "directories": len(entries),
                        "registered": registered,
                        "runnable": sum(1 for e in entries.values() if e.registered and e.has_add_test),
                        "without_labels": actual["max_tests_without_labels"],
                        "without_timeout": actual["max_tests_without_timeout"],
                        "errors": len(errors),
                        "warnings": len(warnings),
                    },
                    "findings": [f.as_dict() for f in findings],
                },
                indent=2,
            )
        )
        return 1 if errors else 0

    print("Test inventory validation")
    print(f"  test directories .............. {len(entries)}")
    print(f"  registered .................... {registered}")
    print(f"  registered and calling add_test {sum(1 for e in entries.values() if e.registered and e.has_add_test)}")
    print(f"  without labels ................ {actual['max_tests_without_labels']}")
    print(f"  without explicit timeout ...... {actual['max_tests_without_timeout']}")
    print()

    if not findings:
        print("OK: no findings.")
        return 0

    for finding in sorted(findings, key=lambda f: (f.severity != "error", f.code, f.location)):
        print(finding.render())
        print()

    print(f"{len(errors)} error(s), {len(warnings)} warning(s)")
    return 1 if errors else 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Validate the invariants of the MNE-CPP test inventory.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--testframes",
        default=str(TESTFRAMES_DIR),
        help="path to src/testframes (default: %(default)s)",
    )
    parser.add_argument(
        "--policy",
        default=str(POLICY_FILE),
        help="path to the inventory policy file (default: %(default)s)",
    )
    parser.add_argument(
        "--api-registry",
        default=str(API_REGISTRY),
        help="path to doc/api_registry.json (default: %(default)s)",
    )
    parser.add_argument(
        "--ctest-json",
        help=(
            "output of 'ctest --show-only=json-v1' to cross-check the static inventory against. "
            "Not yet suitable for CI: some tests are registered behind CMake conditions that this "
            "lexical parser cannot evaluate, so absences are reported that are in fact intentional"
        ),
    )
    parser.add_argument(
        "--format",
        choices=("text", "json"),
        default="text",
        help="output format (default: %(default)s)",
    )
    parser.add_argument(
        "--update-ratchet",
        action="store_true",
        help="rewrite the ratchet counters in the policy file to the current values",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    return run(build_parser().parse_args(argv))


if __name__ == "__main__":
    raise SystemExit(main())
