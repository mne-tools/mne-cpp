# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Export macros of plugins and applications.

A shared module exports its symbols only while the definition that selects
Q_DECL_EXPORT in its *_global.h is set by its CMakeLists.txt. Otherwise the
module declares its own classes dllimport, which only MSVC rejects.

@since 3.0.0
"""

import re
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
SCANNED = [REPO_ROOT / "src" / "apps", REPO_ROOT / "src" / "tools"]

_EXPORT_BRANCH = re.compile(r"^#\s*(?:el)?if\s+defined\((\w+)\)\s*\n#\s*define\s+\w+\s+Q_DECL_EXPORT", re.MULTILINE)


def missing_export_definitions(roots: list[Path], base: Path = REPO_ROOT) -> list[str]:
    """Lists '<header>: <macro>' for every export macro that no CMakeLists.txt of the module defines."""
    missing = []
    for root in roots:
        for header in sorted(root.rglob("*_global.h")):
            text = header.read_text(encoding="utf-8", errors="replace")
            cmake_texts = [p.read_text(encoding="utf-8", errors="replace") for p in (header.parent / "CMakeLists.txt", header.parent.parent / "CMakeLists.txt") if p.is_file()]
            for macro in _EXPORT_BRANCH.findall(text):
                if not any(re.search(rf"\b{macro}\b", cmake) for cmake in cmake_texts):
                    missing.append(f"{header.relative_to(base).as_posix()}: {macro}")
    return missing


class TestExportMacros(unittest.TestCase):
    def test_every_export_macro_is_defined_by_its_cmake(self):
        self.assertEqual(missing_export_definitions(SCANNED), [])

    def test_detects_an_undefined_export_macro(self):
        with tempfile.TemporaryDirectory() as tmp:
            module = Path(tmp)
            (module / "CMakeLists.txt").write_text("target_compile_definitions(x PRIVATE SCAN_X_PLUGIN)\n")
            (module / "x_global.h").write_text("#if defined(X_LIBRARY)\n#define XSHARED_EXPORT Q_DECL_EXPORT\n#endif\n")
            self.assertEqual(missing_export_definitions([module], module), ["x_global.h: X_LIBRARY"])

if __name__ == "__main__":
    unittest.main()
