"""The sanitizer build links a test_acq_* test to mne_acq alone unless its CMakeLists names more libraries."""

import re
import unittest
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[3]


class TestSanitizerAcqTests(unittest.TestCase):
    def test_default_acq_tests_include_only_acq(self) -> None:
        cmake = (_REPO_ROOT / "scripts" / "ci" / "sanitizers" / "CMakeLists.txt").read_text(encoding="utf-8")
        special = set(re.findall(r'_name STREQUAL "(test_acq_\w+)"', cmake))
        offending = []
        for source in sorted((_REPO_ROOT / "src" / "testframes").glob("test_acq_*/test_acq_*.cpp")):
            if source.stem in special:
                continue
            for library in re.findall(r"#include\s*<mne/(\w+)/", source.read_text(encoding="utf-8")):
                if library != "acq":
                    offending.append(f"{source.name}: mne/{library}")
        self.assertEqual([], offending)


if __name__ == "__main__":
    unittest.main()
