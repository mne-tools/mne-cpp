# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Tests for include_sections.py: every include under the banner of its kind.

@since 3.0.0
"""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import include_sections  # noqa: E402

RULE = "//" + "=" * 109


def banner(name: str) -> str:
    return f"{RULE}\n// {name}\n{RULE}\n"


MIXED = (
    "#ifndef FOO_H\n#define FOO_H\n\n"
    + banner("INCLUDES")
    + '\n#include "foo_global.h"\n\n#include <vector>\n#include <QString>\n\n'
    + banner("STD INCLUDES")
    + "\n#include <Eigen/Core>\n\n"
    + banner("QT INCLUDES")
    + "\n\n"
    + banner("DEFINE NAMESPACE mne")
    + "\nnamespace mne\n{\n}\n#endif\n"
)


class TestIncludeSections(unittest.TestCase):
    def test_kinds(self):
        self.assertEqual(include_sections.include_kind("<", "vector"), "STL INCLUDES")
        self.assertEqual(include_sections.include_kind("<", "Eigen/Core"), "EIGEN INCLUDES")
        self.assertEqual(include_sections.include_kind("<", "QString"), "QT INCLUDES")
        self.assertEqual(include_sections.include_kind("<", "QtTest"), "QT INCLUDES")
        self.assertEqual(include_sections.include_kind("<", "fiff/fiff_info.h"), "INCLUDES")
        self.assertEqual(include_sections.include_kind("<", "windows.h"), "INCLUDES")
        self.assertEqual(include_sections.include_kind('"', "vector"), "INCLUDES")

    def test_reports_misplaced_includes_banners_and_empty_sections(self):
        messages = [message for _, message in include_sections.check(MIXED)]
        self.assertIn("<vector> belongs under STL INCLUDES, not INCLUDES", messages)
        self.assertIn("<QString> belongs under QT INCLUDES, not INCLUDES", messages)
        self.assertIn("<Eigen/Core> belongs under EIGEN INCLUDES, not STL INCLUDES", messages)
        self.assertIn("banner 'STD INCLUDES' is not one of INCLUDES, QT INCLUDES, EIGEN INCLUDES, STL INCLUDES", messages)
        self.assertIn("banner 'QT INCLUDES' has no includes", messages)

    def test_fix_regroups_in_order_and_keeps_the_rest(self):
        fixed = include_sections.fix(MIXED)
        self.assertEqual(include_sections.check(fixed), [])
        expected = (
            "#ifndef FOO_H\n#define FOO_H\n\n"
            + banner("INCLUDES")
            + '\n#include "foo_global.h"\n\n'
            + banner("QT INCLUDES")
            + "\n#include <QString>\n\n"
            + banner("EIGEN INCLUDES")
            + "\n#include <Eigen/Core>\n\n"
            + banner("STL INCLUDES")
            + "\n#include <vector>\n\n"
            + banner("DEFINE NAMESPACE mne")
            + "\nnamespace mne\n{\n}\n#endif\n"
        )
        self.assertEqual(fixed, expected)

    def test_own_header_first_and_conditional_groups_move_whole(self):
        text = (
            banner("INCLUDES")
            + '\n#include <cmath>\n#include "bar.h"\n#include "foo.h"\n'
            + "#ifdef _WIN32\n#include <windows.h>\n#endif\n#if 1\n#include <thread>\n#endif\n\n"
            + banner("USED NAMESPACES")
        )
        fixed = include_sections.fix(text, include_sections.own_header("src/x/foo.cpp"))
        self.assertEqual(
            fixed,
            banner("INCLUDES")
            + '\n#include "foo.h"\n\n#include "bar.h"\n#ifdef _WIN32\n#include <windows.h>\n#endif\n\n'
            + banner("STL INCLUDES")
            + "\n#include <cmath>\n#if 1\n#include <thread>\n#endif\n\n"
            + banner("USED NAMESPACES"),
        )

    def test_duplicates_go_and_local_headers_come_before_library_headers(self):
        text = (
            banner("INCLUDES")
            + '\n#include "foo.h"\n#include <mne/base/log.h>\n#include "bar.h"\n#include <mne/base/log.h>\n#include <vector>\n'
        )
        self.assertEqual(
            include_sections.fix(text, "foo.h"),
            banner("INCLUDES")
            + '\n#include "foo.h"\n\n#include "bar.h"\n#include <mne/base/log.h>\n\n'
            + banner("STL INCLUDES")
            + "\n#include <vector>\n",
        )

    def test_a_macro_in_the_block_is_left_to_a_person(self):
        text = banner("INCLUDES") + "\n#define _USE_MATH_DEFINES\n#include <cmath>\n#include \"foo.h\"\n"
        self.assertTrue(include_sections.check(text))
        self.assertIsNone(include_sections.fix(text))

    def test_a_structured_file_is_unchanged(self):
        text = banner("INCLUDES") + '\n#include "foo.h"\n\n' + banner("STL INCLUDES") + "\n#include <vector>\n"
        self.assertEqual(include_sections.check(text), [])
        self.assertEqual(include_sections.fix(text), text)


if __name__ == "__main__":
    unittest.main()
