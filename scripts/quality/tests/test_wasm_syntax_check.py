# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""The WebAssembly-only code paths compile, checked with the native compiler.

@since 3.0.0
"""

import subprocess
import sys
import unittest
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[3]
_BUILD = _REPO_ROOT / "build" / "developer-dynamic"
_SCRIPT = _REPO_ROOT / "scripts" / "ci" / "wasm_syntax_check.py"


class TestWasmSyntax(unittest.TestCase):
    @unittest.skipUnless((_BUILD / "compile_commands.json").is_file(), "needs a configured build/developer-dynamic")
    def test_wasm_only_code_compiles(self) -> None:
        proc = subprocess.run([sys.executable, str(_SCRIPT), "--build", str(_BUILD)],
                              capture_output=True, text=True, check=False)
        self.assertEqual(0, proc.returncode, proc.stdout + proc.stderr)


if __name__ == "__main__":
    unittest.main()
