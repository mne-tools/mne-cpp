# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Headers with over-aligned members silence MSVC's C4324 padding warning, which /WX turns into an error.

@since 3.0.0
"""

import re
import unittest
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[3]


class TestOverAlignedHeaders(unittest.TestCase):
    def test_alignas_headers_disable_c4324(self) -> None:
        missing = []
        for header in sorted((_REPO_ROOT / "src" / "libs").rglob("*.h")):
            text = header.read_text(encoding="utf-8", errors="replace")
            if re.search(r"\balignas\s*\(", text) and "disable : 4324" not in text:
                missing.append(header.relative_to(_REPO_ROOT).as_posix())
        self.assertEqual([], missing)


if __name__ == "__main__":
    unittest.main()
