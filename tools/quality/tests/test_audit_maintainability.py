#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     September, 2026
#
#
# @brief    Tests for the maintainability baseline audit (T0.5).
"""Tests for ``tools/quality/audit_maintainability.py``."""

from __future__ import annotations

import copy
import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

_SCRIPT_PATH = Path(__file__).resolve().parents[1] / "audit_maintainability.py"
_spec = importlib.util.spec_from_file_location("audit_maintainability", _SCRIPT_PATH)
assert _spec is not None and _spec.loader is not None
audit = importlib.util.module_from_spec(_spec)
sys.modules["audit_maintainability"] = audit
_spec.loader.exec_module(audit)


def counted(snippet: str) -> dict[str, int]:
    return {name: value for name, value in audit.count_file(snippet).items() if value}


class PatternTests(unittest.TestCase):
    def test_owned_allocations_are_not_counted(self) -> None:
        for snippet in (
            "FiffStream::SPtr stream(new FiffStream(&file));",
            "QSharedPointer<Foo> x(new Foo());",
            "x.reset(new Foo());",
            "m_p = new Foo(this);",
            "m_p = new Foo(data, this);",
            "auto p = std::make_unique<Foo>();",
        ):
            self.assertEqual(counted(snippet), {}, snippet)

    def test_unowned_allocations_are_counted_separately_for_qt(self) -> None:
        self.assertEqual(counted("Foo* f = new Foo;"), {"raw_new": 1})
        self.assertEqual(counted("auto* l = new QGridLayout();"), {"qt_new_unparented": 1})

    def test_comments_and_strings_do_not_count(self) -> None:
        for snippet in ('// new Foo; NULL (int)x', '/* delete p; */', 'qInfo("NULL printf(")', "int n = 1'000;"):
            self.assertEqual(counted(snippet), {}, snippet)

    def test_legacy_constructs_are_counted(self) -> None:
        self.assertEqual(counted("int y = (int)x;"), {"c_style_cast": 1})
        self.assertEqual(counted("f(a, (const double *)b);"), {"c_style_cast": 1})
        self.assertEqual(counted("sizeof(int) * 2; foo(int) ;"), {})
        self.assertEqual(counted("if (h == NULL)"), {"null_macro": 1})
        self.assertEqual(counted("typedef struct { int a; } rec;"), {"typedef_struct": 1})
        self.assertEqual(counted("delete[] buf; delete p;"), {"raw_delete": 2})
        self.assertEqual(counted('fprintf(stderr, "x"); std::cout << 1;'), {"console_io": 2})
        self.assertEqual(counted("m.printf();"), {})
        self.assertEqual(counted("#define FIFF_FILE_ID 100\n#define STR(x) #x\n"), {"numeric_define": 1})


class RatchetTests(unittest.TestCase):
    def test_growth_in_total_or_single_file_fails(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "src/libraries/a").mkdir(parents=True)
            (root / "src/libraries/a/a.cpp").write_text("int* p = NULL;\n", encoding="utf-8")
            (root / "src/libraries/a/b.cpp").write_text("int* q = NULL;\n", encoding="utf-8")
            policy = {"scan_roots": ["src/libraries"], "extensions": [".cpp"], "oversized_lines": 1}
            baseline = audit.build_report(root, policy)
            self.assertEqual(baseline["metrics"]["null_macro"]["total"], 2)
            self.assertEqual(audit.check(baseline, baseline), [])

            (root / "src/libraries/a/a.cpp").write_text("int* p = NULL; int* r = NULL;\n", encoding="utf-8")
            (root / "src/libraries/a/b.cpp").write_text("int* q = nullptr;\n", encoding="utf-8")
            moved = audit.build_report(root, policy)
            self.assertEqual(moved["metrics"]["null_macro"]["total"], 2)
            failures = audit.check(moved, baseline)
            self.assertTrue(any("a/a.cpp has 2, baseline 1" in f for f in failures), failures)

    def test_suppression_hides_one_file_from_one_metric(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "src/tools/t").mkdir(parents=True)
            (root / "src/tools/t/main.cpp").write_text('fprintf(stderr, "x"); int* p = NULL;\n', encoding="utf-8")
            policy = {"scan_roots": ["src/tools"], "extensions": [".cpp"], "oversized_lines": 100,
                      "suppressions": {"console_io": {"src/tools/t/main.cpp": "progress output"}}}
            report = audit.build_report(root, policy)
        self.assertEqual(report["metrics"]["console_io"]["total"], 0)
        self.assertEqual(report["metrics"]["console_io"]["suppressed"], ["src/tools/t/main.cpp"])
        self.assertEqual(report["metrics"]["null_macro"]["total"], 1)


class RepositoryTests(unittest.TestCase):
    def test_repository_scan_is_deterministic_and_ratchet_detects_growth(self) -> None:
        report = audit.build_report()
        self.assertEqual(report, audit.build_report())
        self.assertEqual(audit.check(report, report), [])
        grown = copy.deepcopy(report)
        grown["metrics"]["null_macro"]["total"] += 1
        self.assertTrue(audit.check(grown, report))


if __name__ == "__main__":
    unittest.main()
