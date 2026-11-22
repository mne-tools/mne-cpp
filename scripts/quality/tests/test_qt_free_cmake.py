# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""cmake/MneQtFree.cmake: only the display libraries may use Qt.

src/libraries/CMakeLists.txt calls mne_require_qt_free() on every library added before disp and disp3D; the
configuration fails if one links a Qt target or runs one of Qt's code generators.

@since 3.0.0
"""

import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
MODULE = REPO_ROOT / "cmake" / "MneQtFree.cmake"


def configure(library: str) -> subprocess.CompletedProcess:
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        (root / "lib").mkdir()
        (root / "lib" / "a.cpp").write_text("int a() { return 1; }\n", encoding="utf-8")
        (root / "lib" / "CMakeLists.txt").write_text(library, encoding="utf-8")
        (root / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.21)\nproject(p LANGUAGES CXX)\n"
            f'include("{MODULE.as_posix()}")\nadd_subdirectory(lib)\n'
            "mne_require_qt_free(${CMAKE_CURRENT_SOURCE_DIR})\n", encoding="utf-8")
        return subprocess.run(["cmake", "-S", str(root), "-B", str(root / "build")], capture_output=True, text=True)


@unittest.skipUnless(shutil.which("cmake"), "needs cmake")
class TestQtFreeCmake(unittest.TestCase):
    def test_a_library_without_qt_passes(self):
        self.assertEqual(configure("add_library(a STATIC a.cpp)\n").returncode, 0)

    def test_a_qt_link_fails(self):
        run = configure("add_library(a STATIC a.cpp)\ntarget_link_libraries(a PUBLIC Qt6::Core)\n")
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("a links Qt6::Core", run.stderr)

    def test_a_qt_code_generator_fails(self):
        run = configure("add_library(a STATIC a.cpp)\nset_target_properties(a PROPERTIES AUTOMOC ON)\n")
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("a has AUTOMOC on", run.stderr)


if __name__ == "__main__":
    unittest.main()
