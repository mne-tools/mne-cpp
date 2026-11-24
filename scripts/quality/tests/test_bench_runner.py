# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Tests for scripts/bench/run_benchmarks.py.

@since 3.0.0
"""

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).resolve().parents[2] / "bench" / "run_benchmarks.py"
_spec = importlib.util.spec_from_file_location("run_benchmarks", _SCRIPT)
bench = importlib.util.module_from_spec(_spec)
sys.modules["run_benchmarks"] = bench
_spec.loader.exec_module(bench)


def metrics(*names: str, value: float = 1.0) -> dict:
    return {name: {"name": name, "value": value} for name in names}


FULL = metrics(*[f"ring_int16_{n}_ch" for n in (1, 64, 1024, 4096)],
               *[f"wake_{r}_{p}" for r in ("spin", "output", "block") for p in ("p50", "p99", "p999")],
               "paced_float32_64_ch_1_khz", "paced_float32_1000_ch_20_khz", "record_float32_256_ch_20_khz")


class TestAcqCharts(unittest.TestCase):
    def test_every_metric_is_charted(self):
        charts = {c["id"]: c for c in bench.acq_charts(FULL)}
        self.assertEqual([p["x"] for p in charts["acq_throughput"]["series"][0]["points"]],
                         ["1 channel", "64 channels", "1024 channels", "4096 channels"])
        self.assertEqual(len(charts["acq_paced"]["series"][0]["points"]), 3)
        self.assertNotIn("notes", charts["acq_paced"])

    def test_a_build_without_fiff_recording_charts_the_rest(self):
        # scripts/ci/acq_bench builds the tool without the FIFF bridge, so it measures no recording
        partial = {k: v for k, v in FULL.items() if not k.startswith("record_")}
        paced = {c["id"]: c for c in bench.acq_charts(partial)}["acq_paced"]
        self.assertEqual([p["x"] for p in paced["series"][0]["points"]], ["64 ch × 1 kHz", "1000 ch × 20 kHz"])

    def test_a_loss_becomes_a_note(self):
        lossy = {**FULL, **metrics("paced_int16_4_ch_62500_khz", value=-1.0)}
        paced = {c["id"]: c for c in bench.acq_charts(lossy)}["acq_paced"]
        self.assertEqual(paced["notes"], ["Lost samples on this machine: 4 ch × 62.5 MHz int16."])


class TestBinary(unittest.TestCase):
    def test_the_given_build_directory_comes_first(self):
        with tempfile.TemporaryDirectory() as tmp:
            tool = Path(tmp) / "mne_acq_bench"
            tool.write_text("")
            self.assertEqual(bench.binary(Path(tmp), "mne_acq_bench"), tool)


if __name__ == "__main__":
    unittest.main()
