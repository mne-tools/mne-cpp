#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# @since    3.0.0
# @date     November, 2026
#
#
# @brief    Count Qt usage per library so it can only fall (gate G13).
"""Qt usage per library for gate G13.

For every library under ``src/libs/mne`` (a directory whose CMakeLists.txt calls ``add_library``; nested
libraries such as ``acq/fiff`` count separately) it reports:

* ``tokens``                  Qt identifiers in code, by category (comments, strings and ``#include`` lines removed)
* ``public_header_includes``  ``#include <Q...>`` in public headers (``detail/`` and ``*_p.h`` are private)
* ``q_object``                ``Q_OBJECT`` classes
* ``modules``                 Qt modules the library's CMakeLists.txt requests

The policy keeps a ceiling per library and metric; CI fails when one is exceeded, so Qt use may only fall and a
library that has become Qt-free stays Qt-free. Uses Qt keeps by design (D16) are listed with their reason.

Usage
-----
    python3 scripts/quality/audit_qt_usage.py --markdown /dev/stdout
    python3 scripts/quality/audit_qt_usage.py --ceilings
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))
from audit_maintainability import strip_comments_and_strings  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parents[2]
POLICY_FILE = Path(__file__).resolve().parent / "qt_usage_policy.json"
EXTENSIONS = {".h", ".hpp", ".cpp", ".cc", ".cxx"}

_TOKEN = re.compile(
    r"\bQT?_[A-Z_]+\b|\bQ(?:FAIL|SKIP|VERIFY2?|COMPARE|TRY_\w+)\b|\bQt(?:Concurrent)?::\w+|\bQ[A-Z]+[a-z]\w*"
    r"|\bq[A-Z]\w*"
    r"|\bq(?:u?int(?:8|16|32|64)?|real|sizetype|ptrdiff|u?intptr)\b|\bemit\b|\b(?:signals|slots)\s*:")
_INCLUDE = re.compile(r"^\s*#\s*include\s*<(Q\w*)", re.MULTILINE)
_INCLUDE_LINE = re.compile(r"^\s*#\s*include\b.*$", re.MULTILINE)
_MODULES = re.compile(r"QT_REQUIRED_COMPONENTS\s+([^)$]*)\)|Qt(?:\$\{QT_VERSION_MAJOR\}|6|5)::(\w+)")

# First match wins; the categories follow the replaced/kept split of G13.
CATEGORIES: list[tuple[str, re.Pattern[str]]] = [(name, re.compile(pattern)) for name, pattern in [
    ("objects_signals", r"^(?:Q_OBJECT|Q_GADGET|Q_SIGNALS|Q_SLOTS|Q_INVOKABLE|Q_PROPERTY|Q_ENUMS?|Q_DECLARE_METATYPE"
                        r"|QObject|QMetaObject|QMetaType|QVariant\w*|QEvent|QCoreApplication|QPointer|emit"
                        r"|signals\s*:|slots\s*:)$"),
    ("logging", r"^(?:q(?:Debug|Info|Warning|Critical|Fatal|InstallMessageHandler)|QDebug|QMessageLog\w*|QtMsgType)$"),
    ("strings", r"^(?:QString\w*|QChar|QByteArray\w*|QLatin1\w*|QStringLiteral|QUtf8StringView|QAnyStringView)$"),
    ("containers", r"^(?:QList|QVector|QMap|QMultiMap|QHash|QMultiHash|QSet|QPair|QQueue|QStack|QVarLengthArray"
                   r"|QSpan)$"),
    ("smart_pointers", r"^(?:QSharedPointer|QScopedPointer|QScopedArrayPointer|QWeakPointer|QSharedData\w*"
                       r"|QExplicitlySharedDataPointer|QEnableSharedFromThis)$"),
    ("files", r"^(?:QFile\w*|QDir\w*|QDataStream|QTextStream|QIODevice\w*|QSaveFile|QTemporary\w+|QStandardPaths"
              r"|QBuffer|QSettings|QResource|QLibrary|QProcess\w*)$"),
    ("threads", r"^(?:QThread\w*|QMutex\w*|QRecursiveMutex|QReadWriteLock|QReadLocker|QWriteLocker"
                r"|QWaitCondition|QAtomic\w+|QSemaphore\w*|QRunnable|QFuture\w*|QPromise|QtConcurrent::\w+)$"),
    ("random", r"^QRandomGenerator\w*$"),
    ("time", r"^(?:QDateTime|QDate|QTime|QElapsedTimer|QTimer|QDeadlineTimer|QTimeZone|QChronoTimer)$"),
    ("gui", r"^(?:QVector[234]D|QMatrix[234]x[234]|QMatrix4x4|QQuaternion|QColor|QImage|QPixmap|QFont\w*|QPainter\w*"
            r"|QPen|QBrush|QPolygonF?|QTransform|QGuiApplication|QOpenGL\w*|QRhi\w*|QWidget|QIcon)$"),
    ("json", r"^(?:QJson\w*|QCbor\w*)$"),
    ("network", r"^(?:QTcp\w*|QUdp\w*|QHostAddress|QHostInfo|QAbstractSocket|QNetwork\w*|QLocal(?:Socket|Server)"
                r"|QSsl\w*)$"),
    ("regex", r"^(?:QRegularExpression\w*|QRegExp)$"),
    ("global", r"^(?:QT?_[A-Z_]+|Qt::\w+|q[A-Z]\w*|qu?int\d*|qreal|qsizetype|qptrdiff|qu?intptr)$"),
]]


def category(token: str) -> str:
    for name, pattern in CATEGORIES:
        if pattern.match(token):
            return name
    return "other"


def library_roots(repo_root: Path, scan_root: str) -> list[str]:
    """Library directories relative to ``scan_root``, deepest first so nested libraries win."""
    roots = [cmake.parent.relative_to(repo_root / scan_root).as_posix()
             for cmake in (repo_root / scan_root).rglob("CMakeLists.txt")
             if re.search(r"\badd_library\s*\(", cmake.read_text(encoding="utf-8", errors="replace"))]
    return sorted(roots, key=lambda root: (-root.count("/"), root))


def count_tokens(text: str) -> dict[str, int]:
    code = _INCLUDE_LINE.sub("", strip_comments_and_strings(text))
    counts: dict[str, int] = {}
    for match in _TOKEN.finditer(code):
        name = category(re.sub(r"\s*:$", ":", match.group(0)).replace(" ", ""))
        counts[name] = counts.get(name, 0) + 1
    return counts


def is_public_header(relative: str) -> bool:
    return relative.endswith((".h", ".hpp")) and "/detail/" not in f"/{relative}" and not relative.endswith("_p.h")


def scan(repo_root: Path, policy: dict[str, Any]) -> dict[str, dict[str, Any]]:
    scan_root = policy["scan_root"]
    roots = library_roots(repo_root, scan_root)
    excluded = tuple(policy.get("exclude_path_fragments", {}))
    libraries: dict[str, dict[str, Any]] = {
        root: {"tokens": 0, "by_category": {}, "public_header_includes": 0, "q_object": 0, "files": 0}
        for root in roots}
    for path in sorted((repo_root / scan_root).rglob("*")):
        relative = path.relative_to(repo_root / scan_root).as_posix()
        if path.suffix not in EXTENSIONS or not path.is_file() or any(f in f"/{relative}" for f in excluded):
            continue
        library = next((root for root in roots if relative.startswith(root + "/")), None)
        if library is None:
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        entry = libraries[library]
        entry["files"] += 1
        for name, value in count_tokens(text).items():
            entry["by_category"][name] = entry["by_category"].get(name, 0) + value
            entry["tokens"] += value
        entry["q_object"] += len(re.findall(r"\bQ_OBJECT\b", strip_comments_and_strings(text)))
        if is_public_header(path.relative_to(repo_root / scan_root / library).as_posix()):
            entry["public_header_includes"] += len(_INCLUDE.findall(strip_comments_and_strings(text)))
    for root, entry in libraries.items():
        cmake = (repo_root / scan_root / root / "CMakeLists.txt").read_text(encoding="utf-8", errors="replace")
        modules: set[str] = set()
        for listed, single in _MODULES.findall(strip_comments_and_strings(cmake)):
            modules.update(listed.split() if listed else [single])
        entry["modules"] = sorted(m for m in modules if not m.startswith("$"))
        entry["by_category"] = dict(sorted(entry["by_category"].items()))
    return libraries


def build_report(repo_root: Path = REPO_ROOT, policy: dict[str, Any] | None = None) -> dict[str, Any]:
    policy = policy or json.loads(POLICY_FILE.read_text(encoding="utf-8"))
    libraries = scan(repo_root, policy)
    return {
        "schema_version": 1,
        "generated_by": "scripts/quality/audit_qt_usage.py",
        "policy": "scripts/quality/qt_usage_policy.json",
        "qt_by_design": policy.get("qt_by_design", []),
        "kept": {k: v for k, v in policy.get("kept", {}).items() if not k.startswith("$")},
        "summary": {
            "libraries": len(libraries),
            "qt_free": sorted(name for name, lib in libraries.items() if lib["tokens"] == 0 and not lib["modules"]),
            "tokens": sum(lib["tokens"] for lib in libraries.values()),
            "gui_outside_design": sorted(name for name, lib in libraries.items()
                                         if name not in policy.get("qt_by_design", [])
                                         and ({"Gui", "Widgets"} & set(lib["modules"]) or lib["by_category"].get("gui"))),
        },
        "libraries": dict(sorted(libraries.items())),
    }


def check_ceilings(report: dict[str, Any], policy: dict[str, Any]) -> list[str]:
    """Fail any library above its ceiling or using a Qt module it did not use before; new libraries need a ceiling."""
    ceilings = {k: v for k, v in policy.get("ceilings", {}).items() if not k.startswith("$")}
    failures: list[str] = []
    for name, lib in report["libraries"].items():
        limit = ceilings.get(name)
        if limit is None:
            failures.append(f"{name}: no ceiling")
            continue
        for metric in ("tokens", "public_header_includes"):
            if lib[metric] > limit[metric]:
                failures.append(f"{name}: {metric} {lib[metric]} > ceiling {limit[metric]}")
        gained = sorted(set(lib["modules"]) - set(limit["modules"]))
        if gained:
            failures.append(f"{name}: links Qt {', '.join(gained)}, not allowed by its ceiling")
    return failures


def current_ceilings(report: dict[str, Any]) -> dict[str, Any]:
    """Ceilings equal to today's counts, for lowering the policy after Qt was removed."""
    return {name: {"tokens": lib["tokens"], "public_header_includes": lib["public_header_includes"],
                   "modules": lib["modules"]} for name, lib in report["libraries"].items()}


