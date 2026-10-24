#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# @since    2.4.0
# @date     October, 2026
#
#
# @brief    The hand-written library overviews only name API that exists.
"""Every identifier in the first column of a ``doc/api_overviews/*.mdx`` table must occur in a header.

These overviews become the API landing pages; they are hand-written and had listed classes
(``MlScaler``, ``MlPipeline``, ``RtFilter``, ...) that were never part of the library.
"""

from __future__ import annotations

import re
import subprocess
import unittest
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[3]
_FIRST_COLUMN = re.compile(r"^\|\s*`([A-Za-z_][A-Za-z0-9_:]*)`", re.MULTILINE)


def missing_names(repo_root: Path) -> list[str]:
    files = subprocess.run(["git", "ls-files", "src/libraries", "src/applications"], cwd=repo_root, check=True,
                           capture_output=True, text=True).stdout.split()
    headers = "\n".join((repo_root / f).read_text(encoding="utf-8", errors="replace")
                        for f in files if f.endswith((".h", ".hpp")))
    words = set(re.findall(r"\b[A-Za-z_][A-Za-z0-9_]*\b", headers))
    missing = []
    for overview in sorted((repo_root / "doc" / "api_overviews").glob("*.mdx")):
        for name in sorted(set(_FIRST_COLUMN.findall(overview.read_text(encoding="utf-8")))):
            if name.split("::")[-1] not in words:
                missing.append(f"{overview.name}: {name}")
    return missing


class ApiOverviewTests(unittest.TestCase):
    def test_overviews_name_existing_api(self) -> None:
        self.assertEqual(missing_names(_REPO_ROOT), [])


if __name__ == "__main__":
    unittest.main()
