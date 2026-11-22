# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Q_INIT_RESOURCE names a resource file that exists.

Q_INIT_RESOURCE(name) calls qInitResources_name(), which rcc generates from
name.qrc. Dynamic builds load plugin resources by themselves, so the call often
sits in a STATICBUILD branch, and a removed .qrc then breaks only the static
release builds, at link time.

@since 3.0.0
"""

import re
import subprocess
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]

_INIT = re.compile(r"^\s*Q_INIT_RESOURCE\(\s*(\w+)\s*\)", re.MULTILINE)


def missing_resources(root: Path = REPO_ROOT) -> list[str]:
    """Lists '<file>: <name>' for every Q_INIT_RESOURCE without a <name>.qrc in the repository."""
    files = subprocess.run(["git", "ls-files"], cwd=root, capture_output=True, text=True, check=True).stdout.split()
    resources = {Path(f).stem for f in files if f.endswith(".qrc")}
    missing = []
    for name in files:
        if not name.startswith("src/") or "/external/" in name or not name.endswith((".cpp", ".h")):
            continue
        text = (root / name).read_text(encoding="utf-8", errors="replace")
        missing += [f"{name}: {r}" for r in _INIT.findall(text) if r not in resources]
    return missing


class TestQtResources(unittest.TestCase):
    def test_every_initialised_resource_exists(self):
        self.assertEqual(missing_resources(), [])


if __name__ == "__main__":
    unittest.main()