def render_markdown(report: dict[str, Any]) -> str:
    summary = report["summary"]
    categories = sorted({c for lib in report["libraries"].values() for c in lib["by_category"]})
    lines = [
        "# MNE-CPP Qt usage per library",
        "",
        "Generated by `scripts/quality/audit_qt_usage.py`. Do not edit by hand.",
        "Qt identifiers in code with comments, string literals and `#include` lines removed; each library's count may"
        " only fall (gate G13).",
        "",
        f"{summary['libraries']} libraries, {summary['tokens']:,} Qt tokens. Qt-free: "
        + (", ".join(f"`{name}`" for name in summary["qt_free"]) or "none") + ". Qt Gui outside "
        + ", ".join(f"`{name}`" for name in report["qt_by_design"]) + ": "
        + (", ".join(f"`{name}`" for name in summary["gui_outside_design"]) or "none") + ".",
        "",
        "| Library | Qt modules | Tokens | Public-header includes | Q_OBJECT | " + " | ".join(categories) + " |",
        "|---|---|---:|---:|---:|" + "---:|" * len(categories),
    ]
    for name, lib in report["libraries"].items():
        cells = " | ".join(f"{lib['by_category'].get(c, 0):,}" for c in categories)
        lines.append(f"| `{name}` | {' '.join(lib['modules']) or '—'} | {lib['tokens']:,} "
                     f"| {lib['public_header_includes']:,} | {lib['q_object']:,} | {cells} |")
    if report["kept"]:
        lines += ["", "## Kept Qt uses (D16)", "", "| Library | Use | Reason |", "|---|---|---|"]
        for name, uses in sorted(report["kept"].items()):
            lines += [f"| `{name}` | {use} | {reason} |" for use, reason in sorted(uses.items())]
    return "\n".join(lines) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--json", type=Path, dest="json_path", help="write the JSON report")
    parser.add_argument("--markdown", type=Path, help="write the Markdown summary")
    parser.add_argument("--ceilings", action="store_true", help="fail if a library exceeds its policy ceiling")
    parser.add_argument("--print-ceilings", action="store_true", help="print ceilings equal to today's counts")
    args = parser.parse_args(argv)

    report = build_report()
    if args.ceilings:
        failures = check_ceilings(report, json.loads(POLICY_FILE.read_text(encoding="utf-8")))
        for failure in failures:
            print(f"ERROR {failure}")
        print("OK: no library uses more Qt than its ceiling." if not failures else f"{len(failures)} over ceiling")
        return 1 if failures else 0
    if args.print_ceilings:
        print(json.dumps(current_ceilings(report), indent=2))
    if args.json_path:
        args.json_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if args.markdown:
        args.markdown.write_text(render_markdown(report), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
