#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     September, 2026
#
#
# @brief    Tests for the maintainability baseline audit.
"""Tests for ``scripts/quality/audit_maintainability.py``."""

from __future__ import annotations

import copy
import json
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
        self.assertEqual(counted("QScopedArrayPointer<char> d(new char[n]);"), {})

    def test_qt_new_passed_into_an_ownership_taking_call_is_owned(self) -> None:
        for snippet in (
            "layout->addWidget(new QLabel(tr(\"Name\")));",
            "layout->addWidget(new QLabel(tr(\"Name\")), 0, 1);",
            "layout->insertWidget(0, new QPushButton(icon, text));",
            "layout->addLayout(new QHBoxLayout());",
            "layout->insertLayout(1, new QHBoxLayout);",
            "widget->setLayout(new QVBoxLayout);",
            "layout->addItem(new QSpacerItem(20, 40));",
            "scene.addItem(new QGraphicsRectItem(rect));",
            "layout->insertItem(2, new QSpacerItem(1, 1));",
            "form->addRow(tr(\"Gain\"), new QSpinBox);",
            "scroll->setWidget(new QWidget());",
            "window.setCentralWidget(new QTextEdit);",
            "setMenuBar(new QMenuBar);",
            "setStatusBar(new QStatusBar());",
            "addDockWidget(Qt::LeftDockWidgetArea, new QDockWidget(title));",
            "addToolBar(new QToolBar(tr(\"Tools\")));",
            "tabs->addTab(new QWidget, tr(\"Info\"));",
            "tabs->insertTab(0, new QWidget(), label);",
            "wizard.addPage(new QWizardPage);",
            "model->setItem(row, 0, new QStandardItem(text));",
            "table->setItem(r, c, new QTableWidgetItem(QString::number(v)));",
            "parentItem->setChild(0, 1, new QStandardItem(value));",
            "model->appendRow(new QStandardItem(name));",
            "model->insertRow(0, new QStandardItem(name));",
            "model->appendColumn({new QStandardItem(name)});",
            "model->setHorizontalHeaderItem(0, new QStandardItem(tr(\"A\")));",
            "table->setVerticalHeaderItem(0, new QTableWidgetItem(tr(\"B\")));",
            "tree->addTopLevelItem(new QTreeWidgetItem(columns));",
            "tree->insertTopLevelItem(0, new QTreeWidgetItem(columns));",
            "item->addChild(new QTreeWidgetItem(columns));",
            "view->setViewport(new QOpenGLWidget);",
            "area->setCornerWidget(new QLabel(text));",
            "action->setDefaultWidget(new QSlider(Qt::Horizontal));",
            "menu->addAction(new QAction(tr(\"Open\")));",
            "button->setMenu(new QMenu(tr(\"More\")));",
            "box->addWidget(new QLabel);",
        ):
            self.assertEqual(counted(snippet), {}, snippet)

    def test_qt_new_with_a_parent_as_last_constructor_argument_is_owned(self) -> None:
        for snippet in (
            "auto* label = new QLabel(text, pGroupBox);",
            "auto* menu = new QMenu(tr(\"Move\"), m_pMenu);",
            "auto* layout = new QVBoxLayout(pWidget);",
            "auto* box = new QGroupBox(name, m_pUi->m_groupBox_detectedTrials);",
            "auto* action = new QAction(QString::number(i), pData->pMenu);",
            "auto* item = new QTreeWidgetItem(parentItem);",
            "m_pView = new QDialog(this, Qt::Window);",
            "auto* label = new QLabel(m_pAboutWindow.data());",
            "auto* layout = new QVBoxLayout(newDisp.get());",
            "auto* view = new QLabel(text, m_pLayout->parentWidget());",
            "m_pRoot = new QTreeWidgetItem(m_pTree.data(), {name});",
            "auto* line = new QFrame(m_tableView->viewport());",
            "auto* item = new QTreeWidgetItem(pRoot,",
        ):
            self.assertEqual(counted(snippet), {}, snippet)
        for snippet in (
            "auto* label = new QLabel(text);",
            "auto* label = new QLabel(pName->text());",
            "auto* item = new QStandardItem(name,",
            "auto* view = new QWidget(0, Qt::Window);",
        ):
            self.assertEqual(counted(snippet), {"qt_new_unparented": 1}, snippet)

    def test_qt_new_with_a_parent_declared_in_the_file_is_owned(self) -> None:
        code = ("QWidget* container = makeContainer();\n"
                "auto* row = new QWidget(container);\n"
                "auto* edit = new QLineEdit(row);\n"
                "QDialog dlg(this);\n"
                "auto* box = new QComboBox(&dlg);\n"
                "auto* view = new QDialog(row, Qt::Window);\n"
                "QString text;\n"
                "auto* label = new QLabel(text);\n"
                "auto* other = new QLabel(&text);\n")
        self.assertEqual(counted(code), {"qt_new_unparented": 1})

    def test_members_declared_in_the_own_header_name_parents(self) -> None:
        source = "auto* layout = new QVBoxLayout(m_panel);\n"
        self.assertEqual({k: v for k, v in audit.count_file(source).items() if v}, {"qt_new_unparented": 1})
        self.assertEqual({k: v for k, v in audit.count_file(source, header="QWidget* m_panel;").items() if v}, {})

    def test_qt_new_whose_parent_follows_on_a_later_line_is_owned(self) -> None:
        self.assertEqual(counted("auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok,\n"
                                 "                                     Qt::Horizontal,\n"
                                 "                                     this);\n"), {})
        self.assertEqual(counted("auto* label = new QLabel(text,\n"
                                 "                         font);\n"
                                 "foo(this);\n"), {"qt_new_unparented": 1})

    def test_qt_new_into_a_call_that_does_not_take_ownership_still_counts(self) -> None:
        for snippet in (
            "view->setModel(new QStandardItemModel);",
            ", m_view(new QTableView)",
            "view->setItemDelegate(new QStyledItemDelegate);",
            "layout->addWidget(wrap(new QLabel(text)));",
            "menu->addAction(new QIcon(path), text);",
            "auto* label = new QLabel(text); layout->addWidget(label);",
        ):
            self.assertEqual(counted(snippet), {"qt_new_unparented": 1}, snippet)

    def test_project_classes_derived_from_qt_count_as_qt_allocations(self) -> None:
        headers = ("class DISPSHARED_EXPORT View : public QWidget, private Ui::Form\n{",
                   "class ScaleView final : public View {",
                   "class Item : public QGraphicsItem, public Base {",
                   "class Plain : public Base {",
                   "class Data : public QSharedData {")
        classes = audit.qt_derived_classes(headers)
        self.assertEqual(classes, {"View", "ScaleView", "Item"})
        code = "auto* a = new mne::disp::ScaleView(label); auto* b = new Item(x); auto* c = new Plain();"
        self.assertEqual({k: v for k, v in audit.count_file(code, classes).items() if v},
                         {"qt_new_unparented": 2, "raw_new": 1})
        self.assertEqual({k: v for k, v in audit.count_file(", ui(new Ui::View)", classes).items() if v}, {})

    def test_comments_and_strings_do_not_count(self) -> None:
        for snippet in ('// new Foo; NULL (int)x', '/* delete p; */', 'qInfo("NULL printf(")', "int n = 1'000;"):
            self.assertEqual(counted(snippet), {}, snippet)

    def test_raw_string_literals_keep_line_structure(self) -> None:
        code = 'auto s = R"json(\n{"a": "NULL", "b": 1}\n)json";\nint y = (int)x;\n'
        stripped = audit.strip_comments_and_strings(code)
        self.assertEqual(stripped.count("\n"), code.count("\n"))
        self.assertEqual(stripped.splitlines()[3], "int y = (int)x;")
        self.assertEqual(counted(code), {"c_style_cast": 1})

    def test_legacy_constructs_are_counted(self) -> None:
        self.assertEqual(counted("int y = (int)x;"), {"c_style_cast": 1})
        self.assertEqual(counted("f(a, (const double *)b);"), {"c_style_cast": 1})
        self.assertEqual(counted("sizeof(int) * 2; foo(int) ;"), {})
        self.assertEqual(counted("if (h == NULL)"), {"null_macro": 1})
        self.assertEqual(counted("typedef struct { int a; } rec;"), {"typedef_struct": 1})
        self.assertEqual(counted("QObject* o = Q_NULLPTR;"), {"null_macro": 1})
        self.assertEqual(counted("w->deleteLater(); deleteEvent(id); m.delete_all(); delete p; delete[] q;"),
                         {"raw_delete": 2})
        self.assertEqual(counted("typedef enum { A } E;"), {"typedef_struct": 1})
        self.assertEqual(counted("return std::unique_ptr<Cov>(new Cov(1));"), {})
        self.assertEqual(counted("p = FiffCov::SDPtr(new FiffCov());"), {})
        self.assertEqual(counted("std::shared_ptr<A> a(new A);"), {})
        self.assertEqual(counted("auto x = new Foo();"), {"raw_new": 1})
        self.assertEqual(counted("delete[] buf; delete p;"), {"raw_delete": 2})
        self.assertEqual(counted('fprintf(stderr, "x"); std::cout << 1;'), {"console_io": 2})
        self.assertEqual(counted("cout << 1; cerr << 2; clog << 3;"), {"console_io": 3})
        self.assertEqual(counted("out << x; m_cout << y; s.cout << z;"), {})
        self.assertEqual(counted("m.printf();"), {})
        self.assertEqual(counted("#define FIFF_FILE_ID 100\n#define STR(x) #x\n"), {"numeric_define": 1})


