#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# @since    2.4.0
# @date     October, 2026
#
#
# @brief    Tests for the parity reference lock (T7.1) and fixture manifest (T7.4).
"""Tests for ``scripts/parity/reference.py`` and ``scripts/parity/check_fixtures.py``."""

from __future__ import annotations

import importlib.util
import json
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from importlib import metadata
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[3]
_PARITY = _REPO_ROOT / "scripts" / "parity"


def _load(name: str):
    spec = importlib.util.spec_from_file_location(name, _PARITY / f"{name}.py")
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


reference = _load("reference")
check_fixtures = _load("check_fixtures")

# Generators that do not produce MNE-Python reference fixtures, with the reason
NOT_MNE_PYTHON = {
    "src/testframes/test_inv_cmne/data/make_cmne_reference.py": "reference output of the cmne package, not MNE-Python",
}


class ReferenceLockTests(unittest.TestCase):
    LOCK = {"python": "3.10", "packages": {"mne": "1.11.0", "numpy": "2.2.6"}}

    def check(self, versions: dict[str, str], python: str = "3.10") -> tuple[list[str], list[str]]:
        def installed(name: str) -> str:
            if name not in versions:
                raise metadata.PackageNotFoundError(name)
            return versions[name]
        return reference.check_reference(self.LOCK, installed, python)

    def test_minor_mismatch_is_refused_and_patch_is_noted(self) -> None:
        """AC-T7.1-1: a generator refuses a wrong major/minor version."""
        self.assertEqual(self.check({"mne": "1.11.0", "numpy": "2.2.6"}), ([], []))
        errors, notes = self.check({"mne": "1.11.1", "numpy": "2.4.0"}, python="3.13")
        self.assertEqual(errors, ["numpy 2.4.0 (reference 2.2.6)"])
        self.assertEqual(notes, ["Python 3.13 (reference 3.10)", "mne 1.11.1 (reference 1.11.0)"])
        self.assertEqual(self.check({"numpy": "2.2.6"})[0], ["mne is not installed (reference 1.11.0)"])

    def test_ci_installs_exactly_the_lock(self) -> None:
        """AC-T7.1-2: CI and fixture generation use one environment."""
        lock = reference.load_lock()
        workflow = (_REPO_ROOT / ".github" / "workflows" / "_reusable-tests.yml").read_text(encoding="utf-8")
        pins = dict(re.findall(r"\b([A-Za-z][\w.-]*)==([\w.]+)", workflow))
        for package, version in lock["packages"].items():
            self.assertEqual(pins.get(package), version, package)
        self.assertIn(f"python-version: '{lock['python']}'", workflow)

    def test_every_mne_python_fixture_generator_checks_the_reference(self) -> None:
        files = subprocess.run(["git", "ls-files", "src/testframes"], cwd=_REPO_ROOT, check=True,
                               capture_output=True, text=True).stdout.split()
        generators = [f for f in files if Path(f).name.startswith("make_") and f.endswith(".py")]
        self.assertTrue(generators)
        for path in generators:
            calls = "require_reference()" in (_REPO_ROOT / path).read_text(encoding="utf-8")
            self.assertEqual(calls, path not in NOT_MNE_PYTHON, path)


class FixtureManifestTests(unittest.TestCase):
    def setUp(self) -> None:
        self.root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.root)
        data = self.root / "src" / "testframes" / "test_demo" / "data"
        data.mkdir(parents=True)
        (data / "a.fif").write_bytes(b"\x00\x01")
        (data.parent / "make_a.py").write_text("require_reference()\n", encoding="utf-8")
        self.files = ["src/testframes/test_demo/data/a.fif", "src/testframes/test_demo/make_a.py"]
        self.manifest = check_fixtures.build_manifest(self.root, self.files)

    def problems(self, files: list[str] | None = None) -> list[str]:
        return check_fixtures.compare(self.manifest, check_fixtures.build_manifest(self.root, files or self.files))

    def test_manifest_records_generator_and_checksum(self) -> None:
        """AC-T7.4-1."""
        entry = self.manifest["tests"]["test_demo"]
        self.assertEqual(entry["generators"], ["src/testframes/test_demo/make_a.py"])
        self.assertEqual(len(entry["fixtures"]["src/testframes/test_demo/data/a.fif"]), 64)
        self.assertEqual(self.problems(), [])

    def test_corrupted_orphaned_and_missing_fixtures_fail(self) -> None:
        """AC-T7.4-3: a corrupted or stale fixture fails and names its generator."""
        (self.root / self.files[0]).write_bytes(b"\x00\x02")
        self.assertIn("regenerate with src/testframes/test_demo/make_a.py", self.problems()[0])
        (self.root / self.files[0]).write_bytes(b"\x00\x01")
        (self.root / "src/testframes/test_demo/data/b.fif").write_bytes(b"x")
        self.assertIn("not in the manifest", self.problems(self.files + ["src/testframes/test_demo/data/b.fif"])[0])
        self.assertIn("not committed", self.problems([self.files[1]])[0])

    def test_committed_manifest_is_current(self) -> None:
        manifest = json.loads(check_fixtures.MANIFEST.read_text(encoding="utf-8"))
        self.assertEqual(check_fixtures.compare(manifest, check_fixtures.build_manifest(_REPO_ROOT)), [])


if __name__ == "__main__":
    unittest.main()
