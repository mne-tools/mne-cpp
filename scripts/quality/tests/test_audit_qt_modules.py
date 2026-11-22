#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# @since    3.0.0
# @date     November, 2026
#
#
# @brief    Tests for the per-module Qt audit of the display stack and applications.
"""Tests for ``scripts/quality/audit_qt_modules.py``."""

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

_SCRIPT_PATH = Path(__file__).resolve().parents[1] / "audit_qt_modules.py"
sys.path.insert(0, str(_SCRIPT_PATH.parent))
_spec = importlib.util.spec_from_file_location("audit_qt_modules", _SCRIPT_PATH)
assert _spec is not None and _spec.loader is not None
audit = importlib.util.module_from_spec(_spec)
sys.modules["audit_qt_modules"] = audit
_spec.loader.exec_module(audit)

SCOPES = {"disp": "src/libraries/disp", "applications": "src/applications"}


def make_repo(root: Path, files: dict[str, str]) -> None:
    for relative, text in files.items():
        path = root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")


class MapTests(unittest.TestCase):
    def setUp(self) -> None:
        self.qt = audit.build_qt_map(None)

    def test_fallback_is_used_without_qt(self) -> None:
        self.assertEqual(self.qt.source, "fallback")
        self.assertEqual(self.qt.resolve_identifier("QWidget"), ("Widgets", "public"))
        self.assertEqual(self.qt.resolve_identifier("QRhiBuffer"), ("GuiPrivate", "rhi"))
        self.assertIsNone(self.qt.resolve_identifier("QMyOwnThing"))

    def test_include_forms(self) -> None:
        resolve = self.qt.resolve_include
        self.assertEqual(resolve("QtWidgets/QWidget"), ("Widgets", "public"))
        self.assertEqual(resolve("QtConcurrent"), ("Concurrent", "public"))
        self.assertEqual(resolve("qsvggenerator.h"), ("Svg", "public"))
        self.assertEqual(resolve("rhi/qrhi.h"), ("GuiPrivate", "rhi"))
        self.assertEqual(resolve("private/qrhi_p.h"), ("GuiPrivate", "private"))
        self.assertEqual(resolve("QtGui/private/qrhi_p.h"), ("GuiPrivate", "private"))
        self.assertIsNone(resolve("disp/viewers/view.h"))

    def test_analysis_ignores_comments_and_strings(self) -> None:
        result = audit.analyze_source(
            '#include <QtConcurrent/QtConcurrent>\n'
            '// QTcpSocket in a comment\n'
            'auto f = QtConcurrent::run([] { return QString("QSvgGenerator"); });\n'
            'QFutureWatcher<void> w;\n', self.qt)
        self.assertEqual(result["modules"]["Concurrent"], {"<QtConcurrent/QtConcurrent>": 1, "QtConcurrent": 1})
        self.assertEqual(result["modules"]["Core"], {"QFutureWatcher": 1, "QString": 1})
        self.assertNotIn("Network", result["modules"])
        self.assertNotIn("Svg", result["modules"])


class RuleTests(unittest.TestCase):
    def test_verdicts_come_from_the_rules_table(self) -> None:
        self.assertEqual(audit.rule_for("Concurrent")["verdict"], "std")
        self.assertEqual(audit.rule_for("Network", "QTcpSocket")["verdict"], "own")
        self.assertIn("HTTP", audit.rule_for("Network", "<QNetworkAccessManager>")["replacement"])
        self.assertIn("PNG", audit.rule_for("Svg", "QSvgRenderer")["replacement"])
        self.assertEqual(audit.rule_for("PrintSupport")["verdict"], "own/gui")
        self.assertEqual(audit.rule_for("GuiPrivate")["verdict"], "red flag")
        self.assertEqual(audit.rule_for("Bluetooth")["verdict"], "review")


