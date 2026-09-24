#!/usr/bin/env python3
# =============================================================================================================
#
# @file     generate_quality_report.py
# @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
# @since    2.4.0
# @date     September, 2026
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#
# @brief    Regenerate every v2.4.0 baseline report and the combined quality dashboard.
#
# =============================================================================================================
"""One command for the v2.4.0 quality baseline (package T0.7).

Regenerates, from the working tree plus the pinned CI evidence in
``doc/release/v2.4.0``:

* ``test-inventory.{json,md}``          T0.1  (``audit_tests.py``)
* ``api-evidence-baseline.{json,md}``   T0.3  (``audit_api_evidence.py``)
* ``visual-baseline.{json,md}``         T0.4  (``audit_visual_assets.py``)
* ``maintainability-baseline.{json,md}``T0.5  (``audit_maintainability.py``)
* ``parity-baseline.{json,md}``         T0.6  (``audit_parity_baseline.py``)
* ``quality-baseline.{json,md}``        T0.7  gate-by-gate dashboard

``coverage-baseline.json`` (T0.2) and ``ci-test-results.json`` are inputs, not
outputs: both are imported from a named CI run because neither can be measured
from a checkout without building.  ``mne-python-gap.json`` (T7.0) is likewise
an input produced by ``tools/parity/gap_analysis.py`` against the pinned
MNE-Python install.

Usage
-----
    python3 tools/quality/generate_quality_report.py            # regenerate
    python3 tools/quality/generate_quality_report.py --check    # CI: fail if any report is stale
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Callable

sys.path.insert(0, str(Path(__file__).resolve().parent))
import audit_api_evidence  # noqa: E402
import audit_maintainability  # noqa: E402
import audit_parity_baseline  # noqa: E402
import audit_tests  # noqa: E402
import audit_visual_assets  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parents[2]
RELEASE_DIR = REPO_ROOT / "doc" / "release" / "v2.4.0"

# Gate thresholds as recorded in doc/dev-notes/v2.4.0-requirements.md (G2, G3).
THRESHOLDS = {
    "coverage_line_libraries": 70.0,
    "coverage_branch_libraries": 55.0,
    "coverage_line_applications": 50.0,
    "coverage_line_tools": 50.0,
    "coverage_line_combined": 60.0,
    "example_eligible_backed": 100.0,
}


def _json(data: dict[str, Any]) -> str:
    return json.dumps(data, indent=2) + "\n"


def _load(name: str) -> dict[str, Any]:
    return json.loads((RELEASE_DIR / name).read_text(encoding="utf-8"))


def _pct(part: int, whole: int) -> float | None:
    return round(100.0 * part / whole, 2) if whole else None


def section_reports() -> dict[str, tuple[dict[str, Any], Callable[[dict[str, Any]], str]]]:
    return {
        "test-inventory": (
            audit_tests.build_inventory(REPO_ROOT, audit_tests.load_ci_evidence(audit_tests.CI_EVIDENCE)),
            audit_tests.render_markdown,
        ),
        "api-evidence-baseline": (audit_api_evidence.build_report(), audit_api_evidence.render_markdown),
        "visual-baseline": (audit_visual_assets.build_report(), audit_visual_assets.render_markdown),
        "maintainability-baseline": (audit_maintainability.build_report(), audit_maintainability.render_markdown),
        "parity-baseline": (audit_parity_baseline.build_report(), audit_parity_baseline.render_markdown),
    }


def build_dashboard(sections: dict[str, dict[str, Any]]) -> dict[str, Any]:
    coverage = _load("coverage-baseline.json")
    gap = _load("mne-python-gap.json")
    tests = sections["test-inventory"]["summary"]
    api = sections["api-evidence-baseline"]["summary"]
    visual = sections["visual-baseline"]["summary"]
    maintain = sections["maintainability-baseline"]["metrics"]
    parity = sections["parity-baseline"]
    scopes = coverage["scopes"]

    gates: dict[str, dict[str, Any]] = {
        "G1_test_integrity": {
            "registered": tests["registration"]["unconditional"] + tests["registration"]["conditional"],
            "conditional": tests["registration"]["conditional"],
            "unregistered": tests["registration"]["unregistered"],
            "without_labels": tests["without_labels"],
            "without_timeout": tests["without_timeout"],
            "ci_executed": {p: d["executed"] for p, d in tests["ci"].items()},
            "ci_failures": sum(d["failures"] for d in tests["ci"].values()),
            "ci_platform_exclusions": tests["ci_excluded"],
            "retry_paths": len(tests["retry_paths"]),
        },
        "G2_coverage": {
            "commit": coverage["source"]["commit"],
            "source": coverage["source"]["coverage_url"],
            "line_combined": coverage["totals"]["line_percent"],
            "line_libraries": scopes["libraries"]["line_percent"],
            "line_applications": scopes["applications"]["line_percent"],
            "line_tools": scopes["tools"]["line_percent"],
            "branch_reported": coverage["totals"]["branches_found"] > 0,
            "lines_to_combined_target": max(
                0, round(THRESHOLDS["coverage_line_combined"] / 100 * coverage["totals"]["lines_found"]
                         - coverage["totals"]["lines_hit"])),
            "lines_to_scope_targets": {
                scope: max(0, round(THRESHOLDS[f"coverage_line_{scope}"] / 100 * scopes[scope]["lines_found"]
                                    - scopes[scope]["lines_hit"]))
                for scope in ("libraries", "applications", "tools")
            },
        },
        "G3_api_documentation": {
            "public_api_units": api["public_api_units"],
            "exported_not_registered": api["exported_not_registered"],
            "example_eligible": api["example_eligible"],
            "eligible_backed_percent": _pct(api["eligible_backed_by_registry_example"], api["example_eligible"]),
            "snippets": api["with_snippet"],
            "registered_test_percent": _pct(api["registered_with_test"], api["registered_classes"]),
        },
        "G4_screenshots": {
            "referenced_images": visual["referenced_images"],
            "referenced_without_producer": visual["referenced_without_producer"],
            "doc_shots_run_in_ci": visual["doc_shots_run_in_ci"],
            "placeholder_workflows": len(visual["placeholder_workflows"]),
            "golden_comparisons": visual["golden_references"],
        },
        "G5_maintainability": {
            "compiler_warnings": 0,
            **{name: metric["total"] for name, metric in maintain.items()},
        },
        "G6_parity": {
            "reference": parity["reference_environment"]["registry_mne_python_ref"],
            "unpinned_ci_packages": parity["reference_environment"]["ci_unpinned_packages"],
            "inventoried_apis": gap["summary"]["total"],
            "by_status": gap["summary"]["by_status"],
            "claims": parity["summary"]["claims"],
            "claims_by_evidence": parity["summary"]["by_evidence"],
        },
    }
    return {
        "schema_version": 1,
        "generated_by": "tools/quality/generate_quality_report.py",
        "thresholds": THRESHOLDS,
        "gates": gates,
    }


def render_dashboard(report: dict[str, Any]) -> str:
    g = report["gates"]
    t = report["thresholds"]
    g1, g2, g3, g4, g5, g6 = (g[k] for k in (
        "G1_test_integrity", "G2_coverage", "G3_api_documentation", "G4_screenshots",
        "G5_maintainability", "G6_parity"))

    def status(ok: bool) -> str:
        return "met" if ok else "**open**"

    rows = [
        ("G1", "Every registered test runs in CI", f"{g1['registered']} registered; CI ran "
         + ", ".join(f"{n} on {p}" for p, n in g1["ci_executed"].items()),
         status(all(n >= g1["registered"] - len(g1["ci_platform_exclusions"]) for n in g1["ci_executed"].values()))),
        ("G1", "Tests carry labels and timeouts", f"{g1['without_labels']} unlabelled, {g1['without_timeout']} untimed",
         status(g1["without_labels"] == 0 and g1["without_timeout"] == 0)),
        ("G1", "No retry-to-pass path", f"{g1['retry_paths']} retry lines in local scripts", status(g1["retry_paths"] == 0)),
        ("G2", f"Combined line coverage >= {t['coverage_line_combined']}%", f"{g2['line_combined']}% "
         f"({g2['lines_to_combined_target']:,} lines short)", status(g2["line_combined"] >= t["coverage_line_combined"])),
        ("G2", f"Libraries >= {t['coverage_line_libraries']}% line", f"{g2['line_libraries']}%",
         status(g2["line_libraries"] >= t["coverage_line_libraries"])),
        ("G2", f"Applications >= {t['coverage_line_applications']}% line", f"{g2['line_applications']}%",
         status(g2["line_applications"] >= t["coverage_line_applications"])),
        ("G2", f"Tools >= {t['coverage_line_tools']}% line", f"{g2['line_tools']}%",
         status(g2["line_tools"] >= t["coverage_line_tools"])),
        ("G2", f"Libraries >= {t['coverage_branch_libraries']}% branch",
         "branch counters not collected" if not g2["branch_reported"] else "reported", status(False)),
        ("G3", "Eligible APIs have an executable example", f"{g3['eligible_backed_percent']}% of "
         f"{g3['example_eligible']}; {g3['snippets']} snippets", status(g3["eligible_backed_percent"] == 100.0)),
        ("G3", "Exported API is registered", f"{g3['exported_not_registered']} exported classes unregistered",
         status(g3["exported_not_registered"] == 0)),
        ("G4", "Documentation images generated in CI", f"{g4['referenced_images']} referenced, "
         f"{g4['referenced_without_producer']} without producer, placeholders in {g4['placeholder_workflows']} workflows",
         status(g4["doc_shots_run_in_ci"] and g4["placeholder_workflows"] == 0)),
        ("G4", "Visual regression compares to goldens", f"{g4['golden_comparisons']} golden comparisons",
         status(g4["golden_comparisons"] > 0)),
        ("G5", "Zero compiler warnings", "enforced by -Werror on every matrix entry", status(True)),
        ("G5", "Debt metrics ratcheted", ", ".join(f"{k} {v:,}" for k, v in g5.items() if k != "compiler_warnings"),
         "baseline recorded"),
        ("G6", "Pinned reference environment", f"MNE-Python {g6['reference']}; unpinned in CI: "
         + ", ".join(g6["unpinned_ci_packages"]), status(not g6["unpinned_ci_packages"])),
        ("G6", "Parity claims cross-validated", ", ".join(f"{k} {v}" for k, v in g6["claims_by_evidence"].items()),
         status(g6["claims_by_evidence"]["unverified"] == 0 and g6["claims_by_evidence"]["tested"] == 0)),
    ]
    lines = [
        "# MNE-CPP v2.4.0 quality baseline",
        "",
        "Generated by `tools/quality/generate_quality_report.py`. Do not edit by hand.",
        f"Coverage from `{g2['commit'][:9] if g2['commit'] else 'n/a'}` ({g2['source']}).",
        "",
        "| Gate | Criterion | Current | Status |",
        "|---|---|---|---|",
    ]
    lines += [f"| {gate} | {criterion} | {current} | {state} |" for gate, criterion, current, state in rows]
    lines += [
        "",
        "Detailed reports: [test inventory](test-inventory.md), [coverage](coverage-baseline.md), "
        "[API evidence](api-evidence-baseline.md), [visual](visual-baseline.md), "
        "[maintainability](maintainability-baseline.md), [parity evidence](parity-baseline.md), "
        "[MNE-Python gap](mne-python-gap.md).",
        "",
        "G7 (signed artifacts) and G8 (clean-machine onboarding) are not measurable from the tree; they are tracked in TASK 10.",
    ]
    return "\n".join(lines) + "\n"


def expected_outputs(out_dir: Path = RELEASE_DIR) -> dict[Path, str]:
    outputs: dict[Path, str] = {}
    data: dict[str, dict[str, Any]] = {}
    for name, (report, render) in section_reports().items():
        data[name] = report
        outputs[out_dir / f"{name}.json"] = _json(report)
        outputs[out_dir / f"{name}.md"] = render(report)
    dashboard = build_dashboard(data)
    outputs[out_dir / "quality-baseline.json"] = _json(dashboard)
    outputs[out_dir / "quality-baseline.md"] = render_dashboard(dashboard)
    return outputs


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="fail if a committed report differs from a fresh one")
    parser.add_argument("--out-dir", type=Path, default=RELEASE_DIR,
                        help="write reports here instead of the committed baseline (e.g. for a CI artifact)")
    args = parser.parse_args(argv)

    out_dir = args.out_dir.resolve()
    outputs = expected_outputs(out_dir)
    if args.check:
        stale = [p for p, text in outputs.items() if not p.is_file() or p.read_text(encoding="utf-8") != text]
        for path in stale:
            print(f"STALE {path}")
        if stale:
            print("Regenerate with: python3 tools/quality/generate_quality_report.py")
            return 1
        print(f"OK: {len(outputs)} reports are current.")
        return 0

    out_dir.mkdir(parents=True, exist_ok=True)
    if out_dir != RELEASE_DIR:
        # Inputs travel with the artifact so its report links resolve.
        for name in ("coverage-baseline.md", "mne-python-gap.md"):
            (out_dir / name).write_text((RELEASE_DIR / name).read_text(encoding="utf-8"), encoding="utf-8")
    for path, text in outputs.items():
        path.write_text(text, encoding="utf-8")
        print(f"wrote {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
