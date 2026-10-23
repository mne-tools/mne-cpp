# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Tests for tools/release/pe_metadata.py and the SignPath metadata restrictions."""

import re
import struct
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
CONFIGURATIONS = REPO_ROOT / ".signpath" / "artifact-configurations"
sys.path.insert(0, str(REPO_ROOT / "tools" / "release"))

import pe_metadata  # noqa: E402

GOOD = {"ProductName": "MNE-CPP", "ProductVersion": "2.4.0"}


def _res_entry(type_id: int, name_id: int, language: int, payload: bytes) -> bytes:
    header = struct.pack("<II", len(payload), 32) + struct.pack("<HHHH", 0xFFFF, type_id, 0xFFFF, name_id)
    header += struct.pack("<IHHII", 0, 0x30, language, 0, 0)
    return header + payload + b"\0" * (-len(payload) % 4)


class ArtifactConfigurationTest(unittest.TestCase):
    def test_every_signed_file_is_restricted_to_mne_cpp_and_the_version_parameter(self):
        slugs = set(re.findall(r"artifact-configuration-slug: ([\w-]+)",
                               "\n".join(p.read_text(encoding="utf-8")
                                         for p in (REPO_ROOT / ".github" / "workflows").glob("*.yml"))))
        self.assertEqual({p.stem for p in CONFIGURATIONS.glob("*.xml")}, slugs)
        for path in CONFIGURATIONS.glob("*.xml"):
            _kind, rules = pe_metadata.load_configuration(path, {"version": "9.8.7"})
            self.assertTrue(rules, path.name)
            for rule in rules:
                self.assertEqual((rule.product_name, rule.product_version), ("MNE-CPP", "9.8.7"), path.name)

    def test_missing_restriction_or_parameter_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            bare = Path(tmp) / "bare.xml"
            bare.write_text('<artifact-configuration xmlns="http://signpath.io/artifact-configuration/v1">'
                            '<pe-file><authenticode-sign/></pe-file></artifact-configuration>')
            with self.assertRaisesRegex(ValueError, "no product-name/product-version"):
                pe_metadata.load_configuration(bare, {})
        with self.assertRaisesRegex(ValueError, "required parameter 'version'"):
            pe_metadata.load_configuration(CONFIGURATIONS / "windows-installer.xml", {})

    def test_include_overrides_the_file_set(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "c.xml"
            path.write_text('<artifact-configuration xmlns="http://signpath.io/artifact-configuration/v1">'
                            '<zip-file><pe-file-set product-name="A" product-version="1">'
                            '<include path="a.exe"/><include path="b.dll" product-name="B"/>'
                            '<for-each><authenticode-sign/></for-each></pe-file-set></zip-file>'
                            '</artifact-configuration>')
            kind, rules = pe_metadata.load_configuration(path, {})
        self.assertEqual(kind, "zip-file")
        self.assertEqual([(r.path, r.product_name) for r in rules], [("a.exe", "A"), ("b.dll", "B")])


class GlobTest(unittest.TestCase):
    def test_signpath_wildcards(self):
        cases = {
            "**/mne_*.exe": (["mne_scan.exe", "mne-cpp/bin/mne_scan.exe", "a/b/MNE_X.EXE"],
                             ["mne-cpp/bin/Qt6Core.dll", "mne-cpp/bin/mne_scan.exe.bak"]),
            "**/mne_*_plugins/*.dll": (["bin/mne_scan_plugins/scan_fiffsimulator.dll"],
                                       ["bin/mne_scan_plugins/sub/x.dll", "bin/plugins/x.dll"]),
            "th?s[!0-9].exe": (["thisa.exe"], ["thsa.exe", "this1.exe"]),
        }
        for pattern, (hits, misses) in cases.items():
            regex = pe_metadata.glob_regex(pattern)
            for name in hits:
                self.assertTrue(regex.match(name), (pattern, name))
            for name in misses:
                self.assertFalse(regex.match(name), (pattern, name))


class CheckTest(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.root = Path(self._tmp.name)
        self.archive = self.root / "mne-cpp-windows-dynamic-x86_64.zip"
        with zipfile.ZipFile(self.archive, "w") as z:
            for name in ("mne-cpp/bin/mne_scan.exe", "mne-cpp/bin/mne_fiff.dll", "mne-cpp/bin/Qt6Core.dll",
                         "mne-cpp/bin/mne_scan_plugins/scan_babymeg.dll"):
                z.writestr(name, name)

    def tearDown(self):
        self._tmp.cleanup()

    def _check(self, metadata):
        def reader(path: Path) -> dict:
            return metadata.get(path.read_text(), GOOD)
        return pe_metadata.check(self.archive, CONFIGURATIONS / "windows-archive.xml", "2.4.0", reader)

    def test_consistent_first_party_binaries_pass_and_qt_is_not_checked(self):
        self.assertEqual(self._check({"mne-cpp/bin/Qt6Core.dll": {"ProductName": "Qt6"}}), [])

    def test_wrong_product_name_is_rejected_with_the_file_named(self):
        errors = self._check({"mne-cpp/bin/mne_fiff.dll": {"ProductName": "Qt", "ProductVersion": "2.4.0"}})
        self.assertEqual(errors, ["mne-cpp/bin/mne_fiff.dll: ProductName is 'Qt', expected 'MNE-CPP'"])

    def test_mismatched_or_missing_version_is_rejected(self):
        errors = self._check({"mne-cpp/bin/mne_scan_plugins/scan_babymeg.dll": {"ProductName": "MNE-CPP",
                                                                                "ProductVersion": "2.3.0"},
                              "mne-cpp/bin/mne_scan.exe": {}})
        self.assertEqual(sorted(errors), [
            "mne-cpp/bin/mne_scan.exe: ProductName is missing, expected 'MNE-CPP'",
            "mne-cpp/bin/mne_scan.exe: ProductVersion is missing, expected '2.4.0'",
            "mne-cpp/bin/mne_scan_plugins/scan_babymeg.dll: ProductVersion is '2.3.0', expected '2.4.0'",
        ])

    def test_archive_without_an_executable_fails(self):
        with zipfile.ZipFile(self.archive, "w") as z:
            z.writestr("mne-cpp/bin/Qt6Core.dll", "qt")
        self.assertEqual(self._check({}), ["**/mne_*.exe: matches 0 file(s), needs at least 1"])

    def test_installer_is_checked_itself(self):
        installer = self.root / "MNE-CPP-2.4.0-win64.exe"
        installer.write_bytes(b"MZ")
        config = CONFIGURATIONS / "windows-installer.xml"
        self.assertEqual(pe_metadata.check(installer, config, "2.4.0", lambda _p: GOOD), [])
        self.assertEqual(pe_metadata.check(installer, config, "2.4.0",
                                           lambda _p: {"ProductName": "Qt Installer Framework"}),
                         ["MNE-CPP-2.4.0-win64.exe: ProductName is 'Qt Installer Framework', expected 'MNE-CPP'",
                          "MNE-CPP-2.4.0-win64.exe: ProductVersion is missing, expected '2.4.0'"])


class VersionResourceTest(unittest.TestCase):
    def test_project_version_matches_cmake(self):
        text = (REPO_ROOT / "src" / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertRegex(pe_metadata.project_version(), r"^\d+\.\d+\.\d+$")
        self.assertIn(f"set(MNE_CPP_VERSION_MAJOR {pe_metadata.project_version().split('.')[0]})", text)

    def test_rc_template_names_mne_cpp_with_one_version_everywhere(self):
        rc = pe_metadata.configure_rc("2.4.0", "mne_scan", "MNE-CPP mne_scan", "0x1L")
        self.assertIn('VALUE "ProductName",      "MNE-CPP"', rc)
        self.assertIn('VALUE "ProductVersion",   "2.4.0"', rc)
        self.assertIn('VALUE "FileVersion",      "2.4.0"', rc)
        self.assertEqual(re.findall(r"(?:FILE|PRODUCT)VERSION\s+(\S+)", rc), ["2,4,0,0", "2,4,0,0"])

    def test_cmake_attaches_the_resource_to_every_windows_binary(self):
        module = (REPO_ROOT / "cmake" / "MneVersionInfo.cmake").read_text(encoding="utf-8")
        for type_ in ("EXECUTABLE", "SHARED_LIBRARY", "MODULE_LIBRARY"):
            self.assertIn(type_, module)
        top = (REPO_ROOT / "src" / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertRegex(top, r"mne_add_windows_version_info\(\$\{CMAKE_CURRENT_SOURCE_DIR\} \$\{MNE_CPP_VERSION\}\)")
        self.assertGreater(top.index("mne_add_windows_version_info("), top.rindex("add_subdirectory("))

    def test_res_parser_returns_the_version_resource(self):
        data = _res_entry(0, 0, 0, b"") + _res_entry(14, 1, 0x0409, b"icon") + _res_entry(16, 1, 0x0409, b"version!!")
        resources = pe_metadata.res_resources(data)
        self.assertEqual([(t, n, lang) for t, n, lang, _p in resources], [(14, 1, 0x0409), (16, 1, 0x0409)])
        self.assertEqual(resources[1][3], b"version!!")


class WorkflowTest(unittest.TestCase):
    def test_signing_checks_metadata_and_passes_the_version(self):
        action = (REPO_ROOT / ".github" / "actions" / "signpath-sign" / "action.yml").read_text(encoding="utf-8")
        self.assertLess(action.index("pe_metadata.py check"), action.index("signpath/github-action-submit-signing"))
        self.assertIn('version: "${{ steps.metadata.outputs.version }}"', action)

    def test_installer_template_is_stamped_and_checked(self):
        text = (REPO_ROOT / ".github" / "workflows" / "release-installer.yml").read_text(encoding="utf-8")
        windows = text[text.index("WindowsInstaller:"):]
        self.assertLess(windows.index("pe_metadata.py stamp"), windows.index("binarycreator `"))
        self.assertIn("-t build_installer/installerbase.exe", windows)
        self.assertLess(windows.index("pe_metadata.py check"), windows.index("Authenticode-sign the installer"))


if __name__ == "__main__":
    unittest.main()
