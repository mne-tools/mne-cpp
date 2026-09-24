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
  what the local fixture-generation environment reports,
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


def test_evidence(testframes: Path) -> dict[str, str]:
    """Classify each test directory by how it relates to MNE-Python."""
    evidence: dict[str, str] = {}
    for directory in sorted(p for p in testframes.iterdir() if p.is_dir()):
        text = "\n".join(
            path.read_text(encoding="utf-8", errors="replace")
            for path in sorted(directory.rglob("*")) if path.suffix in {".cpp", ".h", ".py"}
        )
        live = _LIVE_RE.search(text) or any(
            _PROCESS_RE.search(block) and re.search(r"\bpython3?\b\"|import mne", block)
            for block in re.split(r"\n\s*\n", text)
        )
        if live:
            evidence[directory.name] = "cross-validated-live"
        elif _STATIC_RE.search(text):
            evidence[directory.name] = "cross-validated-static"
        else:
            evidence[directory.name] = "tested"
    return evidence


def registered_tests(testframes: Path) -> set[str]:
    cmake = (testframes / "CMakeLists.txt").read_text(encoding="utf-8")
    return set(re.findall(r"^\s*add_subdirectory\s*\(\s*(test_\w+)", cmake, re.MULTILINE))


def reference_environment(repo_root: Path, registry: dict[str, Any]) -> dict[str, Any]:
    workflow = repo_root / ".github" / "workflows" / "_reusable-tests.yml"
    text = workflow.read_text(encoding="utf-8") if workflow.is_file() else ""
    installs = [" ".join(line.split()) for line in _PIP_RE.findall(text) if "pip" not in line.split()[:1]]
    packages = sorted({token for line in installs for token in line.split() if not token.startswith("-")})
    unpinned = [p for p in packages if not re.search(r"[=<>~]", p)]
    return {
        "registry_mne_python_ref": registry.get("meta", {}).get("mne_python_ref"),
        "registry_mne_python_pinned": registry.get("parity", {}).get("mne_python_pinned"),
        "ci_python_versions": sorted(set(_PYTHON_VERSION_RE.findall(text))),
        "ci_pip_packages": packages,
        "ci_unpinned_packages": unpinned,
        "lock_file": None,
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

    claims: list[dict[str, Any]] = []
    for record in registry.get("classes", []):
        python = record.get("python_equiv") or record.get("mne_python")
        if not python:
            continue
        test = record.get("test")
        level = evidence_by_test.get(test, "unverified") if test in registered else "unverified"
        claims.append({"source": "classes", "mne_cpp": record["name"], "python": python,
                       "claimed": "implemented", "test": test, "evidence": level})
    for record in registry.get("parity", {}).get("records", []):
        if record.get("status") not in {"implemented", "partial"}:
            continue
        claims.append({"source": "parity", "mne_cpp": record.get("mne_cpp") or None, "python": record["python"],
                       "claimed": record["status"], "test": None, "evidence": "unverified"})

    levels = ("cross-validated-static", "cross-validated-live", "tested", "unverified")
    by_level = {level: sum(1 for c in claims if c["evidence"] == level) for level in levels}
    return {
        "schema_version": 1,
        "generated_by": "tools/quality/audit_parity_baseline.py",
        "reference_environment": reference_environment(repo_root, registry),
        "summary": {
            "claims": len(claims),
            "by_evidence": by_level,
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
        f"- Lock file for fixture generation: {env['lock_file'] or '**none**'}",
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
