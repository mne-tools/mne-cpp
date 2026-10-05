#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Tests for ``tools/parity/extract_mne_python_api.py`` on a stand-in ``mne`` module."""

from __future__ import annotations

import importlib.util
import sys
import types
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).resolve().parents[2] / "parity" / "extract_mne_python_api.py"
sys.path.insert(0, str(_SCRIPT.parent))
_spec = importlib.util.spec_from_file_location("extract_mne_python_api", _SCRIPT)
assert _spec is not None and _spec.loader is not None
extract = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(extract)


def _fake_mne() -> types.ModuleType:
    mne = types.ModuleType("mne")
    mne.__version__ = "1.11.0"

    def read_raw(fname, preload=False, data_fun=lambda x: x):
        return fname

    def old_reader(fname):
        return fname

    old_reader._deprecated_original = old_reader  # what mne.utils.deprecated leaves behind

    class Epochs:
        def __init__(self, raw, tmin=-0.2):
            self.raw = raw

    def set_log_level(level):
        return level

    mne.read_raw, mne.old_reader, mne.Epochs, mne.set_log_level = read_raw, old_reader, Epochs, set_log_level
    mne.__all__ = ["read_raw", "old_reader", "Epochs", "set_log_level", "_private"]
    mne.io = types.ModuleType("mne.io")
    mne.io.read_raw = read_raw  # re-export: listed once under the first namespace
    mne.io.__all__ = ["read_raw"]
    return mne


class ExtractTests(unittest.TestCase):
    def test_records(self) -> None:
        apis = {a["python"]: a for a in extract.extract(_fake_mne())}
        self.assertEqual(sorted(apis), ["mne.Epochs", "mne.io.read_raw", "mne.old_reader", "mne.read_raw", "mne.set_log_level"])
        self.assertEqual(apis["mne.Epochs"]["kind"], "class")
        self.assertEqual(apis["mne.Epochs"]["signature"], "(raw, tmin=-0.2)")
        self.assertTrue(apis["mne.old_reader"]["deprecated"])
        self.assertFalse(apis["mne.read_raw"]["deprecated"])

    def test_exclusions_stay_visible(self) -> None:
        apis = {a["python"]: a for a in extract.extract(_fake_mne())}
        self.assertTrue(apis["mne.set_log_level"]["excluded"])
        self.assertTrue(apis["mne.set_log_level"]["exclusion_reason"])
        self.assertFalse(apis["mne.read_raw"]["excluded"])

    def test_snapshot_is_stable(self) -> None:
        first = extract.dump(extract.build_snapshot(_fake_mne(), "1.11"))
        second = extract.dump(extract.build_snapshot(_fake_mne(), "1.11"))
        self.assertEqual(first, second)
        self.assertNotIn(" at 0x", first)


if __name__ == "__main__":
    unittest.main()
