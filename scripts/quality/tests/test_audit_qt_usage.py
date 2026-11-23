#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    3.0.0
# @date     November, 2026
#
#
# @brief    Tests for the per-library Qt usage audit.
"""Tests for ``scripts/quality/audit_qt_usage.py``."""

from __future__ import annotations

import copy
import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path

_SCRIPT_PATH = Path(__file__).resolve().parents[1] / "audit_qt_usage.py"
sys.path.insert(0, str(_SCRIPT_PATH.parent))
_spec = importlib.util.spec_from_file_location("audit_qt_usage", _SCRIPT_PATH)
assert _spec is not None and _spec.loader is not None
audit = importlib.util.module_from_spec(_spec)
sys.modules["audit_qt_usage"] = audit
_spec.loader.exec_module(audit)

POLICY = {"scan_root": "src/libs/mne", "qt_by_design": [], "kept": {}, "ceilings": {}}


def make_repo(root: Path, files: dict[str, str]) -> None:
    for relative, text in files.items():
        path = root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")


class CountTests(unittest.TestCase):
    def test_counts_qt_identifiers_by_category_only_in_code(self) -> None:
        counts = audit.count_tokens(
            '#include <QString>\n'
            '// QList in a comment\n'
            'QString name = "QVector in a string";\n'
            'QSharedPointer<QFile> file; qDebug() << name; qint64 n = 0; emit changed();\n'
            'dispatch_qos(QOS_CLASS_DEFAULT);\n')
        self.assertEqual(counts, {"strings": 1, "smart_pointers": 1, "files": 1, "logging": 1, "global": 1,
                                  "objects_signals": 1})

    def test_nested_libraries_and_public_headers_are_separate(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            make_repo(root, {
                "src/libs/mne/acq/CMakeLists.txt": "add_library(mne_acq a.cpp)\n",
                "src/libs/mne/acq/a.h": "int a();\n",
                "src/libs/mne/acq/fiff/CMakeLists.txt":
                    "set(QT_REQUIRED_COMPONENTS Core)\nadd_library(mne_acq_fiff b.cpp)\n",
                "src/libs/mne/acq/fiff/b.h": "#include <QString>\nQString b();\n",
                "src/libs/mne/acq/fiff/detail/c.h": "#include <QList>\nQList<int> c();\n",
            })
            report = audit.build_report(root, POLICY)
        self.assertEqual(report["summary"]["qt_free"], ["acq"])
        fiff = report["libraries"]["acq/fiff"]
        self.assertEqual((fiff["tokens"], fiff["public_header_includes"], fiff["modules"]), (2, 1, ["Core"]))


class CeilingTests(unittest.TestCase):
    def test_qt_free_library_fails_on_a_single_qstring(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            make_repo(root, {"src/libs/mne/acq/CMakeLists.txt": "add_library(mne_acq a.cpp)\n",
                             "src/libs/mne/acq/a.cpp": "int a() { return 0; }\n"})
            policy = copy.deepcopy(POLICY)
            policy["ceilings"] = audit.current_ceilings(audit.build_report(root, policy))
            self.assertEqual(audit.check_ceilings(audit.build_report(root, policy), policy), [])

            (root / "src/libs/mne/acq/a.cpp").write_text("QString a() { return {}; }\n", encoding="utf-8")
            self.assertEqual(audit.check_ceilings(audit.build_report(root, policy), policy),
                             ["acq: tokens 1 > ceiling 0"])

    def test_a_category_cannot_grow_while_the_total_stays(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            make_repo(root, {"src/libs/mne/disp/CMakeLists.txt": "add_library(mne_disp a.cpp)\n",
                             "src/libs/mne/disp/a.cpp": "QString a; QString b;\n"})
            policy = copy.deepcopy(POLICY)
            policy["ceilings"] = audit.current_ceilings(audit.build_report(root, policy))
            self.assertEqual(policy["ceilings"]["disp"]["by_category"], {"strings": 2})

            (root / "src/libs/mne/disp/a.cpp").write_text("QString a; QVector<int> b;\n", encoding="utf-8")
            self.assertEqual(audit.check_ceilings(audit.build_report(root, policy), policy),
                             ["disp: containers 1 > ceiling 0"])

    def test_new_module_and_new_library_fail(self) -> None:
        report = {"libraries": {"a": {"tokens": 1, "public_header_includes": 0, "modules": ["Core", "Gui"]},
                                "b": {"tokens": 0, "public_header_includes": 0, "modules": []}}}
        policy = {"ceilings": {"a": {"tokens": 1, "public_header_includes": 0, "modules": ["Core"]}}}
        self.assertEqual(audit.check_ceilings(report, policy),
                         ["a: links Qt Gui, not allowed by its ceiling", "b: no ceiling"])


class RepositoryTests(unittest.TestCase):
    def test_repository_is_within_ceilings_and_ceilings_are_tight(self) -> None:
        policy = json.loads(audit.POLICY_FILE.read_text(encoding="utf-8"))
        report = audit.build_report()
        self.assertEqual(report, audit.build_report())
        self.assertEqual(audit.check_ceilings(report, policy), [])
        # A ceiling left above the current count would let removed Qt creep back unnoticed.
        self.assertEqual(policy["ceilings"], audit.current_ceilings(report))


if __name__ == "__main__":
    unittest.main()
