# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""SVG goldens of the tests are checked out byte for byte on every platform.

The scene tests compare rendered SVG with the committed golden byte for byte. A Windows checkout with
core.autocrlf converts the golden to CR LF and the comparison fails, so every SVG golden has the text
attribute unset in .gitattributes.

@since 3.0.0
"""

import subprocess
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]


def converted_goldens() -> list[str]:
    """Tracked SVG goldens under src/tests/*/data whose line endings git would convert."""
    names = subprocess.run(["git", "-C", str(REPO_ROOT), "ls-files", "src/tests/*/data/*.svg"], capture_output=True,
                           text=True, check=True).stdout.split()
    if not names:
        return []
    attrs = subprocess.run(["git", "-C", str(REPO_ROOT), "check-attr", "text", "--", *names], capture_output=True,
                           text=True, check=True).stdout.splitlines()
    return sorted(line.split(":")[0] for line in attrs if not line.endswith(": unset"))


class TestGoldenFiles(unittest.TestCase):
    def test_svg_goldens_are_not_converted(self):
        self.assertEqual(converted_goldens(), [])


if __name__ == "__main__":
    unittest.main()
