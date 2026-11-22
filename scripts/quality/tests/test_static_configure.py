# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""The static configuration that the static release packages build.

With BUILD_SHARED_LIBS=OFF every library is static: a shared one would link the
static libraries, which are not position-independent, and the installed SDK
must export every target the libraries need, such as the resource targets of
qt_add_shaders. This test configures the tree that way with the Qt and Eigen of
an existing local build and reads the targets from the CMake file API.

@since 3.0.0
"""

import json
import os
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
BUILD = Path(os.environ.get("MNE_CPP_BUILD_DIR", REPO_ROOT / "build" / "developer-dynamic"))


def prefix_path() -> str | None:
    """The CMAKE_PREFIX_PATH of the local build, which finds Qt and Eigen."""
    cache = BUILD / "CMakeCache.txt"
    if not cache.exists():
        return None
    match = re.search(r"^CMAKE_PREFIX_PATH:[A-Z]+=(.*)$", cache.read_text(encoding="utf-8"), re.MULTILINE)
    return match.group(1) if match else None


def target_types(build: Path) -> dict[str, str]:
    """Name -> type of every target, from the codemodel reply of the CMake file API."""
    reply = build / ".cmake" / "api" / "v1" / "reply"
    index = json.loads(next(reply.glob("index-*.json")).read_text(encoding="utf-8"))
    codemodel = next(o for o in index["objects"] if o["kind"] == "codemodel")
    model = json.loads((reply / codemodel["jsonFile"]).read_text(encoding="utf-8"))
    types = {}
    for target in model["configurations"][0]["targets"]:
        detail = json.loads((reply / target["jsonFile"]).read_text(encoding="utf-8"))
        types[detail["name"]] = detail["type"]
    return types


@unittest.skipUnless(shutil.which("cmake"), "needs cmake")
@unittest.skipUnless(prefix_path(), "needs a configured local build that finds Qt")
class TestStaticConfigure(unittest.TestCase):
    def test_static_configuration_has_only_static_libraries(self):
        with tempfile.TemporaryDirectory() as directory:
            build = Path(directory)
            query = build / ".cmake" / "api" / "v1" / "query"
            query.mkdir(parents=True)
            (query / "codemodel-v2").touch()
            run = subprocess.run(["cmake", "-S", str(REPO_ROOT / "src"), "-B", str(build), "-DBUILD_SHARED_LIBS=OFF",
                                  "-DCMAKE_BUILD_TYPE=Release", f"-DCMAKE_PREFIX_PATH={prefix_path()}"],
                                 capture_output=True, text=True)
            errors = [line for line in run.stderr.splitlines() if "CMake Error" in line or "requires target" in line]
            self.assertEqual(run.returncode, 0, "\n".join(errors) or run.stderr[-2000:])
            shared = sorted(name for name, kind in target_types(build).items() if kind == "SHARED_LIBRARY")
            self.assertEqual(shared, [], "shared libraries in a static build")


if __name__ == "__main__":
    unittest.main()
