# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    3.0.0
# @date     November, 2026
#
#
# @brief    Tests for scripts/doxy2mdx/run_examples.py.
"""Tests for the example runner that collects the gallery's console output."""

from __future__ import annotations

import importlib.util
import os
import sys
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location("run_examples", REPO_ROOT / "scripts/doxy2mdx/run_examples.py")
runner = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(runner)


class RunTests(unittest.TestCase):
    @unittest.skipIf(os.name == "nt", "pseudo-terminals are POSIX only")
    def test_output_keeps_the_order_of_stdout_and_stderr(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            binary = root / "ex_order"
            binary.write_text(f"#!{sys.executable}\nimport sys\nprint('one')\n"
                              "sys.stderr.write('two\\n')\nprint('three')\n", encoding="utf-8")
            binary.chmod(0o755)
            ok, message = runner.run("ex_order", binary, root)
            self.assertTrue(ok, message)
            output = (root / runner.gen.GALLERY / "ex_order" / "output.txt").read_text(encoding="utf-8")
            self.assertEqual(output, "one\ntwo\nthree\n")


if __name__ == "__main__":
    unittest.main()
