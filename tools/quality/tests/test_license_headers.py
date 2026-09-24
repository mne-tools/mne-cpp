# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

"""Tests for the SPDX header validator in ``tools/license_headers``."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.license_headers import core  # noqa: E402

AUTHOR = "Christoph Dinh <christoph.dinh@mne-cpp.org>"


def errors(name: str, text: str) -> list[str]:
    with tempfile.TemporaryDirectory() as temp:
        path = Path(temp) / name
        path.write_text(text, encoding="utf-8")
        return core.validate_file(path)


class LineCommentHeaderTests(unittest.TestCase):
    def test_hash_header_after_shebang(self) -> None:
        text = f"#!/usr/bin/env python3\n# SPDX-License-Identifier: BSD-3-Clause\n# Copyright (c) 2026 MNE-CPP Authors\n#   {AUTHOR}\n\nx = 1\n"
        self.assertEqual(errors("tool.py", text), [])

    def test_batch_prefixes(self) -> None:
        rem = f"@echo off\nrem SPDX-License-Identifier: BSD-3-Clause\nrem Copyright (c) 2026 MNE-CPP Authors\nrem   {AUTHOR}\n"
        polyglot = f":;# SPDX-License-Identifier: BSD-3-Clause\n:;# Copyright (c) 2026 MNE-CPP Authors\n:;#   {AUTHOR}\n"
        self.assertEqual(errors("run.bat", rem), [])
        self.assertEqual(errors("run.bat", polyglot), [])

    def test_line_comment_form_needs_no_doxygen_author_tag(self) -> None:
        text = f"# SPDX-License-Identifier: BSD-3-Clause\n# Copyright (c) 2026 MNE-CPP Authors\n#   {AUTHOR}\n"
        self.assertEqual(errors("CMakeLists.txt", text), [])

    def test_missing_or_misplaced_header_fails(self) -> None:
        self.assertIn("missing SPDX header block", errors("tool.py", "x = 1\n")[0])
        late = f"#!/bin/sh\necho hi\n# SPDX-License-Identifier: BSD-3-Clause\n# Copyright (c) 2026 MNE-CPP Authors\n#   {AUTHOR}\n"
        self.assertIn("missing SPDX header block", errors("run.sh", late)[0])
        no_author = "# SPDX-License-Identifier: BSD-3-Clause\n# Copyright (c) 2026 MNE-CPP Authors\n\nx = 1\n"
        self.assertIn("no author lines", errors("tool.py", no_author)[0])


class DoxygenHeaderTests(unittest.TestCase):
    def test_lowercase_git_handle_is_an_author(self) -> None:
        text = ("/**\n * SPDX-License-Identifier: BSD-3-Clause\n * Copyright (c) 2017-2026 MNE-CPP Authors\n *\n"
                f" * @file     a.cpp\n * @author   {AUTHOR};\n *           johaenns <j.vorw01@gmail.com>\n"
                " * @brief    A.\n */\n")
        self.assertEqual(errors("a.cpp", text), [])

    def test_doxygen_block_still_requires_one_author_tag(self) -> None:
        text = ("/**\n * SPDX-License-Identifier: BSD-3-Clause\n * Copyright (c) 2026 MNE-CPP Authors\n *\n"
                f" * @file     a.cpp\n * @author   {AUTHOR}\n * @author   {AUTHOR}\n */\n")
        self.assertIn("exactly one ``@author``", errors("a.cpp", text)[0])


class RepositoryTests(unittest.TestCase):
    def test_every_in_scope_file_has_a_valid_header(self) -> None:
        repo = core.repo_root_from_here()
        failures = [error for path in core.iter_in_scope(repo) for error in core.validate_file(path)]
        self.assertEqual(failures, [])


if __name__ == "__main__":
    unittest.main()
