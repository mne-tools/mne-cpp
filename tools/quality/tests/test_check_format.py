#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     October, 2026
#
#
# @brief    Fixtures for the clang-format gate (T6.2).
"""Fixtures for ``check_format.py``, ``.clang-format`` and the CI wiring of the format gate."""

from __future__ import annotations

import importlib.util
import re
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

_QUALITY_DIR = Path(__file__).resolve().parent.parent
_REPO_ROOT = _QUALITY_DIR.parents[1]
_WORKFLOWS = _REPO_ROOT / ".github" / "workflows"


def _load(name: str):
    spec = importlib.util.spec_from_file_location(name, _QUALITY_DIR / f"{name}.py")
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


fmt = _load("check_format")


def _pinned_clang_format() -> str | None:
    try:
        return fmt.find_clang_format()
    except (FileNotFoundError, RuntimeError):
        return None


class ScopeTests(unittest.TestCase):
    def test_in_scope(self) -> None:
        self.assertTrue(fmt.in_scope("src/libraries/fiff/fiff_raw_data.cpp"))
        self.assertTrue(fmt.in_scope("src/libraries/fiff/fiff_raw_data.h"))
        self.assertFalse(fmt.in_scope("src/external/eigen/Eigen/Core.h"))
        self.assertFalse(fmt.in_scope("tools/quality/check_format.py"))
        self.assertFalse(fmt.in_scope("src/libraries/fiff/CMakeLists.txt"))
        self.assertFalse(fmt.in_scope("doc/snippet.cpp"))

    def test_all_files_are_tracked_sources(self) -> None:
        files = fmt.all_files()
        self.assertGreater(len(files), 1000)
        self.assertTrue(all(fmt.in_scope(name) for name in files))


class ConfigTests(unittest.TestCase):
    def test_style_keeps_tokens_and_order(self) -> None:
        style = (_REPO_ROOT / ".clang-format").read_text(encoding="utf-8")
        for option in ("SortIncludes: Never", "SortUsingDeclarations: Never", "ReflowComments: false",
                       "FixNamespaceComments: false", "ColumnLimit: 0"):
            self.assertRegex(style, rf"(?m)^{re.escape(option)}\b", option)

    def test_ci_pins_version_and_checks_all_files(self) -> None:
        for name in ("pull-request.yml", "staging.yml", "main.yml"):
            text = (_WORKFLOWS / name).read_text(encoding="utf-8")
            self.assertIn(f"clang-format=={fmt.CLANG_FORMAT_VERSION}", text, name)
            self.assertIn("tools/quality/check_format.py --all", text, name)

    def test_blame_ignores_reformat(self) -> None:
        revs = (_REPO_ROOT / ".git-blame-ignore-revs").read_text(encoding="utf-8")
        self.assertRegex(revs, r"(?m)^[0-9a-f]{40}$")


@unittest.skipIf(_pinned_clang_format() is None, f"clang-format {fmt.CLANG_FORMAT_VERSION} not on PATH")
class GateTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = Path(tempfile.mkdtemp())
        shutil.copy(_REPO_ROOT / ".clang-format", self.tmp / ".clang-format")
        self.binary = _pinned_clang_format()

    def tearDown(self) -> None:
        shutil.rmtree(self.tmp)

    def test_detects_and_fixes(self) -> None:
        good = self.tmp / "good.cpp"
        bad = self.tmp / "bad.cpp"
        good.write_text("int f(int& a)\n{\n    return a;\n}\n", encoding="utf-8")
        bad.write_text("int g(int &a) { return a; }\n", encoding="utf-8")
        files = [str(good), str(bad)]

        unformatted, report = fmt.run_clang_format(self.binary, files, fix=False)
        self.assertEqual(unformatted, [str(bad)])
        self.assertTrue(report)

        fmt.run_clang_format(self.binary, files, fix=True)
        self.assertEqual(fmt.run_clang_format(self.binary, files, fix=False), ([], []))
        self.assertEqual(bad.read_text(encoding="utf-8"), "int g(int& a)\n{\n    return a;\n}\n")


if __name__ == "__main__":
    unittest.main()
