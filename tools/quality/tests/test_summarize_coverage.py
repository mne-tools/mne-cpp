#!/usr/bin/env python3
# =============================================================================================================
#
# @file     test_summarize_coverage.py
# @author   MNE-CPP maintainers
# @since    2.4.0
# @date     August, 2026
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#
# @brief    Tests for the deterministic LCOV coverage baseline summarizer.
#
# =============================================================================================================
"""Tests for ``tools/quality/summarize_coverage.py``."""

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

_SCRIPT_PATH = Path(__file__).resolve().parents[1] / "summarize_coverage.py"
_spec = importlib.util.spec_from_file_location("summarize_coverage", _SCRIPT_PATH)
assert _spec is not None and _spec.loader is not None
coverage = importlib.util.module_from_spec(_spec)
sys.modules["summarize_coverage"] = coverage
_spec.loader.exec_module(coverage)


class CoverageTestCase(unittest.TestCase):
    def parse(self, contents: str):
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "coverage.info"
            path.write_text(contents, encoding="utf-8")
            return coverage.parse_lcov(path)


class TestLcovParsing(CoverageTestCase):
    def test_totals_reconcile(self) -> None:
        files = self.parse(
            "SF:/checkout/src/libraries/fiff/stream.cpp\n"
            "LF:10\nLH:7\nBRF:4\nBRH:3\nend_of_record\n"
            "SF:/checkout/src/tools/info/main.cpp\n"
            "LF:5\nLH:1\nBRF:2\nBRH:0\nend_of_record\n"
        )
        summary = coverage.build_summary(files, commit="abc", source_url=None, top=25)
        self.assertEqual((15, 8, 53.33), tuple(summary["totals"][key] for key in (
            "lines_found", "lines_hit", "line_percent"
        )))
        self.assertEqual((6, 3, 50.0), tuple(summary["totals"][key] for key in (
            "branches_found", "branches_hit", "branch_percent"
        )))

    def test_windows_paths_are_normalised(self) -> None:
        files = self.parse(
            "SF:C:\\work\\src\\applications\\mne_scan\\main.cpp\n"
            "LF:3\nLH:0\nend_of_record\n"
        )
        self.assertEqual("src/applications/mne_scan/main.cpp", files[0].path)
        self.assertEqual("src/applications/mne_scan", files[0].component)

    def test_duplicate_file_uses_best_observed_counts(self) -> None:
        files = self.parse(
            "SF:/a/src/libraries/mne/example.cpp\nLF:10\nLH:2\nend_of_record\n"
            "SF:/b/src/libraries/mne/example.cpp\nLF:10\nLH:8\nend_of_record\n"
        )
        self.assertEqual(1, len(files))
        self.assertEqual(8, files[0].lines_hit)

    def test_unterminated_record_fails(self) -> None:
        with self.assertRaisesRegex(ValueError, "unterminated LCOV record"):
            self.parse("SF:/checkout/src/libraries/fiff/stream.cpp\nLF:10\nLH:7\n")

    def test_out_of_scope_record_fails(self) -> None:
        with self.assertRaisesRegex(ValueError, "outside the measured production scope"):
            self.parse("SF:/checkout/src/testframes/test_fiff/test.cpp\nLF:1\nLH:1\nend_of_record\n")


class TestBaselineClassification(CoverageTestCase):
    def test_generated_candidates_remain_in_totals(self) -> None:
        files = self.parse(
            "SF:/checkout/src/tools/demo/demo_autogen/include/ui_dialog.h\n"
            "LF:20\nLH:0\nend_of_record\n"
            "SF:/checkout/src/tools/demo/main.cpp\nLF:10\nLH:5\nend_of_record\n"
        )
        summary = coverage.build_summary(files, commit=None, source_url=None, top=25)
        self.assertEqual(30, summary["totals"]["lines_found"])
        self.assertEqual(20, summary["generated_candidate_totals"]["lines_found"])
        self.assertEqual(10, summary["maintained_totals"]["lines_found"])
        self.assertEqual(
            ["src/tools/demo/main.cpp"],
            [item["path"] for item in summary["priority_files"]],
        )

    def test_priority_order_is_deterministic(self) -> None:
        files = self.parse(
            "SF:/checkout/src/libraries/mne/z.cpp\nLF:10\nLH:2\nend_of_record\n"
            "SF:/checkout/src/libraries/mne/a.cpp\nLF:10\nLH:2\nend_of_record\n"
            "SF:/checkout/src/libraries/mne/b.cpp\nLF:20\nLH:2\nend_of_record\n"
        )
        summary = coverage.build_summary(files, commit=None, source_url=None, top=2)
        self.assertEqual(
            ["src/libraries/mne/b.cpp", "src/libraries/mne/a.cpp"],
            [item["path"] for item in summary["priority_files"]],
        )


if __name__ == "__main__":
    unittest.main()