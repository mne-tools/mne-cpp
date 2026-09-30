#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     September, 2026
#
#
# @brief    Fixtures for the Doxygen warning ratchet and its entry point (T4.1).
"""Fixtures for ``tools/quality/check_doxygen_warnings.py`` and ``doc/build-api-docs.sh``."""

from __future__ import annotations

import importlib.util
import io
import json
import re
import shutil
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

_QUALITY_DIR = Path(__file__).resolve().parent.parent
_REPO_ROOT = _QUALITY_DIR.parents[1]
_spec = importlib.util.spec_from_file_location("check_doxygen_warnings", _QUALITY_DIR / "check_doxygen_warnings.py")
assert _spec is not None and _spec.loader is not None
checker = importlib.util.module_from_spec(_spec)
sys.modules["check_doxygen_warnings"] = checker
_spec.loader.exec_module(checker)

ROOT = "/checkout/mne-cpp"
LOG = f"""{ROOT}/src/libraries/fiff/a.h:12: warning: unable to resolve reference to 'Foo' for \\ref command
{ROOT}/src/libraries/fiff/a.h:40: warning: The following parameter of FIFFLIB::f(int x) is not documented:
  parameter 'x'
{ROOT}/src/libraries/fiff/b.h:7: warning: end of comment block while expecting command </tt> (Probable start '{ROOT}/src/libraries/fiff/b.h' at line 6)
"""


class RatchetTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.tmp)
        self.allowlist = self.tmp / "allowlist.json"

    def run_checker(self, log: str, *extra: str) -> tuple[int, str]:
        path = self.tmp / "warnings.log"
        path.write_text(log.replace(ROOT, str(_REPO_ROOT)), encoding="utf-8")
        out = io.StringIO()
        with redirect_stdout(out), redirect_stderr(io.StringIO()):
            code = checker.main([str(path), "--allowlist", str(self.allowlist), *extra])
        return code, out.getvalue()

    def seed(self, log: str = LOG) -> None:
        self.assertEqual(0, self.run_checker(log, "--init")[0])


class TestKeys(unittest.TestCase):
    def test_keys_are_relative_and_line_free(self) -> None:
        keys = checker.parse_log(LOG, Path(ROOT))
        self.assertEqual(3, sum(keys.values()))
        self.assertIn("src/libraries/fiff/a.h: warning: The following parameter of FIFFLIB::f(int x) "
                      "is not documented: | parameter 'x'", keys)
        self.assertIn("src/libraries/fiff/b.h: warning: end of comment block while expecting command </tt> "
                      "(Probable start 'src/libraries/fiff/b.h' at line N)", keys)
        self.assertIn("src/libraries/fiff/a.h: warning: unable to resolve reference to 'Foo' for \\ref command", keys)
        self.assertFalse(any(ROOT in key for key in keys))

    def test_shifted_lines_keep_the_same_key(self) -> None:
        moved = LOG.replace("a.h:12:", "a.h:99:").replace("at line 6", "at line 60")
        self.assertEqual(checker.parse_log(LOG, Path(ROOT)), checker.parse_log(moved, Path(ROOT)))

    def test_repeated_warning_is_counted(self) -> None:
        line = LOG.splitlines()[0]
        self.assertEqual(2, checker.parse_log(f"{line}\n{line}\n", Path(ROOT)).most_common(1)[0][1])


