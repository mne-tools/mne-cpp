#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.5.0
# @date     November, 2026
#
#
# @brief    Fixtures for the duplicate mne:: symbol gate.
"""Fixtures for ``check_duplicate_symbols.py`` and its CI wiring."""

from __future__ import annotations

import importlib.util
import sys
import unittest
from pathlib import Path

_QUALITY_DIR = Path(__file__).resolve().parent.parent
_WORKFLOW = _QUALITY_DIR.parents[1] / ".github" / "workflows" / "_reusable-tests.yml"


def _load(name: str):
    spec = importlib.util.spec_from_file_location(name, _QUALITY_DIR / f"{name}.py")
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


dup = _load("check_duplicate_symbols")


class ParsingTests(unittest.TestCase):
    def test_darwin_keeps_strong_mne_definitions_only(self):
        nm = "\n".join([
            "0000000000001000 (__TEXT,__text) external __ZN3mne13buildDateTimeEv",
            "0000000000002000 (__TEXT,__text) weak external __ZN3mne6Linalg4sortIiEEvv",
            "0000000000003000 (__TEXT,__text) external __ZN5Eigen3fooEv",
            "0000000000004000 (__DATA,__data) external __ZNK3mne8FiffInfo4sizeEv",
        ])
        self.assertEqual(dup.strong_symbols_darwin(nm), {"__ZN3mne13buildDateTimeEv", "__ZNK3mne8FiffInfo4sizeEv"})

    def test_elf_keeps_strong_mne_definitions_only(self):
        nm = "\n".join([
            "0000000000001000 T _ZN3mne13buildDateTimeEv",
            "0000000000002000 W _ZN3mne6Linalg4sortIiEEvv",
            "0000000000003000 V _ZN3mne1xE",
            "0000000000004000 B _ZN3mne5countE",
            "0000000000005000 T _ZN5Eigen3fooEv",
        ])
        self.assertEqual(dup.strong_symbols_elf(nm), {"_ZN3mne13buildDateTimeEv", "_ZN3mne5countE"})

    def test_duplicates_lists_every_owner(self):
        found = dup.duplicates({"libmne_fiff.so": {"a", "b"}, "libmne_inv.so": {"b"}, "libmne_mne.so": {"b", "c"}})
        self.assertEqual(found, {"b": ["libmne_fiff.so", "libmne_inv.so", "libmne_mne.so"]})

    def test_missing_libraries_is_an_error(self):
        self.assertEqual(dup.main([str(_QUALITY_DIR)]), 2)


class WorkflowTests(unittest.TestCase):
    def test_ci_runs_the_check_after_the_build(self):
        text = _WORKFLOW.read_text(encoding="utf-8")
        self.assertIn("scripts/quality/check_duplicate_symbols.py", text)


if __name__ == "__main__":
    unittest.main()
