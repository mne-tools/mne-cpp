#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    3.0.0
# @date     November, 2026
#
#
# @brief    Toolchain downloads must come from the canonical repository so forks can build.
"""Toolchain release assets (Qt, Eigen, ONNX Runtime) exist only on mne-tools/mne-cpp."""

from __future__ import annotations

import re
import unittest
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[3]
_CANONICAL = "mne-tools/mne-cpp"


class TestToolchainSource(unittest.TestCase):
    def test_download_scripts_ignore_the_running_repository(self) -> None:
        for name in ("download_toolchain.sh", "download_toolchain.bat"):
            text = (_REPO_ROOT / "scripts" / "ci" / name).read_text(encoding="utf-8")
            with self.subTest(script=name):
                self.assertNotIn("GITHUB_REPOSITORY", text)
                self.assertRegex(text, r'REPOSITORY="?%s"?' % re.escape(_CANONICAL))

    def test_workflow_qt_downloads_name_the_canonical_repository(self) -> None:
        found = 0
        for workflow in sorted((_REPO_ROOT / ".github" / "workflows").glob("*.yml")):
            for line in workflow.read_text(encoding="utf-8").splitlines():
                if "gh release download qt_binaries" in line:
                    found += 1
                    with self.subTest(workflow=workflow.name, line=line.strip()):
                        self.assertIn(f"-R {_CANONICAL}", line)
        self.assertGreater(found, 0)


if __name__ == "__main__":
    unittest.main()
