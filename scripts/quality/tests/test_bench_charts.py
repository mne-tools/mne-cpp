# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Tests for scripts/bench/render_charts.py.

@since 3.0.0
"""

import importlib.util
import sys
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

_SCRIPT = Path(__file__).resolve().parents[2] / "bench" / "render_charts.py"
_spec = importlib.util.spec_from_file_location("render_charts", _SCRIPT)
charts = importlib.util.module_from_spec(_spec)
sys.modules["render_charts"] = charts
_spec.loader.exec_module(charts)

RESULTS = {
    "environment": {"machine": "CPU, 8 cores, 16 GB", "os": "Linux", "compiler": "GCC 14 Release",
                    "commit": "abc1234567", "date": "2026-11-22", "reference": "MNE-Python 1.11"},
    "charts": [{"id": "forward", "title": "Forward <solution>", "unit": "s", "higher_is_better": False,
                "command": "run_benchmarks.py --suites forward",
                "series": [{"label": "MNE-CPP", "points": [{"x": "oct-6", "y": 2.0}]},
                           {"label": "MNE-Python", "points": [{"x": "oct-6", "y": 8.0}]}]}],
}


class TestRenderCharts(unittest.TestCase):
    def test_nice_ceiling(self):
        self.assertEqual([charts.nice_ceiling(v) for v in (0.7, 3, 7, 12, 99)], [1, 5, 10, 20, 100])

    def test_chart_is_valid_svg_with_provenance(self):
        svg = charts.render(RESULTS)["forward.svg"]
        root = ET.fromstring(svg)
        texts = " ".join(t.text or "" for t in root.iter("{http://www.w3.org/2000/svg}text"))
        self.assertIn("Forward <solution>", texts)
        for fact in ("CPU, 8 cores", "GCC 14 Release", "abc1234567", "MNE-Python 1.11", "run_benchmarks.py --suites forward",
                     "lower is better"):
            self.assertIn(fact, texts)
        bars = [r for r in root.iter("{http://www.w3.org/2000/svg}rect") if r.find("{http://www.w3.org/2000/svg}title") is not None]
        widths = [float(r.get("width")) for r in bars]
        self.assertAlmostEqual(widths[1] / widths[0], 4.0, places=3)

    def test_a_loss_is_shown_on_the_chart_and_the_page(self):
        results = {**RESULTS, "charts": [{**RESULTS["charts"][0], "notes": ["Lost samples on this machine: 4 ch."]}]}
        svg = charts.render(results)["forward.svg"]
        self.assertIn("Lost samples on this machine: 4 ch.", svg)
        ET.fromstring(svg)
        page = charts.render_page(results, "# Performance")
        self.assertIn(":::caution\nLost samples on this machine: 4 ch.\n:::", page)

    def test_page_tables_every_number_and_names_the_test_system(self):
        page = charts.render_page(RESULTS, "# Performance")
        self.assertIn("| oct-6 | 2.00 s | 8.00 s |", page)
        # MDX reads < and { as JSX
        self.assertIn("## Forward &lt;solution&gt;", page)
        self.assertIn("| MNE-CPP commit | `abc1234567` |", page)
        self.assertIn("Reproduce: `run_benchmarks.py --suites forward`", page)

    def test_rendering_is_deterministic(self):
        self.assertEqual(charts.render(RESULTS), charts.render(RESULTS))


if __name__ == "__main__":
    unittest.main()
