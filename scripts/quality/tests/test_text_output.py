# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Files the libraries write end their lines with LF on every platform.

A std::ofstream in text mode writes CR LF on Windows, so a label, event or
selection file written there differs from the one written elsewhere and from
what MNE-C, MNE-Python and FreeSurfer write. The libraries open their output
streams in binary mode.

@since 3.0.0
"""

import re
import subprocess
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]

_ARGS = r"\(((?:[^()]|\([^()]*\))*)\)"


def text_mode_writers(files: list[Path], base: Path) -> list[str]:
    """'<file>:<line>' for every std::ofstream constructed or opened with arguments but without std::ios::binary."""
    found = []
    for path in files:
        text = path.read_text(encoding="utf-8", errors="replace")
        names = set(re.findall(r"std::ofstream\s+(\w+)\s*[;({]", text))
        patterns = [re.compile(r"std::ofstream(?:\s+\w+)?\s*" + _ARGS)]
        patterns += [re.compile(rf"\b{re.escape(name)}\.open\s*" + _ARGS) for name in names]
        for pattern in patterns:
            for match in pattern.finditer(text):
                args = match.group(1).strip()
                if args and "binary" not in args:
                    found.append(f"{path.relative_to(base).as_posix()}:{text.count(chr(10), 0, match.start()) + 1}")
    return sorted(found)


class TestTextOutput(unittest.TestCase):
    def test_library_writers_are_binary(self):
        names = subprocess.run(["git", "-C", str(REPO_ROOT), "ls-files", "src/libs/mne"], capture_output=True, text=True,
                               check=True).stdout.split()
        files = [REPO_ROOT / n for n in names if n.endswith((".cpp", ".h"))]
        self.assertEqual(text_mode_writers(files, REPO_ROOT), [])

    def test_detects_a_text_mode_writer(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "a.cpp").write_text(
                "void f(const std::filesystem::path& p)\n{\n    std::ofstream out(p);\n    std::ofstream ok(p, std::ios::binary);\n"
                "    std::ofstream later;\n    later.open(p, std::ios::app);\n    void g(std::ofstream& s);\n}\n")
            self.assertEqual(text_mode_writers([root / "a.cpp"], root), ["a.cpp:3", "a.cpp:6"])


if __name__ == "__main__":
    unittest.main()
