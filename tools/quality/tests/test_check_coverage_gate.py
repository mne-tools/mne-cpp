#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     September, 2026
#
#
# @brief    Fixtures for the coverage gate and its CI wiring (T2.1).
"""Fixtures for ``tools/quality/check_coverage_gate.py`` and the workflows that call it."""

from __future__ import annotations

import importlib.util
import io
import json
import re
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

_QUALITY_DIR = Path(__file__).resolve().parent.parent
_REPO_ROOT = _QUALITY_DIR.parents[1]
_WORKFLOWS = _REPO_ROOT / ".github" / "workflows"

_spec = importlib.util.spec_from_file_location("check_coverage_gate", _QUALITY_DIR / "check_coverage_gate.py")
assert _spec is not None and _spec.loader is not None
gate = importlib.util.module_from_spec(_spec)
sys.modules["check_coverage_gate"] = gate
_spec.loader.exec_module(gate)


def _lcov(hit: int, missed: int) -> str:
    records = [f"DA:{i},3" for i in range(1, hit + 1)] + [f"DA:{i},0" for i in range(hit + 1, hit + missed + 1)]
    return "SF:/src/libraries/a.cpp\n" + "\n".join(records) + "\nend_of_record\n"


class GateTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.root = Path(self._tmp.name)
        self.policy = self.root / "policy.json"
        self.policy.write_text(json.dumps({"line_floor_percent": 50.0, "max_regression_points": 0.1}), encoding="utf-8")
        self.baseline = self.root / ".coverage-baseline"

    def run_gate(self, hit: int, missed: int) -> tuple[int, str]:
        lcov = self.root / "coverage.info"
        lcov.write_text(_lcov(hit, missed), encoding="utf-8")
        out = io.StringIO()
        with redirect_stdout(out), redirect_stderr(io.StringIO()):
            code = gate.main([str(lcov), "--baseline", str(self.baseline), "--policy", str(self.policy)])
        return code, out.getvalue()


class TestGate(GateTestCase):
    def test_log_reports_the_enforced_numbers(self) -> None:
        """AC-T2.1-1: the log states the floor and tolerance the policy configures."""
        code, text = self.run_gate(60, 40)
        self.assertEqual(0, code)
        self.assertIn("Line coverage        : 60.00% (60 / 100 lines)", text)
        self.assertIn("Floor                : 50.00%", text)
        self.assertIn("Regression tolerance : 0.10 points", text)

    def test_below_floor_fails(self) -> None:
        code, text = self.run_gate(49, 51)
        self.assertEqual(1, code)
        self.assertIn("below the floor of 50.00%", text)

    def test_second_run_compares_against_the_first(self) -> None:
        """AC-T2.1-2: a passing run leaves a baseline that the next run is held to."""
        self.assertEqual(0, self.run_gate(60, 40)[0])
        self.assertEqual("60.00", self.baseline.read_text(encoding="utf-8").strip())
        code, text = self.run_gate(598, 402)
        self.assertEqual(1, code)
        self.assertIn("Previous baseline    : 60.00%", text)
        self.assertIn("dropped 0.20 points", text)

    def test_drop_within_tolerance_passes(self) -> None:
        self.baseline.write_text("60.00\n", encoding="utf-8")
        self.assertEqual(0, self.run_gate(5995, 4005)[0])

    def test_failing_run_does_not_lower_the_baseline(self) -> None:
        self.baseline.write_text("60.00\n", encoding="utf-8")
        self.assertEqual(1, self.run_gate(55, 45)[0])
        self.assertEqual("60.00", self.baseline.read_text(encoding="utf-8").strip())

    def test_empty_report_is_an_error(self) -> None:
        lcov = self.root / "empty.info"
        lcov.write_text("SF:/a.cpp\nend_of_record\n", encoding="utf-8")
        with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            self.assertEqual(2, gate.main([str(lcov), "--policy", str(self.policy)]))


class TestPolicy(unittest.TestCase):
    def test_policy_is_consistent_with_release_gate_g2(self) -> None:
        policy = gate.load_policy(gate.POLICY_FILE)
        self.assertGreater(policy["line_floor_percent"], 20.0)
        self.assertLessEqual(policy["line_floor_percent"], 60.0)
        self.assertEqual(0.1, policy["max_regression_points"])


# The lint jobs run on a bare interpreter, so read the few keys under test without PyYAML.
def _block(text: str, header: str) -> str:
    """The lines indented under the first line equal to ``header`` (stripped)."""
    lines = text.splitlines()
    for index, line in enumerate(lines):
        if line.strip() == header:
            indent = len(line) - len(line.lstrip())
            body = []
            for following in lines[index + 1:]:
                if following.strip() and len(following) - len(following.lstrip()) <= indent:
                    break
                body.append(following)
            return "\n".join(body)
    raise AssertionError(f"no line {header!r}")


def _step(name: str) -> str:
    text = (_WORKFLOWS / "_reusable-tests.yml").read_text(encoding="utf-8")
    return _block(text, f"- name: {name}")


def _value(block: str, key: str) -> str | None:
    match = re.search(rf"^\s*{re.escape(key)}:\s*(.*?)\s*$", block, re.MULTILINE)
    return match.group(1) if match else None


class TestWorkflowWiring(unittest.TestCase):
    def test_release_branches_cannot_disable_coverage(self) -> None:
        """AC-T2.1-3: staging and main pass literal true, with no commit-message override."""
        for name in ("staging.yml", "main.yml"):
            with self.subTest(workflow=name):
                text = (_WORKFLOWS / name).read_text(encoding="utf-8")
                call = _block(_block(text, "Tests:"), "with:")
                for key in ("with_code_coverage", "upload_codecov", "enforce_coverage_gate"):
                    self.assertEqual("true", _value(call, key), f"{name}: {key} must be the literal true")
                self.assertNotIn("skip-coverage", text)

    def test_restore_prefix_matches_saved_keys(self) -> None:
        """AC-T2.1-2: the saved key starts with the restore prefix, so the next run finds it."""
        save_key = _value(_step("Save coverage baseline"), "key")
        restore = _step("Restore previous coverage baseline")
        prefixes = [line.strip() for line in _block(restore, "restore-keys: |").splitlines() if line.strip()]
        self.assertTrue(save_key and prefixes)
        for prefix in prefixes:
            self.assertTrue(save_key.startswith(prefix), f"{save_key!r} does not start with {prefix!r}")

    def test_gate_uses_the_policy_script(self) -> None:
        run = _value(_step("Check coverage gate"), "run") or ""
        self.assertIn("tools/quality/check_coverage_gate.py", run)
        self.assertIsNone(re.search(r"\bbc\b|ALLOWED_DROP|THRESHOLD", run))


if __name__ == "__main__":
    unittest.main()
