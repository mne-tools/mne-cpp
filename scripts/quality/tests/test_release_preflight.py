# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Tests for scripts/release/preflight.py (v2.4.0 T10.3)."""

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "release"))

import preflight  # noqa: E402

FILES = {
    "src/CMakeLists.txt": "set(MNE_CPP_VERSION_MAJOR 2)\nset(MNE_CPP_VERSION_MINOR 4)\nset(MNE_CPP_VERSION_PATCH 0)\n",
    "README.md": '<img src="https://img.shields.io/badge/version-2.4.0-blue.svg">\n',
    "CITATION.cff": 'version: 2.4.0\ndate-released: "2026-10-20"\n',
    "doc/website/docusaurus.config.ts": "const stableVersion = '2.4.0';\n",
    "doc/Doxyfile": 'PROJECT_NUMBER         = "2.4.0"\n',
    "doc/Doxyfile.internal": 'PROJECT_NUMBER         = "2.4.0"\n',
    "CHANGELOG.md": "# Changelog\n\n## [2.4.0] - 2026-10-20\n",
}


class PreflightTest(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.root = Path(self._tmp.name)
        for name, text in FILES.items():
            (self.root / name).parent.mkdir(parents=True, exist_ok=True)
            (self.root / name).write_text(text)
        self.git("init", "-q")
        self.git("add", ".")
        self.git("-c", "user.name=t", "-c", "user.email=t@t", "commit", "-qm", "release")

    def tearDown(self):
        self._tmp.cleanup()

    def git(self, *args):
        return subprocess.run(["git", *args], cwd=self.root, check=True, capture_output=True, text=True).stdout

    def run_preflight(self, *args):
        return preflight.main(["--repo-root", str(self.root), *args])

    def test_consistent_untagged_release_passes(self):
        head = self.git("rev-parse", "HEAD").strip()
        self.assertEqual(self.run_preflight("--commit", head[:9]), 0)

    def test_release_candidate_section_counts_for_its_version(self):
        (self.root / "CHANGELOG.md").write_text("## [2.4.0-rc] - 2026-10-20\n## [2.3.0] - 2026-06-13\n")
        self.assertEqual(preflight.file_problems(self.root, "2.4.0"), [])
        (self.root / "CHANGELOG.md").write_text("## [2.4.0-beta] - 2026-10-20\n")
        self.assertEqual(preflight.file_problems(self.root, "2.4.0"),
                         ["CHANGELOG.md has no dated '## [2.4.0] - YYYY-MM-DD' section"])

    def test_every_invariant_is_enforced(self):
        self.assertEqual(preflight.file_problems(self.root, "2.4.0"), [])
        (self.root / "CHANGELOG.md").write_text("## [2.4.0] - 2026-10-21\n")
        self.assertEqual(preflight.file_problems(self.root, "2.4.0"),
                         ["CITATION.cff date-released 2026-10-20 differs from the CHANGELOG date 2026-10-21"])
        (self.root / "doc/Doxyfile").write_text('PROJECT_NUMBER         = "2.4.0-dev"\n')
        (self.root / "CHANGELOG.md").write_text("## [2.4.0] - Unreleased\n## [2.3.0] - 2026-06-13\n")
        self.assertEqual(preflight.file_problems(self.root, "2.4.0"),
                         ["doc/Doxyfile carries 2.4.0-dev, expected 2.4.0",
                          "CHANGELOG.md has no dated '## [2.4.0] - YYYY-MM-DD' section"])
        self.assertEqual(preflight.git_problems(self.root, "2.4.0", None, None),
                         ["working tree has uncommitted changes"])
        self.git("checkout", "--", ".")
        self.git("tag", "v2.4.0")
        self.assertEqual(preflight.git_problems(self.root, "2.4.0", "0000000", None),
                         [f"HEAD is {self.git('rev-parse', 'HEAD')[:12]}, not the verified commit 0000000",
                          "tag v2.4.0 already exists locally"])
        self.assertEqual(self.run_preflight(), 1)


if __name__ == "__main__":
    unittest.main()
