#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     September, 2026
#
#
# @brief    Tests for the API evidence audit (T0.3).
"""Tests for ``tools/quality/audit_api_evidence.py``."""

from __future__ import annotations

import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path

_SCRIPT_PATH = Path(__file__).resolve().parents[1] / "audit_api_evidence.py"
_spec = importlib.util.spec_from_file_location("audit_api_evidence", _SCRIPT_PATH)
assert _spec is not None and _spec.loader is not None
audit = importlib.util.module_from_spec(_spec)
sys.modules["audit_api_evidence"] = audit
_spec.loader.exec_module(audit)


def _write(root: Path, relative: str, text: str) -> None:
    path = root / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


class FixtureTree:
    """A minimal repository with one library, one example, and one test."""

    def __init__(self, root: Path) -> None:
        _write(root, "src/libraries/fiff/fiff_reader.h",
               "class FIFFSHARED_EXPORT FiffReader : public QObject\n{\n};\n"
               "struct FIFFSHARED_EXPORT FiffRecord\n{\n};\n"
               "FIFFSHARED_EXPORT int countTags(const QString& path);\n")
        _write(root, "src/libraries/fiff/fiff_utils.h", "namespace FIFFLIB { int helper(); }\n")
        _write(root, "src/libraries/disp/viewers/rawview.h", "class DISPSHARED_EXPORT RawView\n{\n};\n")
        _write(root, "src/examples/CMakeLists.txt", "add_subdirectory(ex_read)\n")
        _write(root, "src/examples/ex_read/main.cpp", "#include <fiff/fiff_reader.h>\n")
        _write(root, "src/examples/ex_orphan/main.cpp", "int main() {}\n")
        _write(root, "src/testframes/test_reader/CMakeLists.txt", "")
        registry = {
            "modules": {"fiff": {"dir_slug": "fiff"}, "disp": {"dir_slug": "disp"}},
            "classes": [
                {"name": "FiffReader", "module": "fiff", "header": "fiff/fiff_reader.h",
                 "test": "test_reader", "example": "ex_read", "python_equiv": "mne.io.read_raw_fif"},
                {"name": "FiffUtils", "module": "fiff", "header": "fiff/fiff_utils.h", "kind": "module",
                 "test": "test_gone", "example": "ex_orphan"},
                {"name": "FiffOld", "module": "fiff", "header": "fiff/fiff_old.h"},
            ],
        }
        _write(root, "doc/api_registry.json", json.dumps(registry))
        _write(root, "doc/website/docs/api/fiff/fiff-reader.mdx", "")


class AuditTests(unittest.TestCase):
    def build(self) -> dict:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            FixtureTree(root)
            return audit.build_report(root)

    def test_exports_and_free_functions_are_found(self) -> None:
        classes, functions = audit.exported_symbols(Path(__file__).resolve().parents[3] / "src" / "libraries")
        self.assertTrue(any(c["name"] == "FiffStream" for c in classes))
        self.assertTrue(any(f["name"] == "applyInverseRaw" for f in functions))

    def test_records_are_classified_by_kind(self) -> None:
        records = {r["name"]: r for r in self.build()["classes"]}
        self.assertEqual(records["FiffReader"]["kind"], "exported-class")
        self.assertEqual(records["FiffRecord"]["kind"], "exported-class")
        self.assertFalse(records["FiffRecord"]["registered"])
        self.assertEqual(records["FiffUtils"]["kind"], "function-module")
        self.assertEqual(records["FiffOld"]["kind"], "stale")

    def test_evidence_resolution(self) -> None:
        records = {r["name"]: r for r in self.build()["classes"]}
        reader = records["FiffReader"]
        self.assertTrue(reader["test_resolves"])
        self.assertEqual(reader["example_state"], "built")
        self.assertEqual(reader["including_examples"], ["ex_read"])
        self.assertEqual(reader["api_page"], "doc/website/docs/api/fiff/fiff-reader.mdx")
        utils = records["FiffUtils"]
        self.assertFalse(utils["test_resolves"])
        self.assertEqual(utils["example_state"], "not-built")
        self.assertIsNone(utils["api_page"])

    def test_exempt_views_leave_the_denominator_explicit(self) -> None:
        report = self.build()
        records = {r["name"]: r for r in report["classes"]}
        self.assertFalse(records["RawView"]["example_eligible"])
        self.assertIn("disp/viewers/", records["RawView"]["exempt_reason"])
        summary = report["summary"]
        self.assertEqual(summary["public_api_units"], 4)
        self.assertEqual(summary["example_eligible"] + summary["example_exempt"], summary["public_api_units"])
        self.assertEqual(summary["eligible_backed_by_registry_example"], 1)
        self.assertEqual(summary["registered_stale"], 1)
        self.assertEqual(summary["exported_free_functions"], 1)

    def test_repository_report_reconciles(self) -> None:
        summary = audit.build_report()["summary"]
        registered_exported = summary["exported_classes"] - summary["exported_not_registered"]
        self.assertEqual(
            registered_exported + summary["registered_function_modules"]
            + summary["registered_header_only"] + summary["registered_stale"],
            summary["registered_classes"],
        )


if __name__ == "__main__":
    unittest.main()
