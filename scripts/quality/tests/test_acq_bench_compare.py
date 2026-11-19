#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    3.0.0
# @date     November, 2026
#
#
# @brief    Tests for scripts/ci/acq_bench/compare.py, the A/B regression gate of the acquisition suite.
"""The gate fails a gated metric that got more than the tolerance worse or a metric that misses its target."""

from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).resolve().parents[2] / "ci" / "acq_bench" / "compare.py"
_spec = importlib.util.spec_from_file_location("acq_bench_compare", _SCRIPT)
compare = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(compare)


def run(values: dict[str, float]) -> list[dict]:
    better = {"op": "lower", "ring": "higher", "wake": "lower"}
    return [
        {"name": n, "value": v, "unit": "x", "better": better[n.split("_")[0]]} for n, v in values.items()
    ]


class TestAcqBenchCompare(unittest.TestCase):
    def test_best_of_runs_per_direction(self) -> None:
        best = compare.best_of([run({"op_cost": 50, "ring_1": 2.0}), run({"op_cost": 40, "ring_1": 3.0})])
        self.assertEqual(best["op_cost"]["value"], 40)
        self.assertEqual(best["ring_1"]["value"], 3.0)

    def test_regression_beyond_tolerance_fails(self) -> None:
        base = compare.best_of([run({"op_cost": 50, "ring_1": 10.0})])
        head = compare.best_of([run({"op_cost": 61, "ring_1": 7.9})])
        rows = compare.compare(base, head, tolerance=0.2)
        failed = {r["name"] for r in rows if r["status"] == "REGRESSION"}
        self.assertEqual(failed, {"op_cost", "ring_1"})

    def test_within_tolerance_and_improvement_pass(self) -> None:
        base = compare.best_of([run({"op_cost": 50, "ring_1": 10.0})])
        head = compare.best_of([run({"op_cost": 59, "ring_1": 30.0})])
        self.assertFalse([r for r in compare.compare(base, head, tolerance=0.2) if r["status"] == "REGRESSION"])

    def test_latency_is_reported_not_gated(self) -> None:
        base = compare.best_of([run({"wake_spin_p99": 100})])
        head = compare.best_of([run({"wake_spin_p99": 1000})])
        rows = compare.compare(base, head, tolerance=0.2)
        self.assertEqual(rows[0]["status"], "report")

    def test_lost_samples_and_new_metrics(self) -> None:
        base = compare.best_of([run({"ring_1": 10.0})])
        head = compare.best_of([run({"ring_1": -1.0, "op_new": 5})])
        rows = {r["name"]: r["status"] for r in compare.compare(base, head, tolerance=0.2)}
        self.assertEqual(rows, {"ring_1": "LOST", "op_new": "new"})

    def test_missed_target_fails_even_when_reported_only(self) -> None:
        base = compare.best_of([run({"wake_spin_p99": 100, "ring_1": 2.0, "ring_2": 2.0})])
        head = run({"wake_spin_p99": 60000, "ring_1": 1.9, "ring_2": 2.1})
        targets = {"wake_spin_p99": 50000, "ring_1": 2.0, "ring_2": 2.0}
        for m in head:
            m["target"] = targets[m["name"]]
        rows = {r["name"]: r["status"] for r in compare.compare(base, compare.best_of([head]), tolerance=0.2)}
        self.assertEqual(rows, {"wake_spin_p99": "TARGET", "ring_1": "TARGET", "ring_2": "ok"})


if __name__ == "__main__":
    unittest.main()
