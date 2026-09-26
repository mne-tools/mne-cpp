#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     September, 2026
#
#
# @brief    Summarize a CTest JUnit file: failures, skipped tests, and QSKIPs.
"""Summarize a CTest ``--output-junit`` file for the CI run summary.

CTest reports a Qt Test binary that calls ``QSKIP`` as a pass, so a skip inside
an otherwise green test is invisible unless someone reads its output.  This
script lists both kinds of skip next to the failures and quarantined tests:

* tests CTest did not run (``mne_add_test`` target/platform skips, missing
  ``REQUIRES_DATA`` files), with CTest's reason,
* ``SKIP :`` lines inside tests that ran, with the QSKIP message,
* tests listed under ``quarantine`` in ``test_inventory_policy.json``.

Pass ``--require-reasons`` to fail when any skip has no reason.

Usage
-----
    python3 tools/quality/summarize_test_run.py build/test-results.xml \\
        --platform ubuntu-24.04 >> "$GITHUB_STEP_SUMMARY"
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from pathlib import Path

POLICY_FILE = Path(__file__).resolve().parent / "test_inventory_policy.json"

_QSKIP_RE = re.compile(r"^SKIP\s*:\s*(\S+?\(.*?\))\s*(.*?)\s*$", re.MULTILINE)
_LOC_RE = re.compile(r"\s*Loc:\s*\[.*$")
_PLACEHOLDER_RE = re.compile(r"^MNE_TEST_SKIPPED:\s*(.*)$", re.MULTILINE)


@dataclass
class TestRun:
    """Everything the summary needs from one JUnit file."""

    total: int = 0
    failed: list[str] = field(default_factory=list)
    not_run: list[tuple[str, str]] = field(default_factory=list)
    qskips: list[tuple[str, str, str]] = field(default_factory=list)


def parse_junit(path: Path) -> TestRun:
    run = TestRun()
    for case in ET.parse(path).getroot().iter("testcase"):
        run.total += 1
        name = case.get("name") or "?"
        output = case.findtext("system-out") or ""
        skipped = case.find("skipped")
        if case.find("failure") is not None or case.get("status") == "fail":
            run.failed.append(name)
        elif skipped is not None or case.get("status") == "notrun":
            placeholder = _PLACEHOLDER_RE.search(output)
            if placeholder:
                reason = placeholder.group(1).strip()
            else:
                message = skipped.get("message", "") if skipped is not None else ""
                detail = output.strip().splitlines()[0] if output.strip() else ""
                reason = f"{message}: {detail}".strip(": ") if detail else message
            run.not_run.append((name, reason))
        for function, reason in _QSKIP_RE.findall(output):
            run.qskips.append((name, function, _LOC_RE.sub("", reason).strip()))
    return run


def load_quarantine(policy: Path) -> list[dict[str, str]]:
    if not policy.is_file():
        return []
    return json.loads(policy.read_text(encoding="utf-8")).get("quarantine", [])


def render(run: TestRun, platform: str, quarantine: list[dict[str, str]]) -> str:
    lines = [
        f"### Test run: {platform}",
        "",
        f"{run.total} registered, {len(run.failed)} failed, {len(run.not_run)} not run, "
        f"{len(run.qskips)} QSKIP'd functions in passing tests, {len(quarantine)} quarantined.",
        "",
    ]
    if run.failed:
        lines += ["**Failed:** " + ", ".join(f"`{name}`" for name in run.failed), ""]
    if run.not_run:
        lines += ["| Not run | Reason |", "|---|---|"]
        lines += [f"| `{name}` | {reason or '**no reason given**'} |" for name, reason in run.not_run]
        lines.append("")
    if run.qskips:
        lines += ["| Test | Skipped function | Reason |", "|---|---|---|"]
        lines += [f"| `{name}` | `{function}` | {reason or '**no reason given**'} |"
                  for name, function, reason in run.qskips]
        lines.append("")
    if quarantine:
        lines += ["| Quarantined | Owner | Expires | Issue |", "|---|---|---|---|"]
        lines += [f"| `{q.get('test')}` | {q.get('owner')} | {q.get('expires')} | {q.get('issue')} |"
                  for q in quarantine]
        lines.append("")
    return "\n".join(lines) + "\n"


def unexplained(run: TestRun) -> list[str]:
    missing = [name for name, reason in run.not_run if not reason]
    missing += [f"{name}::{function}" for name, function, reason in run.qskips if not reason]
    return missing


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("junit", type=Path, help="CTest --output-junit file")
    parser.add_argument("--platform", default="local", help="label for the summary heading")
    parser.add_argument("--policy", type=Path, default=POLICY_FILE, help="test inventory policy file")
    parser.add_argument("--require-reasons", action="store_true", help="fail when a skip has no reason")
    args = parser.parse_args(argv)

    try:
        run = parse_junit(args.junit)
    except (OSError, ET.ParseError) as exc:
        print(f"error: cannot read {args.junit}: {exc}", file=sys.stderr)
        return 2

    sys.stdout.write(render(run, args.platform, load_quarantine(args.policy)))

    if args.require_reasons:
        missing = unexplained(run)
        if missing:
            print(f"error: skipped without a reason: {', '.join(missing)}", file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
