# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""The standalone A/B build of mne_acq_bench (scripts/ci/acq_bench) links every library the tool includes.

That build adds the libraries one by one, without the rest of MNE-CPP; a library the tool includes but the
build leaves out fails only in the CI job that compares two commits.

@since 3.0.0
"""

import re
import unittest
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[3]
_MAIN = _REPO_ROOT / "src" / "tools" / "acquisition" / "mne_acq_bench" / "main.cpp"
_STANDALONE = _REPO_ROOT / "scripts" / "ci" / "acq_bench" / "CMakeLists.txt"


def unconditional_libraries(source: str) -> set[str]:
    """mne_* targets of the headers included outside #if blocks."""
    libraries, depth = set(), 0
    for line in source.splitlines():
        stripped = line.strip()
        if stripped.startswith("#if"):
            depth += 1
        elif stripped.startswith("#endif"):
            depth -= 1
        elif depth == 0:
            match = re.match(r"#include\s*<mne/(\w+)/", stripped)
            if match:
                libraries.add(f"mne_{match.group(1)}")
    return libraries


class TestAcqBenchStandaloneBuild(unittest.TestCase):
    def test_every_included_library_is_linked(self) -> None:
        cmake = _STANDALONE.read_text(encoding="utf-8")
        linked = set(re.findall(r"\bmne_\w+", " ".join(re.findall(r"target_link_libraries\(([^)]*)\)", cmake))))
        self.assertEqual(set(), unconditional_libraries(_MAIN.read_text(encoding="utf-8")) - linked)


if __name__ == "__main__":
    unittest.main()
