#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Release evidence bundle (v2.4.0 T10.2, T10.8d).

``build`` collects the generated release reports (``doc/release/v2.4.0``), the
onboarding reports of the documented setup path and, once a release exists, its
signing evidence into one directory with a checksummed ``index.json`` that maps
every gate G1-G8 to the files that justify it::

    python3 scripts/release/evidence_bundle.py build --out bundle \\
        [--onboarding DIR] [--signing release-evidence.json] [--ci-run URL]

``pin-onboarding`` records the onboarding reports of one CI run, with the setup
time history, in ``doc/release/v2.4.0/onboarding-results.json`` for the dashboard.

``verify`` checks a bundle offline: every listed file is present with its
checksum, nothing unlisted was added, and every gate points at existing files::

    python3 scripts/release/evidence_bundle.py verify bundle
"""

from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
# The release whose reports scripts/quality/generate_quality_report.py maintains
RELEASE_DIR = Path("doc") / "release" / "v2.4.0"

# gate -> report files (relative to the bundle's reports/ directory) that justify it
REPORT_GATES = {
    "G1_test_integrity": ["test-inventory.json", "ci-test-results.json"],
    "G2_coverage": ["coverage-baseline.json"],
    "G3_api_documentation": ["api-evidence-baseline.json"],
    "G4_screenshots": ["visual-baseline.json"],
    "G5_maintainability": ["maintainability-baseline.json"],
    "G6_parity": ["parity-baseline.json", "mne-python-gap.json"],
    "G8_onboarding": ["onboarding-results.json"],
}


def _sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def project_version(root: Path) -> str:
    text = (root / "src" / "CMakeLists.txt").read_text(encoding="utf-8")
    return ".".join(re.search(rf"set\(MNE_CPP_VERSION_{part}\s+(\d+)\)", text).group(1)
                    for part in ("MAJOR", "MINOR", "PATCH"))


def onboarding_summary(reports: list[Path]) -> dict:
    summary = {}
    for path in reports:
        result = json.loads(path.read_text(encoding="utf-8"))
        entry = {key: result.get(key) for key in
                 ("ok", "failed_step", "total_seconds", "checkout_bytes", "platform", "dependencies")}
        # Reports written before the runner checked for steps that never ran claimed success anyway
        not_run = [step["command"] for step in result.get("steps", []) if not step["ok"]]
        if not_run:
            entry.update(ok=False, failed_step=entry["failed_step"] or not_run[0])
        summary[result["os"]] = entry
    return summary


def pin_onboarding(target: Path, reports: list[Path], run: str, date: str) -> dict:
    """Record the onboarding results of one CI run and append its setup times to the history."""
    pinned = json.loads(target.read_text(encoding="utf-8")) if target.is_file() else {"history": []}
    results = onboarding_summary(reports)
    history = [entry for entry in pinned["history"] if entry["run"] != run]
    history.append({"run": run, "date": date, **{os_name: r["total_seconds"] for os_name, r in results.items()}})
    pinned = {"run": run, "results": results, "history": history}
    target.write_text(json.dumps(pinned, indent=2) + "\n", encoding="utf-8")
    return pinned


def build(out: Path, reports: Path, onboarding: Path | None, signing: Path | None,
          ci_run: str | None, commit: str, version: str) -> dict:
    if out.exists():
        shutil.rmtree(out)
    (out / "reports").mkdir(parents=True)
    for path in sorted(reports.iterdir()):
        if path.is_file():
            shutil.copy2(path, out / "reports" / path.name)
    gates = {gate: {"files": [f"reports/{name}" for name in names]} for gate, names in REPORT_GATES.items()}

    gates["G7_signing"] = {"files": [], "status": "pending until the tagged release publishes its assets"}
    if signing:
        (out / "signing").mkdir()
        shutil.copy2(signing, out / "signing" / signing.name)
        gates["G7_signing"] = {"files": [f"signing/{signing.name}"]}

    if onboarding:
        (out / "onboarding").mkdir()
        for path in sorted(onboarding.rglob("onboarding-*.json")):
            shutil.copy2(path, out / "onboarding" / path.name)
            gates["G8_onboarding"]["files"].append(f"onboarding/{path.name}")

    files = {p.relative_to(out).as_posix(): _sha256(p) for p in sorted(out.rglob("*")) if p.is_file()}
    index = {
        "schema_version": 1,
        "version": version,
        "commit": commit,
        "generated_at": datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds"),
        "ci_run": ci_run,
        "gates": gates,
        "files": files,
    }
    (out / "index.json").write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    return index


def verify(bundle: Path) -> list[str]:
    index_path = bundle / "index.json"
    if not index_path.is_file():
        return ["index.json is missing"]
    index = json.loads(index_path.read_text(encoding="utf-8"))
    problems = []
    listed = index.get("files", {})
    for name, digest in listed.items():
        path = bundle / name
        if not path.is_file():
            problems.append(f"{name} is missing")
        elif _sha256(path) != digest:
            problems.append(f"{name} does not match its checksum")
    present = {p.relative_to(bundle).as_posix() for p in bundle.rglob("*") if p.is_file()} - {"index.json"}
    problems += [f"{name} is not listed in index.json" for name in sorted(present - set(listed))]
    expected = [*REPORT_GATES, "G7_signing"]
    gates = index.get("gates", {})
    for gate in expected:
        if gate not in gates:
            problems.append(f"{gate} has no entry")
            continue
        for name in gates[gate]["files"]:
            if name not in listed:
                problems.append(f"{gate} refers to {name}, which is not in the bundle")
        if gate in REPORT_GATES and not gates[gate]["files"]:
            problems.append(f"{gate} has no evidence")
    return problems


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    commands = parser.add_subparsers(dest="command", required=True)
    build_parser = commands.add_parser("build", help="assemble a bundle")
    build_parser.add_argument("--out", type=Path, required=True)
    build_parser.add_argument("--reports", type=Path, help=f"default: {RELEASE_DIR}")
    build_parser.add_argument("--onboarding", type=Path, help="directory holding onboarding-*.json")
    build_parser.add_argument("--signing", type=Path, help="release-evidence.json of a published release")
    build_parser.add_argument("--ci-run", help="URL of the CI run the reports were pinned to")
    build_parser.add_argument("--repo-root", type=Path, default=REPO_ROOT)
    pin_parser = commands.add_parser("pin-onboarding", help="record onboarding results in the release reports")
    pin_parser.add_argument("reports", type=Path, help="directory holding onboarding-*.json")
    pin_parser.add_argument("--run", required=True, help="URL of the onboarding run")
    pin_parser.add_argument("--date", default=datetime.date.today().isoformat())
    pin_parser.add_argument("--repo-root", type=Path, default=REPO_ROOT)
    verify_parser = commands.add_parser("verify", help="check a bundle offline")
    verify_parser.add_argument("bundle", type=Path)
    args = parser.parse_args(argv)

    if args.command == "pin-onboarding":
        target = args.repo_root / RELEASE_DIR / "onboarding-results.json"
        pinned = pin_onboarding(target, sorted(args.reports.rglob("onboarding-*.json")), args.run, args.date)
        print(f"wrote {target} ({', '.join(pinned['results'])})")
        return 0
    if args.command == "build":
        version = project_version(args.repo_root)
        reports = args.reports or args.repo_root / RELEASE_DIR
        commit = subprocess.run(["git", "rev-parse", "HEAD"], cwd=args.repo_root, capture_output=True,
                                text=True, check=True).stdout.strip()
        build(args.out, reports, args.onboarding, args.signing, args.ci_run, commit, version)
    problems = verify(args.out if args.command == "build" else args.bundle)
    for problem in problems:
        print(f"FAIL: {problem}")
    if not problems:
        print("PASS: evidence bundle is complete and matches its checksums.")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
