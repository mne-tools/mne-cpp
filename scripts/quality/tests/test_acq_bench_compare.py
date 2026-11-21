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


    def test_a_regression_must_survive_a_second_round(self) -> None:
        # A shared runner has slow minutes: the head's first rounds all landed in one, the later ones did not
        calls = {"base": 0, "head": 0}

        def bench(binary: str, quick: bool) -> list[dict]:
            calls[binary] += 1
            slow = binary == "head" and calls[binary] <= 5
            return run({"ring_1": 2.2 if slow else 4.4})

        rows, rounds = compare.gate("base", "head", runs=5, confirm_runs=5, tolerance=0.2, quick=False, bench=bench)
        self.assertEqual(rows[0]["status"], "ok")
        self.assertEqual(rounds, 10)
        self.assertEqual(rows[0]["head_runs"], [2.2] * 5 + [4.4] * 5)

    def test_a_lasting_regression_still_fails(self) -> None:
        def bench(binary: str, quick: bool) -> list[dict]:
            return run({"ring_1": 2.2 if binary == "head" else 4.4, "op_cost": 50})

        rows, rounds = compare.gate("base", "head", runs=5, confirm_runs=5, tolerance=0.2, quick=False, bench=bench)
        self.assertEqual({r["name"]: r["status"] for r in rows}, {"ring_1": "REGRESSION", "op_cost": "ok"})
        self.assertEqual(rounds, 10)

    def test_a_rare_fast_run_of_the_base_is_no_regression(self) -> None:
        # CI run 44904077618: the base hit the machine's fast phase in two of ten rounds, the head in none.
        # Each round runs both builds back to back, so the ratio within a round cancels the phase.
        base = iter([1.51, 1.14, 1.08, 1.16, 1.75, 1.46, 1.26, 1.15, 1.19, 1.76])
        head = iter([1.25, 1.09, 1.19, 1.18, 1.2, 1.09, 1.23, 1.16, 1.19, 1.12])

        def bench(binary: str, quick: bool) -> list[dict]:
            return run({"ring_dma": next(base if binary == "base" else head)})

        rows, rounds = compare.gate("base", "head", runs=5, confirm_runs=5, tolerance=0.2, quick=False, bench=bench)
        self.assertEqual(rows[0]["status"], "ok")
        self.assertEqual(rounds, 5)
        self.assertEqual((rows[0]["base"], rows[0]["head"]), (1.75, 1.25))  # the best values are still reported

    def test_no_second_round_without_a_regression(self) -> None:
        rows, rounds = compare.gate("base", "head", runs=3, confirm_runs=5, tolerance=0.2, quick=False,
                                    bench=lambda binary, quick: run({"ring_1": 4.4}))
        self.assertEqual(rounds, 3)


if __name__ == "__main__":
    unittest.main()
