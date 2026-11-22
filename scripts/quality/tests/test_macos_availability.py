# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Library calls that the minimum supported macOS does not provide.

Apple's libc++ marks some functions, such as floating-point std::from_chars, as
introduced in a later macOS release. They compile on a machine whose build has
no deployment target but fail in release builds, which target the macOS minimum
set in src/CMakeLists.txt. This test compiles every unit that uses such a
function against that minimum, using the commands of an existing local build.

@since 3.0.0
"""

import json
import os
import platform
import re
import shlex
import subprocess
import unittest
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
BUILD = Path(os.environ.get("MNE_CPP_BUILD_DIR", REPO_ROOT / "build" / "developer-dynamic"))

# Calls whose availability on Apple platforms depends on the deployment target
_GUARDED = re.compile(r"\bfrom_chars\b|matrix_io\.h")


def minimum_macos() -> str:
    text = (REPO_ROOT / "src" / "CMakeLists.txt").read_text(encoding="utf-8")
    return re.search(r'CMAKE_OSX_DEPLOYMENT_TARGET\s+"([\d.]+)"', text).group(1)


def syntax_only(arguments: list[str], minimum: str) -> list[str]:
    """The compile command checking syntax against @p minimum, without output files or precompiled headers."""
    result = []
    skip = 0
    for index, argument in enumerate(arguments):
        if skip:
            skip -= 1
            continue
        if argument in ("-o", "-MF", "-MT", "-MQ"):
            skip = 1
        elif argument == "-Xclang" and index + 1 < len(arguments) and arguments[index + 1] in ("-include-pch", "-include"):
            skip = 3
        elif argument in ("-c", "-MD", "-MMD", "-Winvalid-pch") or argument.startswith("-mmacosx-version-min"):
            continue
        else:
            result.append(argument)
    return result + ["-fsyntax-only", f"-mmacosx-version-min={minimum}"]


@unittest.skipUnless(platform.system() == "Darwin", "Apple availability attributes exist only in the macOS SDK")
@unittest.skipUnless((BUILD / "compile_commands.json").exists(), "needs a configured build with compile_commands.json")
class TestMacosAvailability(unittest.TestCase):
    def test_units_compile_for_the_minimum_macos(self):
        minimum = minimum_macos()
        entries = []
        for entry in json.loads((BUILD / "compile_commands.json").read_text(encoding="utf-8")):
            source = Path(entry["file"])
            if "external" in source.parts or not source.exists():
                continue
            if _GUARDED.search(source.read_text(encoding="utf-8", errors="replace")):
                arguments = entry.get("arguments") or shlex.split(entry["command"])
                entries.append((entry["directory"], source, syntax_only(arguments, minimum)))
        self.assertTrue(entries, "no unit uses a guarded call; the pattern is stale")

        def compile_unit(item):
            directory, source, arguments = item
            run = subprocess.run(arguments, cwd=directory, capture_output=True, text=True)
            errors = [line for line in run.stderr.splitlines() if "is unavailable: introduced in macOS" in line]
            return f"{source.relative_to(REPO_ROOT)}: {errors[0] if errors else run.stderr[:300]}" if run.returncode else None

        with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
            failures = [failure for failure in pool.map(compile_unit, entries) if failure]
        self.assertEqual(failures, [], f"units do not compile for macOS {minimum}")


if __name__ == "__main__":
    unittest.main()
