#!/usr/bin/env python3
# =============================================================================================================
#
# @file     test_audit_tests.py
# @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
# @since    2.4.0
# @date     September, 2026
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#
# @brief    Tests for the test inventory audit (T0.1).
#
# =============================================================================================================
"""Tests for ``tools/quality/audit_tests.py``."""

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

_SCRIPT_PATH = Path(__file__).resolve().parents[1] / "audit_tests.py"
_spec = importlib.util.spec_from_file_location("audit_tests", _SCRIPT_PATH)
assert _spec is not None and _spec.loader is not None
audit = importlib.util.module_from_spec(_spec)
sys.modules["audit_tests"] = audit
_spec.loader.exec_module(audit)


def _write(directory: Path, name: str, text: str) -> Path:
    path = directory / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")
    return path


class ConditionTests(unittest.TestCase):
    def test_nested_else_and_elseif_conditions_are_attributed(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            cmake = _write(Path(temp), "CMakeLists.txt", "\n".join([
                "add_subdirectory(test_plain)",
                "if(BUILD_A)  # comment",
                "  add_subdirectory(test_a)",
                "  if(WIN32)",
                "    add_subdirectory(test_a_win)",
                "  endif()",
                "elseif(BUILD_B)",
                "  add_subdirectory(test_b)",
                "else()",
                "  add_subdirectory(test_neither)",
                "endif()",
            ]))
            conditions = audit.conditional_subdirectories(cmake)
        self.assertEqual(conditions["test_plain"], [])
        self.assertEqual(conditions["test_a"], ["BUILD_A"])
        self.assertEqual(conditions["test_a_win"], ["BUILD_A", "WIN32"])
        self.assertEqual(conditions["test_b"], ["NOT (BUILD_A) AND (BUILD_B)"])
        self.assertEqual(conditions["test_neither"], ["NOT (NOT (BUILD_A) AND (BUILD_B))"])


class WorkflowTests(unittest.TestCase):
    def test_platform_exclusion_is_attributed_to_its_step(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            workflow = _write(Path(temp), "tests.yml", "\n".join([
                "steps:",
                "    - name: Run tests (Linux)",
                "      if: matrix.os == 'ubuntu-24.04'",
                "      run: ctest --test-dir build",
                "    - name: Run tests (Windows)",
                "      if: matrix.os == 'windows-2025'",
                "      run: |",
                "        ctest --test-dir build `",
                "              -E \"test_one|test_two\"",
            ]))
            excluded = audit.platform_exclusions(workflow)
        self.assertEqual(excluded, {"test_one": ["windows-2025"], "test_two": ["windows-2025"]})

    def test_retry_paths_are_found_and_ctest_without_retry_is_not(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            _write(root, "scripts/test/run.sh", "ctest\n# Re-run failed tests once\n")
            _write(root, ".github/workflows/ci.yml", "run: ctest --output-on-failure\n")
            hits = audit.retry_paths(root)
        self.assertEqual([(h["path"], h["line"]) for h in hits], [("scripts/test/run.sh", 2)])


class JunitTests(unittest.TestCase):
    def test_failures_and_skips_are_counted(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            path = _write(Path(temp), "results.xml", "\n".join([
                "<testsuite tests='3'>",
                "  <testcase name='test_ok' status='run' time='1.25'/>",
                "  <testcase name='test_bad' status='fail' time='2'><failure message='x'/></testcase>",
                "  <testcase name='test_off' status='notrun' time='0'><skipped/></testcase>",
                "</testsuite>",
            ]))
            parsed = audit.parse_junit(path)
        self.assertEqual((parsed["tests"], parsed["failures"], parsed["skipped"]), (3, 1, 1))
        self.assertEqual(list(parsed["results"]), ["test_bad", "test_off", "test_ok"])
        self.assertEqual(parsed["results"]["test_ok"], {"status": "run", "seconds": 1.2})

    def test_import_rejects_missing_platform(self) -> None:
        with self.assertRaises(ValueError):
            audit.import_junit(["results.xml"], None, None)


class RepositoryTests(unittest.TestCase):
    def test_inventory_is_byte_stable_and_complete(self) -> None:
        evidence = audit.load_ci_evidence(audit.CI_EVIDENCE)
        first = audit.dump_json(audit.build_inventory(audit.REPO_ROOT, evidence))
        second = audit.dump_json(audit.build_inventory(audit.REPO_ROOT, evidence))
        self.assertEqual(first, second)

        report = audit.build_inventory(audit.REPO_ROOT, evidence)
        directories = sorted(p.name for p in audit.TESTFRAMES_DIR.iterdir()
                             if p.is_dir() and p.name.startswith("test_"))
        self.assertEqual([record["name"] for record in report["tests"]], directories)
        self.assertEqual(sum(report["summary"]["registration"].values()), len(directories))


if __name__ == "__main__":
    unittest.main()
