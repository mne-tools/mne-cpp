# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Tests for scripts/release/artifact_evidence.py (v2.4.0 T10.7c)."""

import hashlib
import json
import sys
import tempfile
import unittest
import zipfile
from unittest import mock
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "release"))

import artifact_evidence  # noqa: E402


class ArtifactEvidenceTest(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.root = Path(self._tmp.name)
        self.assets = self.root / "assets"
        self.assets.mkdir()
        for name, data in (("mne-cpp-linux.tar.gz", b"linux"),
                           ("mne-cpp-macos-installer.dmg", b"dmg"),
                           ("mne-cpp-windows-installer.exe", b"exe")):
            (self.assets / name).write_bytes(data)

    def tearDown(self):
        self._tmp.cleanup()

    def _evidence(self, name, signature):
        data = (self.assets / name).read_bytes()
        return {"name": name, "sha256": hashlib.sha256(data).hexdigest(),
                "size": len(data), "signature": signature}

    def test_inspect_records_checksum_and_unsigned_installers(self):
        records = {r["name"]: r for r in artifact_evidence.inspect(
            sorted(self.assets.iterdir()))}
        linux = records["mne-cpp-linux.tar.gz"]
        self.assertEqual(linux["sha256"], hashlib.sha256(b"linux").hexdigest())
        self.assertEqual(linux["signature"], "checksum-only")
        # Junk installers are never reported as signed, on any host
        for name in ("mne-cpp-macos-installer.dmg", "mne-cpp-windows-installer.exe"):
            self.assertIn(records[name]["signature"], ("unsigned", "unverified"))

    def test_inspect_windows_archive_checks_every_mne_executable(self):
        archive = self.assets / "mne-cpp-windows-dynamic.zip"
        with zipfile.ZipFile(archive, "w") as z:
            z.writestr("bin/mne_scan.exe", b"exe")
            z.writestr("bin/mne_browse.exe", b"exe")
            z.writestr("bin/Qt6Core.dll", b"dll")
        checked = []

        def authenticode(path):
            checked.append(path.name)
            return "unsigned" if path.name == "mne_browse.exe" else "signed"

        with mock.patch.object(artifact_evidence, "_authenticode", side_effect=authenticode), \
                mock.patch.object(artifact_evidence.shutil, "which", return_value="powershell"):
            self.assertEqual(artifact_evidence.signature(archive), "unsigned")
            self.assertEqual(sorted(checked), ["mne_browse.exe", "mne_scan.exe"])
            checked.clear()
            with zipfile.ZipFile(archive, "w") as z:
                z.writestr("bin/mne_scan.exe", b"exe")
            self.assertEqual(artifact_evidence.signature(archive), "signed")
        # Without PowerShell the archive is only checksummed, never claimed as signed
        with mock.patch.object(artifact_evidence.shutil, "which", return_value=None):
            self.assertEqual(artifact_evidence.signature(archive), "checksum-only")

    def test_publish_writes_sums_report_and_idempotent_notes(self):
        evidence = self.root / "evidence.json"
        evidence.write_text(json.dumps([
            self._evidence("mne-cpp-macos-installer.dmg", "unsigned"),
            self._evidence("mne-cpp-windows-installer.exe", "signed")]))
        notes = self.root / "notes.md"
        notes.write_text("Release text.\n")
        args = ["publish", "--assets", str(self.assets), "--evidence", str(evidence),
                "--sums", str(self.root / "SHA256SUMS"),
                "--report", str(self.root / "report.json"), "--notes", str(notes)]
        self.assertEqual(artifact_evidence.main(args), 0)
        sums = (self.root / "SHA256SUMS").read_text().splitlines()
        self.assertEqual(len(sums), 3)
        self.assertIn(f"{hashlib.sha256(b'linux').hexdigest()}  mne-cpp-linux.tar.gz",
                      sums)
        report = {r["name"]: r["signature"]
                  for r in json.loads((self.root / "report.json").read_text())}
        self.assertEqual(report, {"mne-cpp-linux.tar.gz": "checksum-only",
                                  "mne-cpp-macos-installer.dmg": "unsigned",
                                  "mne-cpp-windows-installer.exe": "signed"})
        first = notes.read_text()
        self.assertTrue(first.startswith("Release text.\n"))
        self.assertIn("| `mne-cpp-macos-installer.dmg` | **unsigned** |", first)
        self.assertIn("| `mne-cpp-windows-installer.exe` | signed |", first)
        self.assertIn("xattr -dr com.apple.quarantine", first)
        self.assertNotIn("xattr", artifact_evidence.notes_section(
            [{"name": "mne-cpp-macos-installer.dmg", "signature": "signed-notarized"}]))
        self.assertNotIn("SmartScreen", first)
        self.assertIn("Run anyway", artifact_evidence.notes_section(
            [{"name": "mne-cpp-windows-installer.exe", "signature": "unsigned"}]))
        # A rerun replaces the section instead of appending a second one
        self.assertEqual(artifact_evidence.main(args), 0)
        self.assertEqual(notes.read_text(), first)
        # An installer no platform job inspected is never presented as checked
        unrecorded = {r["name"]: r["signature"] for r in artifact_evidence.publish(
            self.assets, [])}
        self.assertEqual(unrecorded["mne-cpp-macos-installer.dmg"], "unverified")
        self.assertEqual(unrecorded["mne-cpp-linux.tar.gz"], "checksum-only")

    def test_workflow_passes_only_inspection_records(self):
        # The signing-evidence artifacts also hold macos-notarization-*.json, which has no asset name
        workflow = (Path(__file__).resolve().parents[3] / ".github" / "workflows"
                    / "_reusable-release-evidence.yml").read_text()
        self.assertIn("--evidence evidence/evidence-*.json ", workflow)

    def test_publish_rejects_replaced_or_missing_installers(self):
        recorded = self._evidence("mne-cpp-windows-installer.exe", "signed")
        (self.assets / "mne-cpp-windows-installer.exe").write_bytes(b"other")
        with self.assertRaisesRegex(ValueError, "differs"):
            artifact_evidence.publish(self.assets, [recorded])
        (self.assets / "mne-cpp-windows-installer.exe").unlink()
        with self.assertRaisesRegex(ValueError, "not published"):
            artifact_evidence.publish(self.assets, [recorded])


if __name__ == "__main__":
    unittest.main()
