#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    3.0.0
# @date     November, 2026
#
#
# @brief    A/B regression gate of the acquisition performance suite.
"""Runs mne_acq_bench of a base and a head build alternately and fails on a regression.

Absolute numbers on shared CI machines drift by tens of percent between runs, so the two builds are run
interleaved on the same machine and each metric keeps its best value. Throughput, per-call cost and converter
rates are gated: head may be at most ``--tolerance`` worse than base. Wake-up latencies and CPU time depend on the
scheduler of the machine and are compared only for the report. Any metric that has a target (e.g. a p99 wake-up
latency, a paced rate) must also meet it, whatever the base did.

    python3 scripts/ci/acq_bench/compare.py --base build-base/mne_acq_bench --head build-head/mne_acq_bench
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path

REPORT_ONLY_PREFIXES = ("wake_", "cpu_")


def run_bench(binary: str, quick: bool) -> list[dict]:
    """Runs one suite and returns its metrics."""
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "result.json"
        cmd = [binary, "--repeats", "1", "--json", str(out)] + (["--quick"] if quick else [])
        proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
        if not out.exists():
            raise RuntimeError(f"{binary} failed ({proc.returncode}):\n{proc.stdout}\n{proc.stderr}")
        return json.loads(out.read_text(encoding="utf-8"))["metrics"]


def best_of(runs: list[list[dict]]) -> dict[str, dict]:
    """Best value per metric over all runs, in the metric's own direction; a lost-sample run (-1) is kept."""
    best: dict[str, dict] = {}
    for metrics in runs:
        for m in metrics:
            old = best.get(m["name"])
            if old is not None and old["value"] < 0:
                continue
            if old is None or m["value"] < 0:
                best[m["name"]] = dict(m)
            elif (m["value"] > old["value"]) if m["better"] == "higher" else (m["value"] < old["value"]):
                best[m["name"]] = dict(m)
    return best


def compare(base: dict[str, dict], head: dict[str, dict], tolerance: float) -> list[dict]:
    """One row per head metric with its status: ok, REGRESSION, LOST, TARGET (missed), report or new."""
    rows = []
    for name, h in head.items():
        b = base.get(name)
        row = {"name": name, "unit": h["unit"], "head": h["value"], "base": None if b is None else b["value"]}
        if h["value"] < 0:
            row["status"] = "LOST"
        elif b is None or b["value"] <= 0:
            row["status"] = "new"
        else:
            higher = h["better"] == "higher"
            change = (h["value"] - b["value"]) / b["value"]
            row["change"] = change
            worse = -change if higher else change
            if name.startswith(REPORT_ONLY_PREFIXES):
                row["status"] = "report"
            else:
                row["status"] = "REGRESSION" if worse > tolerance else "ok"
        target = h.get("target", 0)
        if row["status"] != "LOST" and target > 0:
            missed = h["value"] < target if h["better"] == "higher" else h["value"] > target
            if missed:
                row["status"] = "TARGET"
        rows.append(row)
    return rows


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--base", required=True, help="mne_acq_bench of the base commit")
    parser.add_argument("--head", required=True, help="mne_acq_bench of the commit under test")
    parser.add_argument("--runs", type=int, default=5, help="alternating runs per build (default 5)")
    parser.add_argument("--tolerance", type=float, default=0.2, help="allowed relative slowdown (default 0.2)")
    parser.add_argument("--quick", action="store_true", help="pass --quick to the suite")
    parser.add_argument("--json", help="write the comparison as JSON")
    args = parser.parse_args(argv)

    base_runs, head_runs = [], []
    for i in range(args.runs):
        # Alternate the order too, so neither build always runs on a warmer or cooler machine.
        order = [(args.base, base_runs), (args.head, head_runs)]
        for binary, runs in order if i % 2 == 0 else reversed(order):
            runs.append(run_bench(binary, args.quick))
    rows = compare(best_of(base_runs), best_of(head_runs), args.tolerance)

    print(f"{'metric':36} {'base':>12} {'head':>12} {'change':>8}  status")
    for r in rows:
        base = "-" if r["base"] is None else f"{r['base']:.4g}"
        change = f"{100 * r['change']:+.1f}%" if "change" in r else ""
        print(f"{r['name']:36} {base:>12} {r['head']:>12.4g} {change:>8}  {r['status']} [{r['unit']}]")
    if args.json:
        Path(args.json).write_text(json.dumps(rows, indent=2) + "\n", encoding="utf-8")
    failed = [r["name"] for r in rows if r["status"] in ("REGRESSION", "LOST", "TARGET")]
    if failed:
        print(f"FAIL: {len(failed)} metric(s) regressed by more than {100 * args.tolerance:.0f}%, "
              f"lost samples or missed their target: {', '.join(failed)}")
        return 1
    print(f"PASS: no gated metric more than {100 * args.tolerance:.0f}% worse than the base, every target met")
    return 0


if __name__ == "__main__":
    sys.exit(main())
