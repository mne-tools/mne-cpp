# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""The sanitizer build links every test_acq_* test to each library whose headers it includes.

scripts/ci/sanitizers/CMakeLists.txt builds the acq tests against static libraries with their own link lines;
a library reached by none of them leaves its symbols undefined. Static libraries pass their PRIVATE
dependencies on too, so the closure follows PUBLIC and PRIVATE links.

@since 3.0.0
"""

import re
import unittest
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[3]
_LIBS = _REPO_ROOT / "src" / "libs" / "mne"
_SANITIZERS = _REPO_ROOT / "scripts" / "ci" / "sanitizers" / "CMakeLists.txt"
_SUBLIBRARIES = ("acq/devices", "acq/fiff", "acq/lsl")


def library_links() -> dict[str, set[str]]:
    """mne_* target -> the mne_* targets its CMakeLists links."""
    links: dict[str, set[str]] = {}
    for cmake in _LIBS.rglob("CMakeLists.txt"):
        target = "mne_" + cmake.parent.relative_to(_LIBS).as_posix().replace("/", "_")
        text = cmake.read_text(encoding="utf-8")
        found = set()
        for call in re.findall(r"target_link_libraries\(\$\{PROJECT_NAME\}([^)]*)\)", text):
            found.update(re.findall(r"\bmne_\w+", call))
        links[target] = found
    return links


def closure(targets: set[str], links: dict[str, set[str]]) -> set[str]:
    reached, todo = set(), list(targets)
    while todo:
        target = todo.pop()
        if target not in reached:
            reached.add(target)
            todo.extend(links.get(target, ()))
    return reached


def test_links(cmake: str) -> dict[str, set[str]]:
    """Test name -> mne_* targets the sanitizer build links it to (the else branch under '*')."""
    result: dict[str, set[str]] = {}
    pattern = r'(?:if|elseif)\(_name STREQUAL "(\w+)"(?: OR _name STREQUAL "(\w+)")?\)\s*(.*?)(?=\n\s*(?:elseif|else|endif)\()'
    for first, second, body in re.findall(pattern, cmake, re.S):
        targets = set(re.findall(r"\bmne_\w+", " ".join(re.findall(r"target_link_libraries\(([^)]*)\)", body))))
        if targets:
            for name in filter(None, (first, second)):
                result[name] = targets
    default = re.search(r"else\(\)\s*target_link_libraries\(([^)]*)\)", cmake)
    result["*"] = set(re.findall(r"\bmne_\w+", default.group(1))) if default else set()
    return result


def included_libraries(source: Path) -> set[str]:
    targets = set()
    for path in re.findall(r"#include\s*<mne/([\w/]+)/[\w.]+>", source.read_text(encoding="utf-8")):
        library = next((sub for sub in _SUBLIBRARIES if path == sub or path.startswith(sub + "/")), path.split("/")[0])
        targets.add("mne_" + library.replace("/", "_"))
    return targets


class TestSanitizerAcqTests(unittest.TestCase):
    def test_every_included_library_is_linked(self) -> None:
        links = library_links()
        per_test = test_links(_SANITIZERS.read_text(encoding="utf-8"))
        missing = []
        for source in sorted((_REPO_ROOT / "src" / "tests").glob("test_acq_*/test_acq_*.cpp")):
            linked = closure(per_test.get(source.stem, per_test["*"]), links)
            missing += [f"{source.stem}: {target}" for target in sorted(included_libraries(source) - linked)]
        self.assertEqual([], missing)


if __name__ == "__main__":
    unittest.main()