class ScanTests(unittest.TestCase):
    def scan(self, files: dict[str, str]) -> dict:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            make_repo(root, files)
            return audit.scan(root, audit.build_qt_map(None), SCOPES)

    def test_renders_vs_chrome(self) -> None:
        report = self.scan({
            "src/libraries/disp/CMakeLists.txt": "set(QT_REQUIRED_COMPONENTS Core Widgets)\nadd_library(d a.cpp)\n",
            "src/libraries/disp/plot.h": "#include <QWidget>\nclass DISPSHARED_EXPORT Plot : public QWidget {\n"
                                         "protected:\n void paintEvent(QPaintEvent* e) override;\n};\n",
            "src/libraries/disp/subplot.h": "class SubPlot : public Plot { };\n",
            "src/libraries/disp/graph.h": "#include <QWidget>\nclass Graph : public QWidget { void draw(); };\n",
            "src/libraries/disp/graph.cpp": "void Graph::draw() { QPainter painter(this); }\n",
            "src/libraries/disp/rhiview.h": "class RhiView : public QRhiWidget { };\n",
            "src/libraries/disp/settings.h": "#include <QDialog>\nclass Settings : public QDialog {\n"
                                             " QVBoxLayout* m_layout; QPushButton* m_ok; };\n",
            "src/libraries/disp/settings.ui": "<ui/>\n",
        })
        widgets = report["scopes"]["disp"]["widgets"]
        render = {c["class"]: c["reasons"] for c in widgets["render_classes"]}
        self.assertEqual(set(render), {"Plot", "SubPlot", "Graph", "RhiView"})
        self.assertIn("overrides paintEvent", render["Plot"])
        self.assertIn("derives rendering Plot", render["SubPlot"])
        self.assertIn("QPainter on this/viewport()", render["Graph"])
        self.assertIn("derives QRhiWidget", render["RhiView"])
        self.assertEqual([c["class"] for c in widgets["chrome_classes"]], ["Settings"])
        self.assertIn("src/libraries/disp/settings.h", widgets["chrome_files"])
        self.assertIn("src/libraries/disp/plot.h", widgets["renders_files"])
        self.assertEqual(report["units"]["src/libraries/disp"]["ui_forms"], 1)

    def test_cmake_requested_vs_used_per_target(self) -> None:
        report = self.scan({
            "src/applications/viewer/CMakeLists.txt":
                "set(QT_REQUIRED_COMPONENTS Core Widgets Svg)  # Svg was for export\n"
                "if(NOT WASM)\n  list(APPEND QT_REQUIRED_COMPONENTS PrintSupport)\nendif()\n"
                "find_package(Qt6 REQUIRED COMPONENTS ShaderTools)\n"
                "qt_add_executable(viewer main.cpp)\nqt_add_shaders(viewer \"s\" FILES a.frag)\n"
                "target_link_libraries(viewer PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)\n",
            "src/applications/viewer/main.cpp":
                "#include <QApplication>\nint main() { QApplication a; QPainter p; QtConcurrent::run([]{}); }\n",
            "src/applications/viewer/plugins/net/CMakeLists.txt":
                "find_package(Qt6 REQUIRED COMPONENTS Core Network)\nadd_library(net net.cpp)\n",
            "src/applications/viewer/plugins/net/net.cpp": "#include <QTcpSocket>\nQTcpSocket s;\n",
        })
        viewer = report["units"]["src/applications/viewer"]
        self.assertEqual(viewer["requested"], ["Core", "PrintSupport", "ShaderTools", "Svg", "Widgets"])
        self.assertEqual(viewer["requested_unused"], ["Core", "PrintSupport", "Svg"])
        self.assertEqual(viewer["build_time_only"], ["ShaderTools"])
        self.assertEqual(viewer["implied"], ["Gui"])
        self.assertEqual(viewer["transitive"], ["Concurrent"])
        net = report["units"]["src/applications/viewer/plugins/net"]
        self.assertEqual(net["used"], ["Network"])
        self.assertEqual(net["requested_unused"], ["Core"])
        modules = report["scopes"]["applications"]["modules"]
        self.assertEqual(modules["Network"]["verdict"], "own")
        self.assertEqual(list(modules["Network"]["files"]), ["src/applications/viewer/plugins/net/net.cpp"])
        self.assertIn("## Requested but unused", audit.render_markdown(report))

    def test_gui_private_findings(self) -> None:
        report = self.scan({
            "src/libraries/disp/CMakeLists.txt": "add_library(d a.cpp)\ntarget_link_libraries(d Qt6::GuiPrivate)\n",
            "src/libraries/disp/a.cpp": "#include <rhi/qrhi.h>\n#include <private/qrhi_p.h>\nQRhiBuffer* b;\n",
        })
        kinds = {(f["header"], f["kind"]) for f in report["gui_private"]}
        self.assertEqual(kinds, {("rhi/qrhi.h", "rhi"), ("private/qrhi_p.h", "private"), (None, "rhi")})
        self.assertEqual(report["units"]["src/libraries/disp"]["requested_unused"], [])


if __name__ == "__main__":
    unittest.main()
