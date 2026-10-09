# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Tests for tools/parity/gap_analysis.py."""

import contextlib
import io
import sys
import types
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "parity"))

import gap_analysis  # noqa: E402


class _NeedsSklearn(types.ModuleType):
    def __getattr__(self, name):
        if name == "Xdawn":
            raise ImportError("No module named 'sklearn'")
        raise AttributeError(name)


def _fake_mne(with_sklearn):
    mne = types.ModuleType("mne")
    mne.__version__ = "1.11.0"
    mne.__all__ = ["read_evokeds", "preprocessing"]
    mne.read_evokeds = lambda: None
    preprocessing = types.ModuleType("mne.preprocessing") if with_sklearn \
        else _NeedsSklearn("mne.preprocessing")
    preprocessing.__all__ = ["Xdawn"]
    if with_sklearn:
        preprocessing.Xdawn = type("Xdawn", (), {})
    mne.preprocessing = preprocessing
    return mne


class InventoryTest(unittest.TestCase):
    def test_inventory_lists_public_api(self):
        with mock.patch.dict(sys.modules, {"mne": _fake_mne(True)}):
            names = [r["python"] for r in gap_analysis.build_python_inventory("1.11")]
        self.assertEqual(names, ["mne.read_evokeds", "mne.preprocessing.Xdawn"])

    def test_missing_optional_dependency_fails_instead_of_shrinking(self):
        with mock.patch.dict(sys.modules, {"mne": _fake_mne(False)}), \
                contextlib.redirect_stderr(io.StringIO()) as stderr, \
                self.assertRaises(SystemExit):
            gap_analysis.build_python_inventory("1.11")
        self.assertIn("mne.preprocessing.Xdawn", stderr.getvalue())


if __name__ == "__main__":
    unittest.main()
