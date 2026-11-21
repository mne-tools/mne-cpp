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
interleaved on the same machine, a round running both back to back. A gated metric compares the median over the
rounds of head relative to base within a round, because a shared machine has slow and fast phases that meet both
builds of a round but not every round alike; the best values are reported. A regression must show again after
further rounds before it fails, and the JSON keeps every run's value for the analysis of a failure. Throughput, per-call cost and converter
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


def paired_change(name: str, base_runs: list[list[dict]], head_runs: list[list[dict]]) -> float | None:
    """Median over the rounds of head relative to base within the round, as a change of head; None without pairs.

    The two builds of a round run back to back, so a slow or fast phase of a shared machine meets both."""
    ratios = []
    for b_metrics, h_metrics in zip(base_runs, head_runs):
        b = next((m["value"] for m in b_metrics if m["name"] == name), None)
        h = next((m["value"] for m in h_metrics if m["name"] == name), None)
        if b is not None and h is not None and b > 0 and h > 0:
            ratios.append(h / b)
    if not ratios:
        return None
    ratios.sort()
    middle = len(ratios) // 2
    median = ratios[middle] if len(ratios) % 2 else (ratios[middle - 1] + ratios[middle]) / 2
    return median - 1.0


def compare(base: dict[str, dict], head: dict[str, dict], tolerance: float,
            base_runs: list[list[dict]] | None = None, head_runs: list[list[dict]] | None = None) -> list[dict]:
    """One row per head metric with its status: ok, REGRESSION, LOST, TARGET (missed), report or new.

    With the runs of both builds, a gated metric is judged by the median ratio within the rounds; the best values
    are reported either way."""
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
            if base_runs and head_runs:
                paired = paired_change(name, base_runs, head_runs)
                change = change if paired is None else paired
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


def alternate(base: str, head: str, rounds: int, quick: bool, bench, base_runs: list, head_runs: list) -> None:
    """Runs both builds @p rounds times, alternating which goes first, so neither always meets a warmer machine."""
    for _ in range(rounds):
        order = [(base, base_runs), (head, head_runs)]
        for binary, runs in order if (len(base_runs) + len(head_runs)) % 4 == 0 else reversed(order):
            runs.append(bench(binary, quick))


def per_run(runs: list[list[dict]], name: str) -> list[float]:
    return [m["value"] for metrics in runs for m in metrics if m["name"] == name]


def gate(base: str, head: str, runs: int, confirm_runs: int, tolerance: float, quick: bool,
         bench=run_bench) -> tuple[list[dict], int]:
    """The comparison and the rounds it took. A regression must also show after @p confirm_runs more rounds,
    over the best of all of them: a shared runner has slow minutes that can catch every run of one build."""
    base_runs: list = []
    head_runs: list = []
    alternate(base, head, runs, quick, bench, base_runs, head_runs)
    rows = compare(best_of(base_runs), best_of(head_runs), tolerance, base_runs, head_runs)
    if confirm_runs > 0 and any(r["status"] == "REGRESSION" for r in rows):
        first = {r["name"]: r["status"] for r in rows}
        alternate(base, head, confirm_runs, quick, bench, base_runs, head_runs)
        confirm = {r["name"]: r["status"] for r in compare(best_of(base_runs[runs:]), best_of(head_runs[runs:]), tolerance,
                                                           base_runs[runs:], head_runs[runs:])}
        rows = compare(best_of(base_runs), best_of(head_runs), tolerance, base_runs, head_runs)
        for r in rows:
            # A regression must show in the first rounds and again in the further ones
            if r["status"] in ("REGRESSION", "ok"):
                r["status"] = "REGRESSION" if first.get(r["name"]) == confirm.get(r["name"]) == "REGRESSION" else "ok"
    for r in rows:
        r["base_runs"] = per_run(base_runs, r["name"])
        r["head_runs"] = per_run(head_runs, r["name"])
    return rows, len(head_runs)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--base", required=True, help="mne_acq_bench of the base commit")
    parser.add_argument("--head", required=True, help="mne_acq_bench of the commit under test")
    parser.add_argument("--runs", type=int, default=5, help="alternating runs per build (default 5)")
    parser.add_argument("--confirm-runs", type=int, default=5,
                        help="further alternating runs per build when a metric regresses (default 5)")
    parser.add_argument("--tolerance", type=float, default=0.2, help="allowed relative slowdown (default 0.2)")
    parser.add_argument("--quick", action="store_true", help="pass --quick to the suite")
    parser.add_argument("--json", help="write the comparison as JSON")
    args = parser.parse_args(argv)

    rows, rounds = gate(args.base, args.head, args.runs, args.confirm_runs, args.tolerance, args.quick)
    if rounds > args.runs:
        print(f"A metric regressed after {args.runs} runs per build; {rounds} runs per build decide.")

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