class RatchetTests(unittest.TestCase):
    def test_growth_in_total_or_single_file_fails(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "src/libs/mne/a").mkdir(parents=True)
            (root / "src/libs/mne/a/a.cpp").write_text("int* p = NULL;\n", encoding="utf-8")
            (root / "src/libs/mne/a/b.cpp").write_text("int* q = NULL;\n", encoding="utf-8")
            policy = {"scan_roots": ["src/libs/mne"], "extensions": [".cpp"]}
            baseline = audit.build_report(root, policy)
            self.assertEqual(baseline["metrics"]["null_macro"]["total"], 2)
            self.assertEqual(audit.check(baseline, baseline), [])

            (root / "src/libs/mne/a/a.cpp").write_text("int* p = NULL; int* r = NULL;\n", encoding="utf-8")
            (root / "src/libs/mne/a/b.cpp").write_text("int* q = nullptr;\n", encoding="utf-8")
            moved = audit.build_report(root, policy)
            self.assertEqual(moved["metrics"]["null_macro"]["total"], 2)
            failures = audit.check(moved, baseline)
            self.assertTrue(any("a/a.cpp has 2, baseline 1" in f for f in failures), failures)

    def test_suppression_hides_one_file_from_one_metric(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "src/tools/t").mkdir(parents=True)
            (root / "src/tools/t/main.cpp").write_text('fprintf(stderr, "x"); int* p = NULL;\n', encoding="utf-8")
            policy = {"scan_roots": ["src/tools"], "extensions": [".cpp"],
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

    def test_repository_is_within_ceilings_and_ceilings_are_tight(self) -> None:
        policy = json.loads(audit.POLICY_FILE.read_text(encoding="utf-8"))
        report = audit.build_report()
        self.assertEqual(audit.check_ceilings(report, policy), [])
        # A ceiling left above the current count would let retired debt creep back unnoticed.
        loose = {name: (limit, report["metrics"][name]["total"]) for name, limit in policy["ceilings"].items()
                 if not name.startswith("$") and limit > report["metrics"][name]["total"]}
        self.assertEqual(loose, {})
        grown = copy.deepcopy(report)
        grown["metrics"]["console_io"]["total"] = 1
        self.assertEqual(audit.check_ceilings(grown, policy), ["console_io: 1 > ceiling 0"])


if __name__ == "__main__":
    unittest.main()
