#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# @since    3.0.0
# @date     November, 2026
#
#
# @brief    Attribute Qt use of the display stack and applications to Qt modules, per file and class.
"""Qt modules used by the display stack and the applications.

Plans shrinking the Qt dependency to Qt Core + Gui. For each scope (``src/libs/mne/disp``,
``src/libs/mne/disp3D``, ``src/applications``) and each CMake target inside it, it reports:

* which Qt module every ``#include`` and every Q-identifier in code belongs to (comments and string literals are
  removed); the class -> module map is built from the Qt headers in ``src/external/qt/dynamic`` (forwarding headers
  such as ``QWidget``, ``Headers/<version>/Qt<X>/private`` for ``<X>Private`` and ``rhi/`` for the semi-public QRhi
  API), with a small built-in map when Qt is missing
* a verdict and a replacement for every module outside Core/Gui, from the data-driven ``RULES`` table
* the Qt components each CMakeLists.txt requests, the ones requested but unused, and the ones used but only reachable
  transitively (through a linked library)
* Widgets split into ``renders`` (widgets that draw data: paint/GL/RHI overrides, ``QPainter(this)``, graphics view,
  ``QOpenGLWidget``/``QRhiWidget``) and ``chrome`` (dialogs, forms, layouts, menus); the split is a heuristic

Usage
-----
    python3 scripts/quality/audit_qt_modules.py                      # Markdown to stdout
    python3 scripts/quality/audit_qt_modules.py --json out.json --markdown out.md
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))
from audit_maintainability import strip_comments_and_strings  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parents[2]
POLICY_FILE = Path(__file__).resolve().parent / "qt_usage_policy.json"
QT_ROOT = REPO_ROOT / "src" / "external" / "qt" / "dynamic"
SCOPES = {"disp": "src/libs/mne/disp", "disp3D": "src/libs/mne/disp3D", "applications": "src/applications"}
EXTENSIONS = {".h", ".hpp", ".cpp", ".cc", ".cxx", ".mm"}
BASE_MODULES = {"Core", "Gui"}

# Preferred module when a header name exists in several Qt modules.
PRIORITY = ["Core", "Gui", "Widgets", "Concurrent", "Network", "Svg", "SvgWidgets", "PrintSupport", "OpenGL",
            "OpenGLWidgets", "Test", "Xml", "Sql", "SerialPort"]

# Module -> modules it links publicly (to tell "implied" from "transitive").
MODULE_DEPENDS = {
    "Gui": ["Core"], "Widgets": ["Gui"], "Concurrent": ["Core"], "Network": ["Core"], "Svg": ["Gui"],
    "SvgWidgets": ["Svg", "Widgets"], "PrintSupport": ["Widgets"], "OpenGL": ["Gui"],
    "OpenGLWidgets": ["OpenGL", "Widgets"], "GuiPrivate": ["Gui", "CorePrivate"], "CorePrivate": ["Core"],
    "WidgetsPrivate": ["Widgets", "GuiPrivate"], "Test": ["Core"], "Xml": ["Core"], "Sql": ["Core"],
    "SerialPort": ["Core"], "ShaderTools": ["Gui"], "Charts": ["Widgets"], "QuickWidgets": ["Quick", "Widgets"],
}

# Used when the Qt headers are not available (and by the unit tests).
FALLBACK_CLASSES: dict[str, list[str]] = {
    "Core": ["QObject", "QString", "QStringList", "QList", "QVector", "QMap", "QHash", "QSet", "QPair", "QFile",
             "QDir", "QFileInfo", "QThread", "QMutex", "QMutexLocker", "QTimer", "QElapsedTimer", "QFuture",
             "QFutureWatcher", "QPromise", "QDebug", "QVariant", "QByteArray", "QPoint", "QPointF", "QRect",
             "QRectF", "QSize", "QSizeF", "QSharedPointer", "QPointer", "QSettings", "QDateTime", "QEvent",
             "QCoreApplication", "QAbstractItemModel", "QAbstractTableModel", "QModelIndex", "QJsonDocument",
             "QJsonObject", "QRegularExpression", "QXmlStreamReader", "QXmlStreamWriter", "QtCore", "QtGlobal"],
    "Gui": ["QPainter", "QPainterPath", "QImage", "QPixmap", "QColor", "QPen", "QBrush", "QFont", "QFontMetrics",
            "QWindow", "QRasterWindow", "QOpenGLWindow", "QPdfWriter", "QIcon", "QVector2D", "QVector3D",
            "QVector4D", "QMatrix4x4", "QQuaternion", "QOpenGLFunctions", "QOpenGLContext", "QSurfaceFormat",
            "QPaintEvent", "QResizeEvent", "QMouseEvent", "QWheelEvent", "QKeyEvent", "QGuiApplication", "QAction",
            "QShortcut", "QStandardItemModel", "QStandardItem", "QBackingStore", "QTransform", "QPolygonF",
            "QtGui"],
    "Widgets": ["QWidget", "QDialog", "QMainWindow", "QDockWidget", "QPushButton", "QToolButton", "QLabel",
                "QVBoxLayout", "QHBoxLayout", "QGridLayout", "QFormLayout", "QComboBox", "QSpinBox",
                "QDoubleSpinBox", "QCheckBox", "QRadioButton", "QMenu", "QMenuBar", "QToolBar", "QStatusBar",
                "QAbstractScrollArea", "QScrollArea", "QGraphicsView", "QGraphicsScene", "QGraphicsItem",
                "QGraphicsObject", "QGraphicsRectItem", "QRhiWidget", "QApplication", "QTableView", "QTreeView",
                "QListView", "QTableWidget", "QStyledItemDelegate", "QAbstractItemDelegate", "QFrame", "QSplitter",
                "QTabWidget", "QMessageBox", "QFileDialog", "QSlider", "QLineEdit", "QGroupBox", "QProgressBar",
                "QTextEdit", "QtWidgets"],
    "OpenGL": ["QOpenGLShaderProgram", "QOpenGLBuffer", "QOpenGLVertexArrayObject", "QOpenGLTexture", "QtOpenGL"],
    "OpenGLWidgets": ["QOpenGLWidget", "QtOpenGLWidgets"],
    "Concurrent": ["QtConcurrent", "QtConcurrentRun", "QtConcurrentMap"],
    "Network": ["QTcpSocket", "QTcpServer", "QUdpSocket", "QHostAddress", "QAbstractSocket", "QNetworkAccessManager",
                "QNetworkRequest", "QNetworkReply", "QLocalSocket", "QLocalServer", "QtNetwork"],
    "Svg": ["QSvgGenerator", "QSvgRenderer", "QtSvg"],
    "SvgWidgets": ["QSvgWidget", "QGraphicsSvgItem", "QtSvgWidgets"],
    "PrintSupport": ["QPrinter", "QPrintDialog", "QPrintPreviewDialog", "QPrintPreviewWidget", "QtPrintSupport"],
    "Test": ["QTest", "QSignalSpy", "QtTest"],
    "Charts": ["QChart", "QChartView", "QLineSeries", "QValueAxis", "QBarSeries", "QtCharts"],
    "Xml": ["QDomDocument", "QDomElement", "QDomNode", "QtXml"],
    "SerialPort": ["QSerialPort", "QSerialPortInfo", "QtSerialPort"],
}
FALLBACK_RHI_HEADERS = {"qrhi.h": ["QRhi", "QRhiBuffer", "QRhiTexture", "QRhiSampler", "QRhiRenderBuffer",
                                   "QRhiShaderResourceBindings", "QRhiShaderResourceBinding",
                                   "QRhiGraphicsPipeline", "QRhiComputePipeline", "QRhiCommandBuffer",
                                   "QRhiRenderTarget", "QRhiTextureRenderTarget", "QRhiRenderPassDescriptor",
                                   "QRhiResourceUpdateBatch", "QRhiVertexInputLayout", "QRhiViewport",
                                   "QRhiScissor", "QRhiColorAttachment", "QRhiSwapChain", "QRhiReadbackResult"],
                        "qshader.h": ["QShader", "QShaderVersion", "QShaderKey", "QShaderCode"],
                        "qshaderdescription.h": ["QShaderDescription"], "qrhi_platform.h": []}

# Verdicts and replacements per module (and, where it differs, per class). Data, not code: extend here.
RULES: dict[str, dict[str, Any]] = {
    "Concurrent": {
        "verdict": "std",
        "replacement": "mne::ThreadPool (src/libs/mne/base/thread_pool.h), std::async / std::jthread "
                       "(QFuture/QFutureWatcher are QtCore, counted there)",
        "classes": {},
    },
    "Network": {
        "verdict": "own",
        "replacement": "com library sockets (src/libs/mne/com: tcp_socket, tcp_server, udp_socket)",
        "classes": {"QNetworkAccessManager": "no HTTP client in com; needs an own one or a third-party library",
                    "QNetworkRequest": "no HTTP client in com; needs an own one or a third-party library",
                    "QNetworkReply": "no HTTP client in com; needs an own one or a third-party library",
                    "QLocalSocket": "com has no local (Unix socket / named pipe) socket; TCP loopback via com "
                                    "tcp_socket/tcp_server, or add a local socket to com",
                    "QLocalServer": "com has no local (Unix socket / named pipe) server; TCP loopback via com "
                                    "tcp_server, or add a local server to com"},
    },
    "SerialPort": {"verdict": "own", "replacement": "com library serial_port (src/libs/mne/com/serial_port.h)",
                   "classes": {}},
    "Svg": {
        "verdict": "own/gui",
        "replacement": "own SVG writer, or QPainter on QImage / QPdfWriter (Gui)",
        "classes": {"QSvgRenderer": "icons: ship PNGs and use QIcon/QPixmap (Gui); data: own SVG reader",
                    "QSvgGenerator": "own SVG writer (QPainter-free), or QPdfWriter/QImage export (Gui)"},
    },
    "SvgWidgets": {"verdict": "own/gui", "replacement": "QIcon/QPixmap from PNG (Gui) instead of QSvgWidget",
                   "classes": {}},
    "PrintSupport": {"verdict": "own/gui", "replacement": "QPdfWriter (Gui) for PDF export; drop print dialogs",
                     "classes": {}},
    "GuiPrivate": {
        "verdict": "red flag",
        "replacement": "semi-public QRhi API <rhi/qrhi.h> (Qt >= 6.6, symbols exported from QtGui); true *_p.h "
                       "headers have no compatibility promise and must go",
        "classes": {},
    },
    "CorePrivate": {"verdict": "red flag", "replacement": "public QtCore API", "classes": {}},
    "WidgetsPrivate": {"verdict": "red flag", "replacement": "public API", "classes": {}},
    "ShaderTools": {"verdict": "build-time",
                    "replacement": "only needed by qt_add_shaders at build time (qsb); keep as a build tool or "
                                   "precompile .qsb files and drop the component",
                    "classes": {}},
    "OpenGL": {"verdict": "gui", "replacement": "QRhi (Gui) on a QWindow", "classes": {}},
    "OpenGLWidgets": {"verdict": "gui", "replacement": "QWindow + QRhi (Gui), or QRhiWidget while Widgets stays",
                      "classes": {}},
    "Charts": {"verdict": "own/gui", "replacement": "own QPainter plot (Gui)", "classes": {}},
    "Test": {"verdict": "test-only", "replacement": "keep in test targets only", "classes": {}},
    "Xml": {"verdict": "core", "replacement": "QXmlStreamReader/Writer (Core)", "classes": {}},
    "Widgets": {
        "verdict": "split",
        "replacement": "renders: QWindow + QRhi or QPainter on QRasterWindow/QBackingStore (Gui); "
                       "chrome: application UI, the one module worth justifying",
        "classes": {},
    },
}
DEFAULT_RULE = {"verdict": "review", "replacement": "no rule yet; add one to RULES", "classes": {}}

RENDER_BASES = {"QOpenGLWidget", "QRhiWidget", "QGraphicsView", "QGraphicsScene", "QGraphicsItem",
                "QGraphicsObject", "QGraphicsWidget", "QQuickWidget", "QChartView", "QOpenGLWindow"}
WINDOW_BASES = {"QWindow", "QRasterWindow", "QPaintDeviceWindow", "QOpenGLWindow"}
WIDGET_MODULES = {"Widgets", "OpenGLWidgets", "SvgWidgets", "QuickWidgets", "MultimediaWidgets", "Charts",
                  "PrintSupport"}
RENDER_OVERRIDES = ("paintEvent", "paintGL", "initializeGL", "resizeGL", "render", "initialize",
                    "releaseResources", "paint", "drawBackground", "drawForeground")

_IDENT = re.compile(r"\bQt?[A-Z]\w*")
_INCLUDE = re.compile(r"^\s*#\s*include\b")
_INCLUDE_PATH = re.compile(r"#\s*include\s*[<\"]([^>\"]+)[>\"]")
_TARGET = re.compile(r"\b(?:add_library|add_executable|qt_add_executable|qt_add_library|qt_add_plugin)\s*\(")
_CLASS = re.compile(r"\b(?:class|struct)\s+(?:[A-Z][A-Z0-9_]*\s+)*(\w+)\s*(?:final\s*)?:\s*([^{;()]+)\{")
_PAINTER_THIS = re.compile(r"\bQPainter\s+\w+\s*[({]\s*(?:this|viewport\s*\(\s*\))\s*[)}]"
                           r"|\.begin\s*\(\s*(?:this|viewport\s*\(\s*\))\s*\)")
_METHOD_DEF = re.compile(r"\b(\w+)::~?\w+\s*\(")


# ---------------------------------------------------------------------------------------------------------------------
# Qt class/header -> module map
# ---------------------------------------------------------------------------------------------------------------------

class QtMap:
    """Header and class names to Qt modules; ``rhi`` holds names of the semi-public QRhi API."""

    def __init__(self) -> None:
        self.classes: dict[str, str] = {}
        self.headers: dict[str, str] = {}
        self.private_headers: dict[str, str] = {}
        self.rhi_headers: set[str] = set()
        self.rhi_classes: set[str] = set()
        self.modules: set[str] = set()
        self.source = "fallback"

    def _set(self, table: dict[str, str], name: str, module: str) -> None:
        current = table.get(name)
        if current is None or _rank(module) < _rank(current):
            table[name] = module

    def resolve_include(self, path: str) -> tuple[str, str] | None:
        """``(module, kind)`` for an include path, kind is ``public``, ``rhi`` or ``private``; None if not Qt."""
        parts = path.split("/")
        if len(parts) >= 2 and parts[0].startswith("Qt") and parts[0][2:] in self.modules:
            module, rest = parts[0][2:], parts[1:]
            if rest[0] == "rhi":
                return "GuiPrivate", "rhi"
            if rest[0] in ("private", "qpa"):
                return f"{module}Private", "private"
            return module, "public"
        if parts[0] == "rhi" and len(parts) == 2:
            return ("GuiPrivate", "rhi") if parts[1] in self.rhi_headers or not self.rhi_headers else None
        if parts[0] in ("private", "qpa") and len(parts) == 2:
            return self.private_headers.get(parts[1], "UnknownPrivate"), "private"
        if len(parts) != 1:
            return None
        name = parts[0]
        if name in self.classes:
            return self.classes[name], "public"
        if name in self.headers:
            return self.headers[name], "public"
        if name.endswith("_p.h") and name in self.private_headers:
            return self.private_headers[name], "private"
        return None

    def resolve_identifier(self, name: str) -> tuple[str, str] | None:
        if name in self.classes:
            return self.classes[name], "public"
        if name in self.rhi_classes:
            return "GuiPrivate", "rhi"
        return None


def _rank(module: str) -> tuple[int, str]:
    return (PRIORITY.index(module) if module in PRIORITY else len(PRIORITY), module)


def _module_dirs(qt_root: Path) -> list[tuple[str, Path]]:
    dirs = [(fw.name[2:-len(".framework")], fw / "Headers") for fw in sorted((qt_root / "lib").glob("Qt*.framework"))
            if (fw / "Headers").is_dir()]
    dirs += [(d.name[2:], d) for d in sorted((qt_root / "include").glob("Qt*")) if d.is_dir()]
    return dirs


def build_qt_map(qt_root: Path | None = QT_ROOT) -> QtMap:
    """Scan the Qt headers; fall back to ``FALLBACK_CLASSES`` when none are found."""
    qt = QtMap()
    dirs = _module_dirs(qt_root) if qt_root is not None and qt_root.is_dir() else []
    for module, headers in dirs:
        qt.modules.add(module)
        for entry in headers.iterdir():
            if entry.is_file() and entry.name.startswith("Q") and "." not in entry.name:
                qt._set(qt.classes, entry.name, module)
            elif entry.is_file() and entry.name.endswith(".h"):
                qt._set(qt.headers, entry.name, module)
            elif entry.is_dir() and entry.name[:1].isdigit():
                for sub in ("private", "qpa"):
                    for header in (entry / f"Qt{module}" / sub).glob("*.h"):
                        qt._set(qt.private_headers, header.name, f"{module}Private")
                for header in (entry / f"Qt{module}" / "rhi").glob("*.h"):
                    qt.rhi_headers.add(header.name)
                    text = header.read_text(encoding="utf-8", errors="replace")
                    qt.rhi_classes.update(re.findall(r"^\s*(?:class|struct)\s+(?:Q_\w+_EXPORT\s+)?(Q\w+)\b(?!\s*;)",
                                                     text, re.MULTILINE))
    if qt.classes:
        qt.source = str(qt_root)
        qt.rhi_classes -= set(qt.classes)
        return qt
    for module, names in FALLBACK_CLASSES.items():
        qt.modules.add(module)
        for name in names:
            qt.classes[name] = module
            qt.headers[name.lower() + ".h"] = module
    qt.modules.update({"GuiPrivate"})
    qt.rhi_headers = set(FALLBACK_RHI_HEADERS)
    qt.rhi_classes = {c for names in FALLBACK_RHI_HEADERS.values() for c in names}
    qt.private_headers = {"qrhi_p.h": "GuiPrivate", "qguiapplication_p.h": "GuiPrivate",
                          "qwidget_p.h": "WidgetsPrivate", "qobject_p.h": "CorePrivate"}
    return qt


# ---------------------------------------------------------------------------------------------------------------------
# Source analysis
# ---------------------------------------------------------------------------------------------------------------------

def analyze_source(text: str, qt: QtMap) -> dict[str, Any]:
    """Qt modules a source file uses: ``modules`` -> name -> count, plus the Qt ``includes``."""
    stripped = strip_comments_and_strings(text)
    raw_lines = text.splitlines()
    code_lines: list[str] = []
    modules: dict[str, dict[str, int]] = defaultdict(lambda: defaultdict(int))
    includes: list[dict[str, str]] = []
    for index, line in enumerate(stripped.splitlines()):
        if not _INCLUDE.match(line):
            code_lines.append(line)
            continue
        code_lines.append("")
        match = _INCLUDE_PATH.search(raw_lines[index] if index < len(raw_lines) else line)
        resolved = qt.resolve_include(match.group(1)) if match else None
        if resolved:
            modules[resolved[0]][f"<{match.group(1)}>"] += 1
            includes.append({"header": match.group(1), "module": resolved[0], "kind": resolved[1]})
    code = "\n".join(code_lines)
    rhi_identifiers: set[str] = set()
    for match in _IDENT.finditer(code):
        resolved = qt.resolve_identifier(match.group(0))
        if resolved:
            modules[resolved[0]][match.group(0)] += 1
            if resolved[1] == "rhi":
                rhi_identifiers.add(match.group(0))
    return {"modules": {m: dict(sorted(c.items())) for m, c in sorted(modules.items())}, "includes": includes,
            "rhi_identifiers": sorted(rhi_identifiers), "code": code}


def _matching_brace(code: str, start: int) -> int:
    depth = 0
    for index in range(start, len(code)):
        if code[index] == "{":
            depth += 1
        elif code[index] == "}":
            depth -= 1
            if depth == 0:
                return index
    return len(code)


def find_classes(code: str) -> list[dict[str, Any]]:
    """Class declarations with their direct bases and overridden render hooks."""
    classes = []
    for match in _CLASS.finditer(code):
        bases = []
        for base in match.group(2).split(","):
            base = re.sub(r"<.*", "", base)
            base = re.sub(r"\b(?:public|protected|private|virtual)\b", "", base).strip()
            if base:
                bases.append(base.split("::")[-1])
        body = code[match.end() - 1:_matching_brace(code, match.end() - 1)]
        hooks = sorted({h for h in RENDER_OVERRIDES if re.search(rf"\b{h}\s*\(", body)})
        classes.append({"name": match.group(1), "bases": bases, "hooks": hooks})
    return classes


def painter_on_this(code: str) -> set[str]:
    """Classes whose methods construct ``QPainter`` on ``this`` or ``viewport()``."""
    owners = set()
    definitions = [(m.start(), m.group(1)) for m in _METHOD_DEF.finditer(code)]
    for match in _PAINTER_THIS.finditer(code):
        before = [name for position, name in definitions if position < match.start()]
        if before:
            owners.add(before[-1])
    return owners


def method_hooks(code: str) -> dict[str, set[str]]:
    hooks: dict[str, set[str]] = defaultdict(set)
    for match in re.finditer(r"\b(\w+)::(" + "|".join(RENDER_OVERRIDES) + r")\s*\(", code):
        hooks[match.group(1)].add(match.group(2))
    return hooks


# ---------------------------------------------------------------------------------------------------------------------
# CMake
# ---------------------------------------------------------------------------------------------------------------------

def _strip_cmake_comments(text: str) -> str:
    return "\n".join(re.sub(r"(?<![$\\])#.*", "", line) for line in text.splitlines())


def cmake_components(text: str) -> dict[str, Any]:
    """Qt components a CMakeLists.txt requests, and whether it runs ``qt_add_shaders``."""
    text = _strip_cmake_comments(text)
    requested: set[str] = set()
    for match in re.finditer(r"\b(?:set|list\s*\(\s*APPEND)\s*\(?\s*QT_REQUIRED_COMPONENTS\s+([^)]*)\)", text):
        requested.update(match.group(1).split())
    for match in re.finditer(r"\bfind_package\s*\(([^)]*)\)", text):
        words = match.group(1).split()
        if "COMPONENTS" in words and words[0].startswith(("Qt", "QT")):
            for word in words[words.index("COMPONENTS") + 1:]:
                if word in ("OPTIONAL_COMPONENTS", "CONFIG", "QUIET", "REQUIRED", "NO_MODULE", "NAMES"):
                    break
                requested.add(word)
    requested.update(re.findall(r"\bQt(?:\$\{QT_VERSION_MAJOR\}|6|5)?::(\w+)", text))
    requested = {m for m in requested if not m.startswith("$") and re.fullmatch(r"[A-Z]\w*", m)}
    return {"requested": sorted(requested), "qt_add_shaders": bool(re.search(r"\bqt_add_shaders\s*\(", text))}


def implied(modules: set[str]) -> set[str]:
    closure, stack = set(), list(modules)
    while stack:
        module = stack.pop()
        if module not in closure:
            closure.add(module)
            stack.extend(MODULE_DEPENDS.get(module, []))
    return closure


# ---------------------------------------------------------------------------------------------------------------------
# Scan
# ---------------------------------------------------------------------------------------------------------------------

def _target_dirs(root: Path) -> set[Path]:
    return {cmake.parent for cmake in root.rglob("CMakeLists.txt")
            if _TARGET.search(_strip_cmake_comments(cmake.read_text(encoding="utf-8", errors="replace")))}


def _unit_of(path: Path, scope_root: Path, targets: set[Path]) -> Path:
    for parent in path.parents:
        if parent in targets:
            return parent
        if parent == scope_root:
            break
    return scope_root


def rule_for(module: str, name: str | None = None) -> dict[str, str]:
    rule = RULES.get(module, DEFAULT_RULE)
    name = name.strip("<>").split("/")[-1].removesuffix(".h") if name else name
    replacement = rule["classes"].get(name, rule["replacement"]) if name else rule["replacement"]
    return {"verdict": rule["verdict"], "replacement": replacement}


def scan(repo_root: Path = REPO_ROOT, qt: QtMap | None = None,
         scopes: dict[str, str] | None = None) -> dict[str, Any]:
    qt = qt or build_qt_map()
    scopes = scopes or SCOPES
    files: dict[str, dict[str, Any]] = {}
    units: dict[str, dict[str, Any]] = {}
    class_index: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for scope, relative_root in scopes.items():
        scope_root = repo_root / relative_root
        if not scope_root.is_dir():
            continue
        targets = _target_dirs(scope_root)
        for path in sorted(scope_root.rglob("*")):
            if not path.is_file() or path.suffix not in EXTENSIONS | {".ui"}:
                continue
            unit_dir = _unit_of(path, scope_root, targets)
            unit = unit_dir.relative_to(repo_root).as_posix()
            if unit not in units:
                cmake = unit_dir / "CMakeLists.txt"
                units[unit] = {"scope": scope, "files": 0, "ui_forms": 0,
                               **(cmake_components(cmake.read_text(encoding="utf-8", errors="replace"))
                                  if cmake.is_file() else {"requested": [], "qt_add_shaders": False})}
            relative = path.relative_to(repo_root).as_posix()
            if path.suffix == ".ui":
                units[unit]["ui_forms"] += 1
                continue
            units[unit]["files"] += 1
            analysis = analyze_source(path.read_text(encoding="utf-8", errors="replace"), qt)
            code = analysis.pop("code")
            classes = find_classes(code)
            for cls in classes:
                cls.update(file=relative, unit=unit)
                class_index[cls["name"]].append(cls)
            files[relative] = {**analysis, "scope": scope, "unit": unit, "classes": [c["name"] for c in classes],
                               "painter_on_this": sorted(painter_on_this(code)),
                               "method_hooks": {k: sorted(v) for k, v in method_hooks(code).items()}}
    widget_classes = classify_widgets(files, class_index, qt)
    return assemble(files, units, widget_classes, qt)


def classify_widgets(files: dict[str, dict[str, Any]], class_index: dict[str, list[dict[str, Any]]],
                     qt: QtMap) -> dict[str, dict[str, Any]]:
    """Classes deriving (directly or through project classes) from a widget or QWindow, with render evidence."""
    kind: dict[str, str] = {}
    changed = True
    while changed:
        changed = False
        for name, decls in class_index.items():
            if name in kind:
                continue
            for base in (b for decl in decls for b in decl["bases"]):
                base_kind = ("window" if base in WINDOW_BASES
                             else "widget" if qt.classes.get(base) in WIDGET_MODULES
                             else None if base in qt.classes else kind.get(base))
                if base_kind:
                    kind[name] = base_kind
                    changed = True
                    break
    painters: dict[tuple[str, str], set[str]] = defaultdict(set)
    hooks: dict[tuple[str, str], set[str]] = defaultdict(set)
    for relative, info in files.items():
        for owner in info["painter_on_this"]:
            painters[(info["unit"], owner)].add(relative)
        for owner, names in info["method_hooks"].items():
            hooks[(info["unit"], owner)].update(names)
    result: dict[str, dict[str, Any]] = {}
    for name, decls in class_index.items():
        if name not in kind:
            continue
        for decl in decls:
            reasons = []
            render_bases = [b for b in decl["bases"] if b in RENDER_BASES]
            if render_bases:
                reasons.append("derives " + ", ".join(render_bases))
            overrides = sorted(set(decl["hooks"]) | hooks.get((decl["unit"], name), set()))
            if overrides:
                reasons.append("overrides " + ", ".join(overrides))
            if painters.get((decl["unit"], name)):
                reasons.append("QPainter on this/viewport()")
            result[f"{decl['unit']}::{name}"] = {"class": name, "file": decl["file"], "unit": decl["unit"],
                                                 "bases": decl["bases"], "kind": kind[name],
                                                 "renders": bool(reasons), "reasons": reasons}
    changed = True
    while changed:
        changed = False
        rendering = {c["class"] for c in result.values() if c["renders"]}
        for cls in result.values():
            inherited = [b for b in cls["bases"] if b in rendering and b not in qt.classes]
            if inherited and not cls["renders"]:
                cls["renders"] = True
                cls["reasons"].append("derives rendering " + ", ".join(inherited))
                changed = True
    return result


def assemble(files: dict[str, dict[str, Any]], units: dict[str, dict[str, Any]],
             widget_classes: dict[str, dict[str, Any]], qt: QtMap) -> dict[str, Any]:
    scopes: dict[str, Any] = {}
    gui_private: list[dict[str, Any]] = []
    unit_used: dict[str, set[str]] = defaultdict(set)
    render_files = {c["file"] for c in widget_classes.values() if c["renders"]}
    render_names = {(c["unit"], c["class"]) for c in widget_classes.values() if c["renders"]}
    for relative, info in sorted(files.items()):
        scope = scopes.setdefault(info["scope"], {"files": 0, "modules": {}, "widgets": {
            "renders_files": [], "chrome_files": [], "render_classes": [], "chrome_classes": [],
            "window_classes": []}})
        scope["files"] += 1
        for module, names in info["modules"].items():
            unit_used[info["unit"]].add(module)
            entry = scope["modules"].setdefault(module, {**rule_for(module), "files": {}, "uses": 0, "classes": {}})
            entry["files"][relative] = names
            entry["uses"] += sum(names.values())
            for name, count in names.items():
                entry["classes"][name] = entry["classes"].get(name, 0) + count
        for include in info["includes"]:
            if include["module"].endswith("Private"):
                gui_private.append({"file": relative, **include})
        if info["rhi_identifiers"]:
            gui_private.append({"file": relative, "header": None, "module": "GuiPrivate", "kind": "rhi",
                                "classes": info["rhi_identifiers"]})
        if "Widgets" in info["modules"] or any(m in WIDGET_MODULES for m in info["modules"]):
            implements = any((info["unit"], owner) in render_names for owner in info["method_hooks"])
            implements |= any((info["unit"], owner) in render_names for owner in info["painter_on_this"])
            bucket = "renders_files" if relative in render_files or implements else "chrome_files"
            scope["widgets"][bucket].append(relative)
    for cls in widget_classes.values():
        scope = scopes[files[cls["file"]]["scope"]]["widgets"]
        bucket = "window_classes" if cls["kind"] == "window" else "render_classes" if cls["renders"] \
            else "chrome_classes"
        scope[bucket].append({k: cls[k] for k in ("class", "file", "bases", "reasons")})
    for scope in scopes.values():
        scope["modules"] = dict(sorted(scope["modules"].items()))
        for module, entry in scope["modules"].items():
            entry["files"] = dict(sorted(entry["files"].items(), key=lambda kv: -sum(kv[1].values())))
            entry["file_count"] = len(entry["files"])
            entry["classes"] = {name: {"count": count, **rule_for(module, name)}
                                for name, count in sorted(entry["classes"].items(), key=lambda kv: -kv[1])}
    for unit, info in units.items():
        used = unit_used.get(unit, set())
        if info["ui_forms"]:
            used = used | {"Widgets"}
        requested = set(info["requested"])
        closure = implied(requested)
        unused = sorted(m for m in requested - used if not (m == "ShaderTools" and info["qt_add_shaders"]))
        info["used"] = sorted(used)
        info["requested_unused"] = unused
        info["build_time_only"] = ["ShaderTools"] if "ShaderTools" in requested and info["qt_add_shaders"] else []
        info["implied"] = sorted((used - requested) & closure)
        info["transitive"] = sorted(used - closure)
    return {
        "schema_version": 1,
        "generated_by": "scripts/quality/audit_qt_modules.py",
        "qt_headers": {"source": qt.source, "classes": len(qt.classes), "rhi_classes": len(qt.rhi_classes),
                       "fallback": qt.source == "fallback"},
        "rules": RULES,
        "scopes": dict(sorted(scopes.items())),
        "units": dict(sorted(units.items())),
        "gui_private": gui_private,
    }


# ---------------------------------------------------------------------------------------------------------------------
# Output
# ---------------------------------------------------------------------------------------------------------------------

def _classes_text(names: dict[str, int], limit: int = 6) -> str:
    items = sorted(names.items(), key=lambda kv: -kv[1])
    text = ", ".join(f"`{n}`" + (f"×{c}" if c > 1 else "") for n, c in items[:limit])
    return text + (f" (+{len(items) - limit})" if len(items) > limit else "")


def render_markdown(report: dict[str, Any], top: int = 10) -> str:
    qt = report["qt_headers"]
    lines = ["# Qt modules of the display stack and applications", "",
             "Generated by `scripts/quality/audit_qt_modules.py`. Do not edit by hand. Comments and strings are "
             "removed; `#include` lines and Q-identifiers in code are attributed to Qt modules.", ""]
    lines.append("Class map: " + ("**built-in fallback (Qt headers not found)**" if qt["fallback"]
                                   else f"`{qt['source']}` ({qt['classes']:,} classes, {qt['rhi_classes']} QRhi)")
                 + ".")
    lines += ["", "## Modules per scope", "", "| Scope | Module | Files | Uses | Verdict | Replacement |",
              "|---|---|---:|---:|---|---|"]
    for name, scope in report["scopes"].items():
        for module, entry in sorted(scope["modules"].items(), key=lambda kv: -kv[1]["file_count"]):
            rule = ("—", "—") if module in BASE_MODULES else (entry["verdict"], entry["replacement"])
            lines.append(f"| {name} | {module} | {entry['file_count']} | {entry['uses']:,} | {rule[0]} | {rule[1]} |")
    lines += ["", "## Targets: requested vs used", "",
              "| Target | Files | .ui | Requested | Used | Requested, unused | Transitive only |",
              "|---|---:|---:|---|---|---|---|"]
    for unit, info in report["units"].items():
        lines.append(f"| `{unit}` | {info['files']} | {info['ui_forms']} | {' '.join(info['requested']) or '—'} "
                     f"| {' '.join(info['used']) or '—'} | {' '.join(info['requested_unused']) or '—'}"
                     + (f" (build-time: {' '.join(info['build_time_only'])})" if info["build_time_only"] else "")
                     + f" | {' '.join(info['transitive']) or '—'} |")
    lines += ["", "## Modules outside Core/Gui", ""]
    for name, scope in report["scopes"].items():
        for module, entry in scope["modules"].items():
            if module in BASE_MODULES or module == "Widgets":
                continue
            lines += [f"### {name}: {module} — {entry['verdict']}", "", f"Replacement: {entry['replacement']}", "",
                      "| File | Uses |", "|---|---|"]
            lines += [f"| `{f}` | {_classes_text(n)} |" for f, n in list(entry["files"].items())[:top]]
            if entry["file_count"] > top:
                lines.append(f"| … {entry['file_count'] - top} more | |")
            specific = {c: v for c, v in entry["classes"].items()
                        if not c.startswith("<") and v["replacement"] != entry["replacement"]}
            lines += [f"- `{c}`: {v['replacement']}" for c, v in specific.items()]
            lines.append("")
    lines += ["## GuiPrivate / private headers", ""]
    if not report["gui_private"]:
        lines.append("None.")
    for item in report["gui_private"]:
        what = f"`<{item['header']}>`" if item["header"] else "identifiers " + ", ".join(
            f"`{c}`" for c in item["classes"])
        lines.append(f"- `{item['file']}`: {what} → {item['module']} ({item['kind']})")
    lines += ["", "`rhi` = semi-public QRhi API (`rhi/*.h`, symbols in QtGui); `private` = `*_p.h`/qpa.", "",
              "## Widgets: renders vs chrome", "",
              "| Scope | Render files | Chrome files | Render classes | Chrome classes | QWindow classes |",
              "|---|---:|---:|---:|---:|---:|"]
    for name, scope in report["scopes"].items():
        w = scope["widgets"]
        lines.append(f"| {name} | {len(w['renders_files'])} | {len(w['chrome_files'])} | {len(w['render_classes'])} "
                     f"| {len(w['chrome_classes'])} | {len(w['window_classes'])} |")
    lines += ["", "### Rendering classes", "", "| Class | File | Bases | Evidence |", "|---|---|---|---|"]
    for scope in report["scopes"].values():
        for cls in sorted(scope["widgets"]["render_classes"], key=lambda c: c["file"]):
            lines.append(f"| `{cls['class']}` | `{cls['file']}` | {', '.join(cls['bases'])} "
                         f"| {'; '.join(cls['reasons'])} |")
    window = [c for s in report["scopes"].values() for c in s["widgets"]["window_classes"]]
    if window:
        lines += ["", "QWindow-based (already Gui): " + ", ".join(f"`{c['class']}` ({c['file']})" for c in window)]
    for name, scope in report["scopes"].items():
        lines += ["", f"<details><summary>{name}: chrome files ({len(scope['widgets']['chrome_files'])})</summary>", ""]
        lines += [f"- `{f}`" for f in scope["widgets"]["chrome_files"]]
        lines += ["", "</details>"]
    unused = [(u, i["requested_unused"]) for u, i in report["units"].items() if i["requested_unused"]]
    lines += ["", "## Requested but unused", ""]
    lines += [f"- `{u}`: {' '.join(m)}" for u, m in unused] or ["None."]
    return "\n".join(lines) + "\n"


def check_policy(report: dict[str, Any], policy: dict[str, Any]) -> list[str]:
    """Lists every module a target requests or uses beyond its allowlist, and every module it requests unused."""
    allowed = policy["targets"]
    default = set(policy["default"])
    problems = []
    for unit, info in sorted(report["units"].items()):
        permitted = set(allowed.get(unit, default))
        for module in sorted((set(info["requested"]) | set(info["used"])) - permitted):
            problems.append(f"{unit}: Qt {module} is not in its allowlist ({' '.join(sorted(permitted))})")
        for module in info["requested_unused"]:
            problems.append(f"{unit}: requests Qt {module} but does not use it")
    for unit in sorted(set(allowed) - set(report["units"])):
        problems.append(f"{unit}: listed in the allowlist but no such target")
    return problems


def unrequested_prefetched(repo_root: Path = REPO_ROOT) -> list[str]:
    """Qt modules src/CMakeLists.txt finds for the whole tree that no target requests, so the Qt build can drop them."""
    top = _strip_cmake_comments((repo_root / "src" / "CMakeLists.txt").read_text(encoding="utf-8"))
    prefetched = set()
    for match in re.finditer(r"(?:set|list)\s*\(\s*(?:APPEND\s+)?_MNE_QT_COMPONENTS\s+([^)]*)\)", top):
        prefetched.update(w for w in match.group(1).split() if re.fullmatch(r"[A-Z]\w*", w))
    requested: set[str] = set()
    for cmake in (repo_root / "src").rglob("CMakeLists.txt"):
        if cmake.parent == repo_root / "src" or "external" in cmake.parts:
            continue
        requested.update(cmake_components(cmake.read_text(encoding="utf-8", errors="replace"))["requested"])
    return sorted(prefetched - requested)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--json", type=Path, dest="json_path", help="write the JSON report")
    parser.add_argument("--markdown", type=Path, help="write the Markdown summary")
    parser.add_argument("--qt-root", type=Path, default=QT_ROOT, help="Qt install with lib/ or include/ headers")
    parser.add_argument("--check", action="store_true",
                        help="fail when a target requests or uses a Qt module outside its allowlist in "
                             "qt_usage_policy.json (qt_modules), or requests one it does not use")
    args = parser.parse_args(argv)

    qt = build_qt_map(args.qt_root)
    if qt.source == "fallback":
        print(f"WARNING Qt headers not found under {args.qt_root}; using the built-in class map", file=sys.stderr)
    report = scan(REPO_ROOT, qt)
    if args.check:
        policy = json.loads(POLICY_FILE.read_text(encoding="utf-8"))["qt_modules"]
        problems = check_policy(report, policy)
        problems += [f"src/CMakeLists.txt: finds Qt {m}, which no target requests" for m in unrequested_prefetched()]
        for problem in problems:
            print(f"FAIL: {problem}")
        if not problems:
            print(f"PASS: {len(report['units'])} targets use only the Qt modules they are allowed.")
        return 1 if problems else 0
    if args.json_path:
        args.json_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if args.markdown:
        args.markdown.write_text(render_markdown(report), encoding="utf-8")
    if not args.json_path and not args.markdown:
        sys.stdout.write(render_markdown(report))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
