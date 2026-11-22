# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Compile commands and headers that stay the same from one commit to the next.

ccache hashes a unit's compile command and its preprocessed text. A git hash in
a target's compile definitions changes the command of every unit of the target
with every commit, and __DATE__ or __TIME__ in a header changes the text of
every unit that includes it with every build, so neither unit can ever be a
cache hit. Only the one source file per library that reports the build stamp
may see them.

@since 3.0.0
"""

import re
import subprocess
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]

# The only sources that report the build date and commit
BUILD_STAMP_SOURCES = {"src/libraries/base/base_global.cpp", "src/libraries/acq/acq_global.cpp"}

_TIME_MACRO = re.compile(r"__DATE__|__TIME__|__TIMESTAMP__")
_HASH_DEFINITION = re.compile(r"MNE_GIT_HASH_\w+\s*=")


def tracked(*patterns: str) -> list[str]:
    files = subprocess.run(["git", "ls-files", *patterns], cwd=REPO_ROOT, capture_output=True, text=True, check=True)
    return [f for f in files.stdout.split() if "/external/" not in f]


def code(text: str) -> str:
    """The text without // and /* */ comments."""
    return re.sub(r"//[^\n]*|/\*.*?\*/", "", text, flags=re.DOTALL)


def cmake_code(text: str) -> str:
    return re.sub(r"#[^\n]*", "", text)


class TestCompilerCache(unittest.TestCase):
    def test_only_the_build_stamp_sources_expand_time_macros(self):
        found = [name for name in tracked("src/*.h", "src/*.hpp", "src/*.cpp")
                 if name not in BUILD_STAMP_SOURCES
                 and _TIME_MACRO.search(code((REPO_ROOT / name).read_text(encoding="utf-8", errors="replace")))]
        self.assertEqual(found, [])

    def test_git_hash_is_defined_only_for_the_build_stamp_sources(self):
        found = []
        for name in tracked("src/*CMakeLists.txt", "src/*.cmake", "cmake/*.cmake"):
            text = cmake_code((REPO_ROOT / name).read_text(encoding="utf-8", errors="replace"))
            for call in re.finditer(r"\b(\w+)\s*\(([^()]*(?:\([^()]*\)[^()]*)*)\)", text):
                if not _HASH_DEFINITION.search(call.group(2)):
                    continue
                arguments = call.group(2).split()
                allowed = (call.group(1) == "set_property" and arguments[:1] == ["SOURCE"]
                           and f"{Path(name).parent.as_posix()}/{arguments[1]}" in BUILD_STAMP_SOURCES)
                if not allowed:
                    found.append(f"{name}: {call.group(1)}(...)")
        self.assertEqual(found, [])


if __name__ == "__main__":
    unittest.main()
