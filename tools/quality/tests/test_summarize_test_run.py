#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     September, 2026
#
#
# @brief    Fixtures for the CTest JUnit run summary.
"""Fixtures for ``tools/quality/summarize_test_run.py``."""

from __future__ import annotations

import importlib.util
import io
import json
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path

_MODULE_PATH = Path(__file__).resolve().parent.parent / "summarize_test_run.py"
_spec = importlib.util.spec_from_file_location("summarize_test_run", _MODULE_PATH)
assert _spec is not None and _spec.loader is not None
str_ = importlib.util.module_from_spec(_spec)
sys.modules["summarize_test_run"] = str_
_spec.loader.exec_module(str_)

JUNIT = """<?xml version="1.0" encoding="UTF-8"?>
<testsuite name="(empty)" tests="5">
  <testcase name="test_pass" status="run"><system-out>PASS   : A::x()</system-out></testcase>
  <testcase name="test_qskip" status="run"><system-out>PASS   : B::a()
SKIP   : B::full() FreeSurfer not available
   Loc: [/src/b.cpp(12)]
SKIP   : B::bare()
   Loc: [/src/b.cpp(20)]
</system-out></testcase>
  <testcase name="test_target" status="notrun"><skipped message="SKIP_REGULAR_EXPRESSION_MATCHED"/>
    <system-out>MNE_TEST_SKIPPED: this configuration does not build scan_writetofile
</system-out></testcase>
  <testcase name="test_data" status="notrun"><skipped message="Required Files Missing"/>
    <system-out>Unable to find required file: /data/x.fif</system-out></testcase>
  <testcase name="test_fail" status="fail"><failure message="Failed"/></testcase>
</testsuite>
"""


class SummarizeTestRunTest(unittest.TestCase):
    def setUp(self) -> None:
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.root = Path(self._tmp.name)
        self.junit = self.root / "results.xml"
        self.junit.write_text(JUNIT, encoding="utf-8")
        self.policy = self.root / "policy.json"
        self.policy.write_text(json.dumps({"quarantine": []}), encoding="utf-8")

    def run_main(self, *extra: str) -> tuple[int, str]:
        out = io.StringIO()
        with redirect_stdout(out):
            code = str_.main([str(self.junit), "--platform", "fixture", "--policy", str(self.policy), *extra])
        return code, out.getvalue()

    def test_every_kind_of_skip_is_listed_with_its_reason(self) -> None:
        run = str_.parse_junit(self.junit)
        self.assertEqual(5, run.total)
        self.assertEqual(["test_fail"], run.failed)
        self.assertEqual(
            [("test_target", "this configuration does not build scan_writetofile"),
             ("test_data", "Required Files Missing: Unable to find required file: /data/x.fif")],
            run.not_run,
        )
        self.assertEqual(
            [("test_qskip", "B::full()", "FreeSurfer not available"), ("test_qskip", "B::bare()", "")],
            run.qskips,
        )

    def test_summary_names_the_counts(self) -> None:
        code, text = self.run_main()
        self.assertEqual(0, code)
        self.assertIn("5 registered, 1 failed, 2 not run, 2 QSKIP'd functions in passing tests, 0 quarantined.", text)
        self.assertIn("**no reason given**", text)

    def test_a_skip_without_a_reason_fails_when_reasons_are_required(self) -> None:
        code, _ = self.run_main("--require-reasons")
        self.assertEqual(1, code)

    def test_quarantined_tests_are_listed(self) -> None:
        self.policy.write_text(json.dumps({"quarantine": [
            {"test": "test_fail", "owner": "a <a@b>", "issue": "https://x/1", "expires": "2099-01-01", "reason": "r"}
        ]}), encoding="utf-8")
        _, text = self.run_main()
        self.assertIn("| `test_fail` | a <a@b> | 2099-01-01 | https://x/1 |", text)

    def test_unreadable_file_is_a_usage_error(self) -> None:
        self.junit.write_text("<not xml", encoding="utf-8")
        with redirect_stdout(io.StringIO()):
            self.assertEqual(2, str_.main([str(self.junit)]))


if __name__ == "__main__":
    unittest.main()
