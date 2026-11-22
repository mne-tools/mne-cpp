# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Tests for scripts/compliance/generate_soup.py.

@since 3.0.0
"""

import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).resolve().parents[2] / "compliance" / "generate_soup.py"
_spec = importlib.util.spec_from_file_location("generate_soup", _SCRIPT)
soup = importlib.util.module_from_spec(_spec)
sys.modules["generate_soup"] = soup
_spec.loader.exec_module(soup)


def entry(**fields):
    base = {"id": "lib", "name": "Lib", "supplier": "S", "license": "MIT", "url": "https://x", "purl": "pkg:generic/lib@{version}",
            "linkage": "dynamic", "shipped": True, "cmake_targets": [], "used_by": "a", "purpose": "p", "risk": "r",
            "mitigation": "m", "safety_relevant": True, "version": {"literal": "1"}}
    base.update(fields)
    return base


class TestGenerateSoup(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.root = Path(self.directory.name)
        (self.root / "src" / "3rdparty").mkdir(parents=True)
        (self.root / "src" / "3rdparty" / "external_deps.env").write_text("# pins\nLIB_VERSION=2.5.0\n", encoding="utf-8")
        (self.root / "lib.h").write_text("#define LIB_MAJOR 3\n#define LIB_MINOR 7\n", encoding="utf-8")

    def tearDown(self):
        self.directory.cleanup()

    def test_versions_come_from_their_pins(self):
        env = soup.read_env(self.root / "src" / "3rdparty" / "external_deps.env")
        self.assertEqual(soup.pinned_version({"env": "LIB_VERSION"}, self.root, env), "2.5.0")
        self.assertEqual(soup.pinned_version({"header": "lib.h", "regex": r"#define LIB_(?:MAJOR|MINOR) (\d+)", "join": "."},
                                             self.root, env), "3.7")

    def test_build_version_reads_the_configured_package(self):
        package = self.root / "pkg"
        package.mkdir()
        (package / "LibConfigVersion.cmake").write_text('set(PACKAGE_VERSION "4.1.2")\n', encoding="utf-8")
        cache = {"Lib_DIR": str(package)}
        self.assertEqual(soup.build_version({"cmake_config_version": "Lib_DIR", "file": "LibConfigVersion.cmake"}, cache), "4.1.2")
        self.assertIsNone(soup.build_version({"cmake_config_version": "Missing_DIR", "file": "x"}, cache))

    def test_an_unlisted_third_party_target_is_reported(self):
        policy = self.root / "policy.json"
        policy.write_text(json.dumps({"allowed_external": ["Lib::Lib", "New::Thing"]}), encoding="utf-8")
        catalog = {"entries": [entry(cmake_targets=["Lib::Lib"])], "toolchain": []}
        self.assertEqual(soup.unclaimed_targets(catalog, policy), ["New::Thing"])

    def test_sbom_lists_only_shipped_components_with_their_purl(self):
        catalog = {"entries": [entry(), entry(id="tool", name="Tool", shipped=False, purl="")], "toolchain": []}
        report = soup.inventory(catalog, self.root)
        sbom = soup.cyclonedx(report, catalog)
        self.assertEqual(sbom["bomFormat"], "CycloneDX")
        self.assertEqual([c["name"] for c in sbom["components"]], ["Lib"])
        self.assertEqual(sbom["components"][0]["purl"], "pkg:generic/lib@1")
        self.assertEqual(sbom["dependencies"][0]["dependsOn"], ["lib"])

    def test_committed_files_are_current(self):
        self.assertEqual(soup.main(["--check"]), 0)


if __name__ == "__main__":
    unittest.main()
