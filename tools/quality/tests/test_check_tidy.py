#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     October, 2026
#
#
# @brief    Fixtures for the clang-tidy ratchet (T6.3).
"""Fixtures for ``check_tidy.py``, ``.clang-tidy`` and ``tidy_baseline.json``."""

from __future__ import annotations

import importlib.util
import json
import re
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

_QUALITY_DIR = Path(__file__).resolve().parent.parent
_REPO_ROOT = _QUALITY_DIR.parents[1]
_spec = importlib.util.spec_from_file_location("check_tidy", _QUALITY_DIR / "check_tidy.py")
assert _spec is not None and _spec.loader is not None
tidy = importlib.util.module_from_spec(_spec)
sys.modules["check_tidy"] = tidy
_spec.loader.exec_module(tidy)


class RatchetTests(unittest.TestCase):
    """AC-T6.3-1: a new violation fails; accepted debt does not."""

    def test_header_findings_count_once(self) -> None:
        findings = [("src/a.h", 3, 1, "bugprone-x"), ("src/a.h", 3, 1, "bugprone-x"), ("src/a.cpp", 9, 2, "bugprone-x")]
        self.assertEqual(tidy.count(findings), {"src/a.cpp": {"bugprone-x": 1}, "src/a.h": {"bugprone-x": 1}})

    def test_new_finding_fails_and_debt_passes(self) -> None:
        baseline = {"clean_modules": [], "findings": {"src/a.cpp": {"bugprone-x": 2}}}
        self.assertEqual(tidy.compare({"src/a.cpp": {"bugprone-x": 2}}, baseline, {"src/a.cpp"}), [])
        self.assertEqual(tidy.compare({"src/a.cpp": {"bugprone-x": 1}}, baseline, {"src/a.cpp"}), [])
        self.assertTrue(tidy.compare({"src/a.cpp": {"bugprone-x": 3}}, baseline, {"src/a.cpp"}))
        self.assertTrue(tidy.compare({"src/a.cpp": {"performance-y": 1}}, baseline, {"src/a.cpp"}))
        self.assertTrue(tidy.compare({"src/b.cpp": {"bugprone-x": 1}}, baseline, {"src/b.cpp"}))

    def test_clean_modules_allow_nothing(self) -> None:
        baseline = {"clean_modules": ["src/libraries/fiff/"],
                    "findings": {"src/libraries/fiff/a.cpp": {"bugprone-x": 5}}}
        problems = tidy.compare({"src/libraries/fiff/a.cpp": {"bugprone-x": 1}}, baseline,
                                {"src/libraries/fiff/a.cpp"})
        self.assertEqual(problems, ["src/libraries/fiff/a.cpp: bugprone-x: 1 finding(s), 0 accepted"])

    def test_findings_are_parsed_per_check(self) -> None:
        output = (f"{_REPO_ROOT}/src/libraries/fiff/a.cpp:12:5: warning: text [bugprone-x,cert-y]\n"
                  f"{_REPO_ROOT}/src/external/eigen/x.h:1:1: warning: text [bugprone-x]\n")
        matches = [(tidy.relative(m.group("file")), m.group("checks")) for m in tidy.FINDING_RE.finditer(output)]
        self.assertEqual(matches[0], ("src/libraries/fiff/a.cpp", "bugprone-x,cert-y"))
        self.assertFalse(tidy.in_scope(matches[1][0]))


class SuppressionTests(unittest.TestCase):
    """AC-T6.3-2: a suppression needs a check name and a reason."""

    def setUp(self) -> None:
        self.tmp = Path(tempfile.mkdtemp(dir=_REPO_ROOT / "tools" / "quality" / "tests"))

    def tearDown(self) -> None:
        shutil.rmtree(self.tmp)

    def problems(self, line: str) -> list[str]:
        file = self.tmp / "a.cpp"
        file.write_text(f"int x = 0; {line}\n", encoding="utf-8")
        return tidy.suppression_problems([file.relative_to(_REPO_ROOT).as_posix()])

    def test_rules(self) -> None:
        self.assertTrue(self.problems("// NOLINT"))
        self.assertTrue(self.problems("// NOLINT(bugprone-x)"))
        self.assertTrue(self.problems("// NOLINTNEXTLINE: reason without check"))
        self.assertEqual(self.problems("// NOLINT(bugprone-x): owned by Qt parent"), [])
        self.assertEqual(self.problems("// NOLINTNEXTLINE(bugprone-x, performance-y): FIFF layout"), [])
        self.assertEqual(self.problems("// unrelated comment"), [])


class ConfigTests(unittest.TestCase):
    def test_disabled_checks_have_reasons(self) -> None:
        config = (_REPO_ROOT / ".clang-tidy").read_text(encoding="utf-8")
        disabled = re.findall(r"^\s+-([a-z][\w.-]+),?$", config, re.MULTILINE)
        self.assertTrue(disabled)
        for check in disabled:
            self.assertRegex(config, rf"(?m)^#\s+{re.escape(check)}\s+\S", check)

    def test_baseline_is_valid(self) -> None:
        baseline = json.loads(tidy.BASELINE.read_text(encoding="utf-8"))
        self.assertEqual(set(baseline), {"clean_modules", "findings"})
        for file, checks in baseline["findings"].items():
            self.assertTrue(tidy.in_scope(file), file)
            self.assertTrue(all(isinstance(n, int) and n > 0 for n in checks.values()), file)

    def test_ci_pins_version(self) -> None:
        text = (_REPO_ROOT / ".github" / "workflows" / "_reusable-tests.yml").read_text(encoding="utf-8")
        self.assertIn(f"clang-tidy=={tidy.CLANG_TIDY_VERSION}", text)
        self.assertIn("tools/quality/check_tidy.py", text)


class CacheTests(unittest.TestCase):
    """AC-T6.3-3: the cache key changes with every input and nothing else."""

    def test_key_depends_on_inputs(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            source = Path(temp, "a.cpp")
            source.write_text("int main() { return 0; }\n", encoding="utf-8")
            entry = {"directory": temp, "file": str(source), "arguments": ["c++", "-c", str(source), "-o", "a.o"]}
            first = tidy.cache_key("v1", entry, [])
            self.assertEqual(first, tidy.cache_key("v1", entry, []))
            self.assertNotEqual(first, tidy.cache_key("v2", entry, []))
            self.assertNotEqual(first, tidy.cache_key("v1", entry, ["-DX"]))
            source.write_text("int main() { return 1; }\n", encoding="utf-8")
            self.assertNotEqual(first, tidy.cache_key("v1", entry, []))


if __name__ == "__main__":
    unittest.main()
