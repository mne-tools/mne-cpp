#!/usr/bin/env python3
# =============================================================================================================
#
# @file     test_audit_parity_baseline.py
# @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
# @since    2.4.0
# @date     September, 2026
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#
# @brief    Tests for the parity evidence baseline (T0.6).
#
# =============================================================================================================
"""Tests for ``tools/quality/audit_parity_baseline.py``."""

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

_SCRIPT_PATH = Path(__file__).resolve().parents[1] / "audit_parity_baseline.py"
_spec = importlib.util.spec_from_file_location("audit_parity_baseline", _SCRIPT_PATH)
assert _spec is not None and _spec.loader is not None
audit = importlib.util.module_from_spec(_spec)
sys.modules["audit_parity_baseline"] = audit
_spec.loader.exec_module(audit)


class ClassifierTests(unittest.TestCase):
    def classify(self, sources: dict[str, str]) -> dict[str, str]:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for name, text in sources.items():
                (root / name).mkdir()
                (root / name / f"{name}.cpp").write_text(text, encoding="utf-8")
            return audit.test_evidence(root)

    def test_levels(self) -> None:
        levels = self.classify({
            "test_static": "// Reference values were produced with mne.read_evokeds\nQCOMPARE(n, 55);",
            "test_live": 'QProcess p; p.start("python3", {"-c", "import mne"});',
            "test_helper": "#include <utils/python_test_helper.h>\nPythonTestHelper h;",
            "test_inspired": "// Inspired by mne-python test_surface.py\nQVERIFY(ok);",
            "test_record": 'MnaScript s; s.interpreter = "python3";\n\nQCOMPARE(s.language, x);',
            "test_plain": "QVERIFY(true);",
        })
        self.assertEqual(levels, {
            "test_helper": "cross-validated-live",
            "test_inspired": "tested",
            "test_live": "cross-validated-live",
            "test_plain": "tested",
            "test_record": "tested",
            "test_static": "cross-validated-static",
        })

    def test_percent_claims(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "README.md").write_text("Covers ~92 % of MNE-Python's core.\nFast.\n", encoding="utf-8")
            claims = audit.percent_claims(root)
        self.assertEqual([(c["line"], c["status"]) for c in claims], [(1, "unsupported")])


class RepositoryTests(unittest.TestCase):
    def test_every_claim_has_an_evidence_level(self) -> None:
        report = audit.build_report()
        self.assertEqual(sum(report["summary"]["by_evidence"].values()), report["summary"]["claims"])
        self.assertEqual(report["reference_environment"]["registry_mne_python_ref"], "1.11.0")


if __name__ == "__main__":
    unittest.main()
