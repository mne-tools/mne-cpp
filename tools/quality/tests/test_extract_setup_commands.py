#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Checks for ``tools/onboarding/extract_setup_commands.py`` (T10.8a)."""

from __future__ import annotations

import contextlib
import importlib.util
import io
import sys
import tempfile
import unittest
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[3]
_spec = importlib.util.spec_from_file_location("extract_setup_commands",
                                               _REPO_ROOT / "tools" / "onboarding" / "extract_setup_commands.py")
assert _spec is not None and _spec.loader is not None
extractor = importlib.util.module_from_spec(_spec)
sys.modules["extract_setup_commands"] = extractor
_spec.loader.exec_module(extractor)

DOC = """# Setup

{/* onboarding: unix */}
```bash
# fetch the dependencies
./init.sh

cmake --build build --parallel
```

An unmarked block is documentation only:

```bash
./scripts/build_project.sh all
```

{/* onboarding: windows */}
```bat
REM Windows
.\\init.bat
```

{/* onboarding: unix */}
```bash
./out/Release/examples/ex_math
```
"""


class ExtractSetupCommandsTest(unittest.TestCase):
    def test_marked_blocks_in_order_without_comments(self) -> None:
        self.assertEqual(extractor.extract(DOC, "doc.mdx"), {
            "unix": ["./init.sh", "cmake --build build --parallel", "./out/Release/examples/ex_math"],
            "windows": [".\\init.bat"],
        })

    def test_marker_without_block_fails(self) -> None:
        with self.assertRaisesRegex(ValueError, "doc.mdx:2: onboarding marker 'unix'"):
            extractor.extract("text\n{/* onboarding: unix */}\nnot a block\n", "doc.mdx")

    def test_committed_commands_match_the_docs(self) -> None:
        self.assertEqual(extractor.main(["--check"]), 0)

    def test_edited_doc_command_is_drift(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            for doc in extractor.DOCS:
                text = (_REPO_ROOT / doc).read_text(encoding="utf-8")
                self.assertIn("./init.sh\n", text)
                target = Path(tmp) / doc
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text(text.replace("./init.sh\n", "./init.sh --linkage static\n", 1), encoding="utf-8")
            original = extractor.REPO_ROOT
            extractor.REPO_ROOT = Path(tmp)
            try:
                with contextlib.redirect_stderr(io.StringIO()):
                    self.assertEqual(extractor.main(["--check"]), 1)
            finally:
                extractor.REPO_ROOT = original


if __name__ == "__main__":
    unittest.main()
