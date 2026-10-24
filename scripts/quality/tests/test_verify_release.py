# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Tests for scripts/release/verify_release.py (v2.4.0 T10.4)."""

import hashlib
import io
import json
import struct
import sys
import tarfile
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "release"))

import verify_release  # noqa: E402

PREFIX = "mne-cpp-2.4.0"


class VerifyReleaseTest(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.assets = Path(self._tmp.name)
        evidence = []
        for platform, suffixes in verify_release.REQUIRED.items():
            for suffix in suffixes:
                name = f"{PREFIX}-{suffix}"
                (self.assets / name).write_bytes(name.encode())
                status = verify_release.SIGNED.get(Path(name).suffix, "checksum-only")
                evidence.append({"name": name, "signature": status})
        (self.assets / "release-evidence.json").write_text(json.dumps(evidence))
        self.write_sums()

    def tearDown(self):
        self._tmp.cleanup()

    def write_sums(self):
        lines = [f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}" for p in sorted(self.assets.iterdir())
                 if p.name != "SHA256SUMS"]
        (self.assets / "SHA256SUMS").write_text("\n".join(lines) + "\n")

    def check(self, signed=True):
        return verify_release.check_assets(self.assets, PREFIX, sorted(verify_release.REQUIRED), signed)

    def test_complete_signed_release_passes(self):
        self.assertEqual(self.check(), [])

    def test_missing_corrupt_unlisted_and_unsigned_assets_fail(self):
        (self.assets / f"{PREFIX}-linux-installer-x86_64.run").unlink()
        (self.assets / f"{PREFIX}-linux-static-x86_64.tar.gz").write_bytes(b"corrupt")
        (self.assets / "stray.txt").write_text("x")
        evidence = json.loads((self.assets / "release-evidence.json").read_text())
        for record in evidence:
            if record["name"].endswith(".dmg"):
                record["signature"] = "unsigned"
        (self.assets / "release-evidence.json").write_text(json.dumps(evidence))
        problems = self.check()
        self.assertIn(f"{PREFIX}-linux-installer-x86_64.run is missing", problems)
        self.assertIn(f"{PREFIX}-linux-static-x86_64.tar.gz does not match SHA256SUMS", problems)
        self.assertIn("stray.txt is not listed in SHA256SUMS", problems)
        self.assertIn(f"{PREFIX}-macos-installer-arm64.dmg is unsigned, expected signed-notarized", problems)
        self.assertNotIn(f"{PREFIX}-macos-installer-arm64.dmg is unsigned, expected signed-notarized",
                         self.check(signed=False))

    def test_unsigned_macos_release_requires_dmgs_recorded_as_unsigned(self):
        def check():
            return verify_release.check_assets(self.assets, PREFIX, sorted(verify_release.REQUIRED), True,
                                               unsigned_macos=True)

        dmg = f"{PREFIX}-macos-installer-arm64.dmg"
        self.assertIn(f"{dmg} is signed-notarized, expected unsigned", check())
        evidence = json.loads((self.assets / "release-evidence.json").read_text())
        for record in evidence:
            if record["name"].endswith(".dmg"):
                record["signature"] = "unsigned"
        (self.assets / "release-evidence.json").write_text(json.dumps(evidence))
        self.write_sums()
        self.assertEqual(check(), [])
        evidence[-1]["signature"] = "unsigned"
        for record in evidence:
            if record["name"].endswith(".exe"):
                record["signature"] = "unsigned"
        (self.assets / "release-evidence.json").write_text(json.dumps(evidence))
        self.write_sums()
        self.assertIn(f"{PREFIX}-windows-installer-x86_64.exe is unsigned, expected signed", check())

    def test_smoke_reports_an_archive_without_the_tool(self):
        archive = self.assets / "empty.tar.gz"
        with tarfile.open(archive, "w:gz") as t:
            info = tarfile.TarInfo("mne-cpp/bin/readme.txt")
            info.size = 2
            t.addfile(info, io.BytesIO(b"hi"))
        work = self.assets / "work"
        work.mkdir()
        self.assertEqual(verify_release.smoke(archive, work), [f"empty.tar.gz contains no {verify_release.SMOKE_TOOL}"])

    def test_smoke_reports_links_to_the_build_machine(self):
        archive = self.assets / "linked.tar.gz"
        with tarfile.open(archive, "w:gz") as t:
            for name, target in (("mne-cpp/resources", "/home/runner/work/mne-cpp/mne-cpp/resources"),
                                 ("mne-cpp/lib/QtCore.framework/Versions/Current", "A")):
                info = tarfile.TarInfo(name)
                info.type, info.linkname = tarfile.SYMTYPE, target
                t.addfile(info)
        work = self.assets / "work"
        work.mkdir()
        self.assertEqual(verify_release.smoke(archive, work), [
            "linked.tar.gz: mne-cpp/resources links outside the archive (/home/runner/work/mne-cpp/mne-cpp/resources)"])

    def test_macos_binaries_must_run_on_the_documented_minimum(self):
        def macho(minos, fat=False):
            # 64-bit Mach-O header + one LC_BUILD_VERSION (platform macOS, minos, sdk 26.5)
            version = lambda v: (int(v.split(".")[0]) << 16) | (int(v.split(".")[1]) << 8)  # noqa: E731
            thin = struct.pack("<7I4x", 0xFEEDFACF, 0x0100000C, 0, 2, 1, 24, 0) + struct.pack(
                "<6I", 0x32, 24, 1, version(minos), version("26.5"), 0)
            if not fat:
                return thin
            return struct.pack(">2I", 0xCAFEBABE, 1) + struct.pack(">5I", 0x0100000C, 0, 28, len(thin), 0) + thin

        work = self.assets / "work"
        (work / "lib").mkdir(parents=True)
        (work / "lib" / "libmne_fiff.dylib").write_bytes(macho("26.0"))
        (work / "lib" / "QtCore").write_bytes(macho("14.4", fat=True))
        (work / "lib" / "readme.txt").write_text("not a binary")
        self.assertEqual(verify_release.macos_minimum_problems(work, "14.4"),
                         ["lib/libmne_fiff.dylib needs macOS 26.0, documented minimum is 14.4"])
        (work / "lib" / "libmne_fiff.dylib").write_bytes(macho("14.0"))
        self.assertEqual(verify_release.macos_minimum_problems(work, "14.4"), [])


if __name__ == "__main__":
    unittest.main()
