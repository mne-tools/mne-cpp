#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     September, 2026
#
#
# @brief    Map every MNE-Python parity claim to the evidence behind it.
"""Parity evidence baseline for gate G6 (package T0.6).

``tools/parity/gap_analysis.py`` answers *what* MNE-CPP claims relative to
MNE-Python.  This audit answers *how well each claim is supported*:

* every registry class naming an MNE-Python equivalent, and every ``parity``
  record marked implemented or partial, is mapped to an evidence level:

  - ``cross-validated-static``  a registered test embeds reference values
                                taken from MNE-Python (deterministic, no live Python)
  - ``cross-validated-live``    a registered test runs MNE-Python at test time
                                (unpinned: depends on whatever ``mne`` is installed)
  - ``tested``                  a registered test exists but does not compare against MNE-Python
  - ``unverified``              nothing in the tree backs the claim

* the reference environment: what the registry pins, what CI installs, and
  whether both equal ``tools/parity/reference_lock.json``,
* the claims the registry marks ``equivalent`` or ``exceeds`` (field
  ``equivalence``), each with the comparison thresholds its test asserts,
* unsupported aggregate parity percentages published in the documentation.

Usage
-----
    python3 tools/quality/audit_parity_baseline.py
    python3 tools/quality/audit_parity_baseline.py --json doc/release/v2.4.0/parity-baseline.json \\
        --markdown doc/release/v2.4.0/parity-baseline.md
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[2]
TESTFRAMES_DIR = REPO_ROOT / "src" / "testframes"

# Live: the test launches an interpreter itself.  Static: it states that its expected values came from
# MNE-Python.  "Inspired by mne-python" or an MNA record whose interpreter field reads "python3" is neither.
_LIVE_RE = re.compile(r"PythonRunner|PythonTestHelper|MNE_REQUIRE_PYTHON|generate_\w+\.py")
_PROCESS_RE = re.compile(r"\bQProcess\b")
_PYTHON_RE = re.compile(r"\bpython3?\b|import mne", re.IGNORECASE)
_STATIC_RE = re.compile(
    r"(?:reference|expected)[^\n]{0,60}(?:mne-python|mne\.[a-z_]+)"
    r"|(?:mne-python|mne\.[a-z_.]+\()[^\n]{0,40}(?:reference|produced|generated|gives|returns|reports)"
    r"|values?\s+(?:were\s+|are\s+)?(?:produced|generated|computed|taken|obtained)\s+(?:with|by|from|using)[^\n]{0,20}(?:mne|python)"
    r"|against\s+mne-python|(?:read|checked|come|comes)\s+(?:by|from|against)\s+mne[-.]",
    re.IGNORECASE,
)
_NUM = r"(\d+(?:\.\d+)?(?:[eE][-+]?\d+)?)"
# Comparison thresholds: "x < 1e-6)", isApprox(y, 1e-9), tol = 1e-12, kTolerance = 1e-6
_TOLERANCE_RE = re.compile(
    r"<=?\s*" + _NUM + r"\s*[)*]|isApprox\([^,()]+,\s*" + _NUM + r"|\b\w*[tT]ol\w*\s*=\s*" + _NUM)
_PIP_RE = re.compile(r"pip install\s+([^\n]+)")
_PYTHON_VERSION_RE = re.compile(r"python-version:\s*'?([0-9.]+)'?")
# Aggregate claims such as "~92 % of MNE-Python" or "parity: 63.1%".
_PERCENT_CLAIM_RE = re.compile(
    r"[~≈]?\s*\d{1,3}(?:\.\d+)?\s?%[^\n|]{0,60}(?:MNE-Python|mne-python|parity)"
    r"|(?:parity|MNE-Python)[^\n|]{0,60}?[~≈]?\s*\d{1,3}(?:\.\d+)?\s?%",
    re.IGNORECASE,
)
CLAIM_SCAN = ("README.md", "doc/website/docs/**/*.md*", "doc/website/src/**/*.tsx", "doc/dev-notes/*.md")
CLAIM_SKIP = ("doc/website/docs/api/", "doc/dev-notes/v2.2.0-requirements.md", "doc/dev-notes/v2.3.0-requirements.md",
              "doc/dev-notes/v2.4.0-requirements.md")


def classify_text(text: str) -> str:
    """How the sources of one test directory relate to MNE-Python."""
    live = _LIVE_RE.search(text) or any(
        _PROCESS_RE.search(block) and re.search(r"\bpython3?\b\"|import mne", block)
        for block in re.split(r"\n\s*\n", text)
    )
    if live:
        return "cross-validated-live"
    if _STATIC_RE.search(text):
        return "cross-validated-static"
    return "tested"


def test_evidence(testframes: Path) -> dict[str, str]:
    """Classify each test directory by how it relates to MNE-Python."""
    return {directory.name: classify_text(test_text(testframes, directory.name))
            for directory in sorted(p for p in testframes.iterdir() if p.is_dir())}


def test_text(testframes: Path, name: str) -> str:
    directory = testframes / name
    if not directory.is_dir():
        return ""
    return "\n".join(path.read_text(encoding="utf-8", errors="replace")
                     for path in sorted(directory.rglob("*")) if path.suffix in {".cpp", ".h", ".py"})


def test_tolerances(text: str) -> list[float]:
    """Comparison thresholds asserted in a test, smallest first; empty means exact comparisons only."""
    return sorted({float(v) for m in _TOLERANCE_RE.finditer(text) for v in m.groups() if v and 0 < float(v) < 1})


def tolerance_text(tolerances: list[float]) -> str:
    if not tolerances:
        return "exact"
    low, high = tolerances[0], tolerances[-1]
    return f"{low:g}" if low == high else f"{low:g} to {high:g}"


def names_api(text: str, python: str) -> bool:
    """Whether @p text names one of the APIs of a registry ``python`` field ("mne.a.b / c (note)")."""
    for part in re.sub(r"\([^)]*\)", " ", python).split("/"):
        tokens = part.split()
        if tokens and re.search(r"\b" + re.escape(tokens[0].split(".")[-1]) + r"\b", text):
            return True
    return False


def registered_tests(testframes: Path) -> set[str]:
    cmake = (testframes / "CMakeLists.txt").read_text(encoding="utf-8")
    return set(re.findall(r"^\s*add_subdirectory\s*\(\s*(test_\w+)", cmake, re.MULTILINE))


def reference_environment(repo_root: Path, registry: dict[str, Any]) -> dict[str, Any]:
    workflow = repo_root / ".github" / "workflows" / "_reusable-tests.yml"
    text = workflow.read_text(encoding="utf-8") if workflow.is_file() else ""
    installs = [" ".join(line.split()) for line in _PIP_RE.findall(text) if "pip" not in line.split()[:1]]
    packages = sorted({token for line in installs for token in line.split() if not token.startswith("-")})
    unpinned = [p for p in packages if not re.search(r"[=<>~]", p)]
    lock_path = repo_root / "tools" / "parity" / "reference_lock.json"
    lock = json.loads(lock_path.read_text(encoding="utf-8")) if lock_path.is_file() else {"packages": {}}
    pins = dict(p.split("==", 1) for p in packages if "==" in p)
    lock_mismatch = sorted(f"{name}: lock {version}, CI {pins.get(name, 'not installed')}"
                           for name, version in lock["packages"].items() if pins.get(name) != version)
    if lock.get("python") and lock["python"] not in _PYTHON_VERSION_RE.findall(text):
        lock_mismatch.append(f"python: lock {lock['python']}, CI {', '.join(sorted(set(_PYTHON_VERSION_RE.findall(text))))}")
    return {
        "registry_mne_python_ref": registry.get("meta", {}).get("mne_python_ref"),
        "registry_mne_python_pinned": registry.get("parity", {}).get("mne_python_pinned"),
        "ci_python_versions": sorted(set(_PYTHON_VERSION_RE.findall(text))),
        "ci_pip_packages": packages,
        "ci_unpinned_packages": unpinned,
        "lock_file": lock_path.relative_to(repo_root).as_posix() if lock_path.is_file() else None,
        "lock_mismatch": lock_mismatch,
    }


def percent_claims(repo_root: Path) -> list[dict[str, Any]]:
    claims: list[dict[str, Any]] = []
    seen: set[Path] = set()
    for pattern in CLAIM_SCAN:
        for path in sorted(repo_root.glob(pattern)):
            relative = path.relative_to(repo_root).as_posix()
            if path in seen or not path.is_file() or any(relative.startswith(skip) for skip in CLAIM_SKIP):
                continue
            seen.add(path)
            for number, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
                for match in _PERCENT_CLAIM_RE.finditer(line):
                    generated = "Machine-rendered" in line
                    claims.append({
                        "path": relative, "line": number, "text": match.group(0).strip(),
                        "status": "generated-with-denominator" if generated else "unsupported",
                    })
    return claims


def build_report(repo_root: Path = REPO_ROOT) -> dict[str, Any]:
    registry = json.loads((repo_root / "doc" / "api_registry.json").read_text(encoding="utf-8"))
    testframes = repo_root / "src" / "testframes"
    evidence_by_test = test_evidence(testframes)
    registered = registered_tests(testframes)

    texts: dict[str, str] = {}

    def level_of(test: str | None, python: str) -> str:
        if test not in registered:
            return "unverified"
        level = evidence_by_test.get(test, "unverified")
        # A test that compares with MNE-Python is evidence only for the APIs it names
        if level.startswith("cross-validated"):
            text = texts.setdefault(test, test_text(testframes, test))
            if not names_api(text, python):
                return "tested"
        return level

    def claim(source: str, mne_cpp: str | None, python: str, status: str, record: dict[str, Any]) -> dict[str, Any]:
        test = record.get("test")
        level = level_of(test, python)
        entry = {"source": source, "mne_cpp": mne_cpp, "python": python, "claimed": status,
                 "equivalence": record.get("equivalence"), "test": test, "evidence": level}
        if level.startswith("cross-validated"):
            entry["tolerance"] = tolerance_text(test_tolerances(texts.setdefault(test, test_text(testframes, test))))
        return entry

    claims: list[dict[str, Any]] = []
    for record in registry.get("classes", []):
        python = record.get("python_equiv") or record.get("mne_python")
        if python:
            claims.append(claim("classes", record["name"], python, "implemented", record))
    for record in registry.get("parity", {}).get("records", []):
        if record.get("status") in {"implemented", "partial"}:
            claims.append(claim("parity", record.get("mne_cpp") or None, record["python"], record["status"], record))

    levels = ("cross-validated-static", "cross-validated-live", "tested", "unverified")
    by_level = {level: sum(1 for c in claims if c["evidence"] == level) for level in levels}
    by_claim = {
        "equivalent": sum(1 for c in claims if c["equivalence"] == "equivalent"),
        "exceeds": sum(1 for c in claims if c["equivalence"] == "exceeds"),
        "implemented": sum(1 for c in claims if not c["equivalence"] and c["claimed"] == "implemented"),
        "partial": sum(1 for c in claims if c["claimed"] == "partial"),
    }
    unsupported = sorted(f"{c['mne_cpp']} ({c['python']})" for c in claims
                         if c["equivalence"] and c["evidence"] != "cross-validated-static")
    return {
        "schema_version": 1,
        "generated_by": "tools/quality/audit_parity_baseline.py",
        "reference_environment": reference_environment(repo_root, registry),
        "summary": {
            "claims": len(claims),
            "by_evidence": by_level,
            "by_claim": by_claim,
            "equivalence_without_static_evidence": unsupported,
            "by_source": {
                source: {level: sum(1 for c in claims if c["source"] == source and c["evidence"] == level)
                         for level in levels}
                for source in ("classes", "parity")
            },
            "tests_by_relation": {
                level: sorted(name for name, value in evidence_by_test.items()
                              if value == level and name in registered)
                for level in ("cross-validated-static", "cross-validated-live")
            },
        },
        "denominator": {
            "note": "gap_analysis.py inventories 14 public MNE-Python namespaces; mne.viz, mne.report "
                    "internals, mne.utils, mne.datasets and mne.gui are excluded by design, so any "
                    "percentage is 'of the inventoried namespaces', not 'of MNE-Python'.",
            "source": "doc/release/v2.4.0/mne-python-gap.json",
        },
        "percent_claims": percent_claims(repo_root),
        "claims": claims,
    }


def render_markdown(report: dict[str, Any]) -> str:
    env = report["reference_environment"]
    s = report["summary"]
    lock = f"`{env['lock_file']}`" if env["lock_file"] else "**none**"
    lines = [
        "# MNE-CPP v2.4.0 parity evidence baseline",
        "",
        "Generated by `tools/quality/audit_parity_baseline.py`. Do not edit by hand.",
        "",
        "## Reference environment",
        "",
        f"- Registry reference: MNE-Python `{env['registry_mne_python_ref']}` "
        f"(pinned minor `{env['registry_mne_python_pinned']}`)",
        f"- CI test Python: {', '.join(env['ci_python_versions']) or 'unspecified'}",
        f"- CI installs: {', '.join(f'`{p}`' for p in env['ci_pip_packages']) or 'nothing'}",
        f"- Unpinned in CI: **{', '.join(f'`{p}`' for p in env['ci_unpinned_packages']) or 'none'}**",
        f"- Lock file for fixture generation: {lock}"
        + (f" (**differs from CI:** {'; '.join(env['lock_mismatch'])})" if env["lock_mismatch"] else " (equal to CI)"),
        "",
        "## Claimed parity",
        "",
        "`equivalent` and `exceeds` are claimed in the registry (field `equivalence`) and require a test that",
        "compares with embedded MNE-Python reference values; `implemented` means the feature exists and is",
        "tested, not that its output was compared with MNE-Python.",
        "",
        "| Claim | Count |",
        "|---|---:|",
        *(f"| {name} | {count} |" for name, count in s["by_claim"].items()),
        "",
        "Equivalence claims without static cross-validation: "
        + (", ".join(s["equivalence_without_static_evidence"]) or "none"),
        "",
        "## Evidence behind parity claims",
        "",
        "| Evidence | classes (`python_equiv`) | parity records (implemented/partial) | Total |",
        "|---|---:|---:|---:|",
    ]
    for level, total in s["by_evidence"].items():
        lines.append(f"| {level} | {s['by_source']['classes'][level]} | {s['by_source']['parity'][level]} | {total} |")
    lines += [
        f"| **all claims** | {sum(s['by_source']['classes'].values())} | {sum(s['by_source']['parity'].values())} "
        f"| {s['claims']} |",
        "",
        "`cross-validated-live` tests call whatever `mne` is installed, so they are neither pinned nor deterministic.",
        "",
        "Tests comparing against embedded MNE-Python values: "
        + (", ".join(f"`{t}`" for t in s["tests_by_relation"]["cross-validated-static"]) or "none"),
        "",
        "Tests running MNE-Python live: "
        + (", ".join(f"`{t}`" for t in s["tests_by_relation"]["cross-validated-live"]) or "none"),
        "",
        "## Denominator",
        "",
        report["denominator"]["note"],
        "",
        "## Aggregate percentage claims",
        "",
        "| Location | Claim | Status |",
        "|---|---|---|",
    ]
    for claim in report["percent_claims"]:
        lines.append(f"| `{claim['path']}:{claim['line']}` | {claim['text']} | {claim['status']} |")
    if not report["percent_claims"]:
        lines.append("| - | - | none found |")
    lines += [
        "",
        "## Equivalence claims",
        "",
        "Tolerance is the range of comparison thresholds the test asserts (`exact` when it only compares exactly).",
        "",
        "| MNE-Python | MNE-CPP | Claim | Test | Tolerance |",
        "|---|---|---|---|---|",
    ]
    for claim in sorted((c for c in report["claims"] if c["equivalence"]), key=lambda c: c["python"]):
        lines.append(f"| `{claim['python']}` | `{claim['mne_cpp']}` | {claim['equivalence']} | "
                     f"`{claim['test']}` | {claim.get('tolerance', '-')} |")
    return "\n".join(lines) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--json", type=Path, dest="json_path", help="write the JSON report")
    parser.add_argument("--markdown", type=Path, help="write the Markdown summary")
    args = parser.parse_args(argv)

    report = build_report()
    if args.json_path:
        args.json_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if args.markdown:
        args.markdown.write_text(render_markdown(report), encoding="utf-8")
    if not args.json_path and not args.markdown:
        print(render_markdown(report), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