class TestRatchet(RatchetTestCase):
    def test_known_warnings_pass(self) -> None:
        self.seed()
        code, text = self.run_checker(LOG)
        self.assertEqual(0, code)
        self.assertIn("PASS", text)

    def test_new_warning_fails(self) -> None:
        """AC-T4.1-1: a warning outside the allowlist fails the check."""
        self.seed()
        extra = f"{ROOT}/src/libraries/fiff/c.h:3: warning: Found unknown command '@bogus'\n"
        code, text = self.run_checker(LOG + extra)
        self.assertEqual(1, code)
        self.assertIn("NEW: src/libraries/fiff/c.h: warning: Found unknown command '@bogus'", text)

    def test_extra_occurrence_of_a_known_warning_fails(self) -> None:
        self.seed()
        self.assertEqual(1, self.run_checker(LOG + LOG.splitlines()[0] + "\n")[0])

    def test_update_cannot_grow_the_allowlist(self) -> None:
        """AC-T4.1-2: --update refuses a new key or a higher count and leaves the file untouched."""
        self.seed()
        before = self.allowlist.read_text(encoding="utf-8")
        extra = f"{ROOT}/src/libraries/fiff/c.h:3: warning: Found unknown command '@bogus'\n"
        self.assertEqual(1, self.run_checker(LOG + extra, "--update")[0])
        self.assertEqual(before, self.allowlist.read_text(encoding="utf-8"))

    def test_update_shrinks_after_a_fix(self) -> None:
        self.seed()
        fixed = "\n".join(LOG.splitlines()[1:]) + "\n"
        code, text = self.run_checker(fixed)
        self.assertEqual(0, code)
        self.assertIn("1 fixed since", text)
        self.assertEqual(0, self.run_checker(fixed, "--update")[0])
        self.assertEqual(2, json.loads(self.allowlist.read_text(encoding="utf-8"))["total"])
        self.assertEqual(1, self.run_checker(LOG)[0])

    def test_init_refuses_to_overwrite(self) -> None:
        self.seed()
        self.assertEqual(1, self.run_checker(LOG, "--init")[0])


class TestRepositoryConfiguration(unittest.TestCase):
    def test_doxyfile_enables_documentation_warnings(self) -> None:
        text = (_REPO_ROOT / "doc" / "Doxyfile").read_text(encoding="utf-8")
        for option in ("WARNINGS", "WARN_IF_UNDOCUMENTED", "WARN_IF_DOC_ERROR",
                       "WARN_IF_INCOMPLETE_DOC", "WARN_NO_PARAMDOC"):
            with self.subTest(option=option):
                self.assertRegex(text, rf"(?m)^{option}\s*=\s*YES\s*$")
        self.assertRegex(text, r"(?m)^WARN_LOGFILE\s*=\s*xml_out/doxygen-warnings\.log\s*$")

    def test_allowlist_is_consistent(self) -> None:
        data = json.loads(checker.ALLOWLIST.read_text(encoding="utf-8"))
        self.assertEqual(data["total"], sum(data["warnings"].values()))
        self.assertTrue(all(count >= 1 for count in data["warnings"].values()))
        self.assertFalse(any(re.search(r"/(Users|home)/", key) for key in data["warnings"]))

    def test_entry_point_is_clean_and_pinned(self) -> None:
        """AC-T4.1-3: the documented command starts from an empty xml_out with a pinned Doxygen."""
        script = (_REPO_ROOT / "doc" / "build-api-docs.sh").read_text(encoding="utf-8")
        self.assertIn('DOXYGEN_VERSION="1.16.1"', script)
        commands = [line.strip() for line in script.splitlines() if line.strip() and not line.lstrip().startswith("#")]

        def position(fragment: str) -> int:
            found = [i for i, line in enumerate(commands) if fragment in line]
            self.assertTrue(found, f"build-api-docs.sh has no '{fragment}' step")
            return found[0]

        steps = ["rm -rf doc/xml_out", "doxygen Doxyfile", "check_doxygen_warnings.py", "doxy2mdx.py"]
        self.assertEqual(sorted(map(position, steps)), list(map(position, steps)))
        for workflow in ("staging.yml", "main.yml"):
            text = (_REPO_ROOT / ".github" / "workflows" / workflow).read_text(encoding="utf-8")
            self.assertIn("Release_1_16_1/doxygen-1.16.1", text, workflow)


if __name__ == "__main__":
    unittest.main()
