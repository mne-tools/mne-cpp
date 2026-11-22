# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Tests for scripts/compliance/traceability.py.

@since 3.0.0
"""

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).resolve().parents[2] / "compliance" / "traceability.py"
_spec = importlib.util.spec_from_file_location("traceability", _SCRIPT)
trace = importlib.util.module_from_spec(_spec)
sys.modules["traceability"] = trace
_spec.loader.exec_module(trace)


class TestTraceability(unittest.TestCase):
    def test_matrix_counts_and_rules(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "src" / "testframes" / "test_a").mkdir(parents=True)
            registry = {"classes": [{"name": "A", "module": "m", "test": "test_a"},
                                    {"name": "B", "module": "m", "test": "test_gone"},
                                    {"name": "C", "module": "m", "test": None}]}
            report = trace.matrix(registry, root)
            module = report["modules"]["m"]
            self.assertEqual((module["classes"], module["tested"], module["untested"]), (3, 2, ["C"]))
            self.assertEqual(trace.problems(report, {"floors": {"m": 3}, "safety_relevant": []}), [
                "m: B -> test_gone does not exist in src/testframes",
                "m: 2 classes with a test, below its floor of 3"])
            page = trace.render_mdx(report, {"floors": {}, "safety_relevant": ["m"]})
            self.assertIn("| m | yes | 3 | 2 | 67 % |", page)

    def test_committed_files_are_current(self):
        self.assertEqual(trace.main(["--check"]), 0)


if __name__ == "__main__":
    unittest.main()
