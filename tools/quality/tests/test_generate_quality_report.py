#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     September, 2026
#
#
# @brief    Tests for the combined quality baseline (T0.7).
"""Tests for ``tools/quality/generate_quality_report.py``."""

from __future__ import annotations

import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path

_SCRIPT_PATH = Path(__file__).resolve().parents[1] / "generate_quality_report.py"
_spec = importlib.util.spec_from_file_location("generate_quality_report", _SCRIPT_PATH)
assert _spec is not None and _spec.loader is not None
report = importlib.util.module_from_spec(_spec)
sys.modules["generate_quality_report"] = report
_spec.loader.exec_module(report)


class DashboardTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp)
            cls.exit_code = report.main(["--out-dir", str(out)])
            cls.files = {path.name: path.read_text(encoding="utf-8") for path in out.iterdir()}

    def test_one_command_writes_every_report(self) -> None:
        self.assertEqual(self.exit_code, 0)
        for stem in ("test-inventory", "api-evidence-baseline", "visual-baseline",
                     "maintainability-baseline", "parity-baseline", "quality-baseline"):
            self.assertIn(f"{stem}.json", self.files)
            self.assertIn(f"{stem}.md", self.files)
        self.assertIn("coverage-baseline.md", self.files)

    def test_dashboard_totals_match_the_section_reports(self) -> None:
        gates = json.loads(self.files["quality-baseline.json"])["gates"]
        tests = json.loads(self.files["test-inventory.json"])["summary"]
        api = json.loads(self.files["api-evidence-baseline.json"])["summary"]
        parity = json.loads(self.files["parity-baseline.json"])["summary"]
        self.assertEqual(gates["G1_test_integrity"]["without_labels"], tests["without_labels"])
        self.assertEqual(gates["G3_api_documentation"]["example_eligible"], api["example_eligible"])
        self.assertEqual(sum(gates["G6_parity"]["claims_by_evidence"].values()), parity["claims"])
        coverage = json.loads((report.RELEASE_DIR / "coverage-baseline.json").read_text(encoding="utf-8"))
        self.assertEqual(gates["G2_coverage"]["line_combined"], coverage["totals"]["line_percent"])

    def test_check_detects_a_stale_report(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp)
            self.assertEqual(report.main(["--out-dir", str(out)]), 0)
            self.assertEqual(report.main(["--out-dir", str(out), "--check"]), 0)
            (out / "visual-baseline.md").write_text("edited by hand\n", encoding="utf-8")
            self.assertEqual(report.main(["--out-dir", str(out), "--check"]), 1)


if __name__ == "__main__":
    unittest.main()
