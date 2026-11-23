#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     September, 2026
#
#
# @brief    Count maintainability debt per file so it can be ratcheted and assigned.
"""Maintainability baseline for gate G5.

Counts lexical debt candidates in maintained C++ under ``src/libs/mne``,
``src/apps`` and ``src/tools``:

* ``null_macro``        uses of ``NULL`` or ``Q_NULLPTR`` instead of ``nullptr``
* ``typedef_struct``    C-style ``typedef struct`` / ``typedef enum`` / ``typedef union``
* ``c_style_cast``      casts to a builtin/pointer type written as ``(T)expr``
* ``raw_new``           non-Qt ``new T`` not immediately handed to a smart pointer or a parent
* ``qt_new_unparented`` ``new QXxx`` -- or of a project class deriving from a Qt class -- that gets no owner in
                        the expression that creates it: neither a parent argument nor an ownership-taking call
                        (see below)
* ``raw_delete``        explicit ``delete`` / ``delete[]``
* ``console_io``        ``printf``/``fprintf``/``std::cout``/``std::cerr``
* ``numeric_define``    ``#define NAME <number>`` constants that could be ``constexpr``

A Qt ``new`` has an owner when its constructor names a parent -- the call is read to its end,
also across lines -- that is when

* its last argument is ``this``, a ``parent...`` name (also ``x->parentWidget()``), a pointer named by
  the project convention (``pBox``, ``m_pMenu``, also through members such as ``m_pUi->pList``), the raw
  pointer of a smart pointer (``owner.get()``, ``m_pWindow.data()``), ``x->viewport()`` or
  ``x->centralWidget()``, a Designer ``ui`` member
  (``m_pUi->groupBox``), a name the file declares as a pointer to a Qt object (``QWidget* container``,
  ``auto* row = new QWidget``) or the address of a Qt object the file declares on the stack
  (``QDialog dlg;`` ... ``new QLabel(text, &dlg)``);
* any argument is ``this``, a ``parent...`` name or such a declared Qt object pointer, which covers
  trailing window flags: ``new View(name, this, Qt::Window)``;
* it is a ``QTreeWidgetItem`` whose first argument is such a parent: ``new QTreeWidgetItem(pRoot, {name})``.

A ``new QXxx`` without an argument list counts as well (``m_pView(new QTableView)``); Designer's ``Ui::Name``
structs are not Qt objects even though the widget class they fill shares their name.

A Qt ``new`` that is itself an argument of an ownership-taking Qt call on the
same line already has an owner and is not counted, e.g.
``layout->addWidget(new QLabel(text))``.  The call must be the innermost one
enclosing the ``new`` and be one of ``addWidget``, ``insertWidget``,
``addLayout``, ``insertLayout``, ``setLayout``, ``addItem``, ``insertItem``,
``addRow``, ``setWidget``, ``setCentralWidget``, ``setMenuBar``,
``setStatusBar``, ``addDockWidget``, ``addToolBar``, ``addTab``, ``insertTab``,
``addPage``, ``setItem``, ``setChild``, ``appendRow``, ``insertRow``,
``appendColumn``, ``setHorizontalHeaderItem``, ``setVerticalHeaderItem``,
``addTopLevelItem``, ``insertTopLevelItem``, ``addChild``, ``setViewport``,
``setCornerWidget``, ``setDefaultWidget``, ``setMenu`` or -- for a ``new
Q*Action`` only -- ``addAction``.  Calls that do not take ownership, such as
``setModel`` or ``setItemDelegate``, still count.

Comments and string literals are stripped before matching, so a ``NULL`` in a
doc comment or a ``"printf"`` in a message is not counted.  Every metric lists
the files that contribute to it, so the debt can be cut in path-disjoint
tranches.  Compiler warnings are not counted here: the build is
already ``-Werror``, so the warning count is zero by construction.

Usage
-----
    python3 scripts/quality/audit_maintainability.py
    python3 scripts/quality/audit_maintainability.py --json doc/release/v2.4.0/maintainability-baseline.json \\
        --markdown doc/release/v2.4.0/maintainability-baseline.md
    # fail if any metric exceeds the committed baseline (the ratchet)
    python3 scripts/quality/audit_maintainability.py --check doc/release/v2.4.0/maintainability-baseline.json
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[2]
POLICY_FILE = Path(__file__).resolve().parent / "maintainability_policy.json"

_BUILTIN = r"(?:unsigned\s+|signed\s+|const\s+)?(?:int|float|double|char|long|short|bool|size_t|u?int(?:8|16|32|64)_t|q(?:u)?int(?:8|16|32|64)|qreal|void)"
PATTERNS: dict[str, re.Pattern[str]] = {
    "null_macro": re.compile(r"\b(?:NULL|Q_NULLPTR)\b"),
    "typedef_struct": re.compile(r"\btypedef\s+(?:struct|enum|union)\b"),
    "c_style_cast": re.compile(rf"(?<![\w\]\)>])\(\s*{_BUILTIN}(?:\s*\*+)?\s*\)\s*(?=[\w(\-])"),
    "raw_new": re.compile(r"\bnew\s+(?!\(std::nothrow\))[A-Za-z_][\w:]*(?:<[^;]*?>)?\s*[\[(;{]"),
    "raw_delete": re.compile(r"(?<![\w.>:])delete\b\s*(?:\[\s*\])?\s*[A-Za-z_(*]"),
    "console_io": re.compile(r"(?<![\w:.>])(?:std::)?(?:printf|fprintf|puts)\s*\(|(?<![\w.>:])(?:std::)?(?:cout|cerr|clog)\b"),
    "numeric_define": re.compile(r"^\s*#\s*define\s+[A-Z_][A-Z0-9_]*\s+\(?-?\d", re.MULTILINE),
}
# A `new` whose result lands in a smart pointer (including the project's X::SPtr/UPtr typedefs) is owned.
_OWNED_NEW = re.compile(
    r"(?:make_(?:unique|shared)|(?:std::(?:unique|shared)_ptr|Q(?:Shared|Scoped|ScopedArray)Pointer)\s*<[^;]*>\s*\w*\s*[({=]"
    r"|\b(?:Const)?(?:S|U|SD|CS)Ptr\s*(?:\w+\s*)?[({=]|\.reset\s*\(|::create\s*\()\s*[^;]*$"
)
# Qt objects are usually handed to a parent or layout after construction, which a lexical scan cannot see.
_QT_NEW = re.compile(r"\bnew\s+Q[A-Z]\w*")
# `class [EXPORT_MACRO] Name [final] : bases {` -- project classes deriving from Qt types are Qt-owned too.
_CLASS_DECL = re.compile(r"\bclass\s+(?:[A-Z0-9_]+\s+)?(\w+)\s*(?:final\s*)?:\s*([^{;()]+)\{")
_NEW_TYPE = re.compile(r"new\s+([\w:]+)")
# Like PATTERNS["raw_new"], but also `new T)` / `new T,` (no argument list), e.g. `m_view(new QTableView)`.
_ANY_NEW = re.compile(r"\bnew\s+(?!\(std::nothrow\))[A-Za-z_][\w:]*(?:<[^;]*?>)?\s*[\[(;{),]")
# Qt calls that take ownership of a pointer argument (layouts, scenes, item models/views, main windows, ...).
_OWNING_CALLS = frozenset({
    "addWidget", "insertWidget", "addLayout", "insertLayout", "setLayout", "addItem", "insertItem", "addRow",
    "setWidget", "setCentralWidget", "setMenuBar", "setStatusBar", "addDockWidget", "addToolBar", "addTab",
    "insertTab", "addPage", "setItem", "setChild", "appendRow", "insertRow", "appendColumn",
    "setHorizontalHeaderItem", "setVerticalHeaderItem", "addTopLevelItem", "insertTopLevelItem", "addChild",
    "setViewport", "setCornerWidget", "setDefaultWidget", "setMenu",
})
_CALL_NAME = re.compile(r"(\w+)\s*$")
# A last constructor argument that names a parent object: `this`, a `parent...` name, a pointer named by the
# project's convention (`pX`, `m_pX`), possibly reached through members, a Designer `ui` member, or the raw
# pointer of a smart pointer (`x.get()`, `x.data()`), or a view's viewport / a window's central widget.
_PARENT_ARG = re.compile(
    r"(?:this|(?:\w+(?:->|\.))*(?:m_)?p[A-Z]\w*|(?:\w+(?:->|\.))*\w*[pP]arent\w*(?:\(\))?|(?:m_pUi|m_ui|ui)(?:->|\.)\w+"
    r"|(?:\w+(?:->|\.))*\w+(?:\.|->)(?:get|data|viewport|centralWidget)\(\))"
)


# `this` or a `parent...` name names the parent wherever it stands, e.g. before trailing window flags.
_ANY_PARENT_ARG = re.compile(r"this|(?:\w+(?:->|\.))*\w*[pP]arent\w*(?:\(\))?")


def _ctor_args(line: str, open_paren: int, partial: bool = False) -> list[str] | None:
    """The top-level arguments of the argument list opened at ``open_paren``, if it closes on this line.

    With ``partial`` the arguments completed on this line are returned even when the list goes on.
    """
    depth, start, args = 0, open_paren + 1, []
    for index in range(open_paren + 1, len(line)):
        char = line[index]
        if char in "([{":
            depth += 1
        elif char in ")]}":
            if depth == 0:
                return [*args, line[start:index].strip()] if char == ")" else None
            depth -= 1
        elif char == "," and depth == 0:
            args.append(line[start:index].strip())
            start = index + 1
    return args if partial else None


# (type, name) of pointer declarations: `QWidget* name`, `QPointer<QWidget> name`, and `auto* name = new QWidget`
_POINTER_DECLS = (
    re.compile(r"\b(?:\w+::)*([A-Z]\w*)\s*\*\s*(?:const\s+)?(\w+)\s*[=;,)]"),
    re.compile(r"\bQPointer\s*<\s*(?:\w+::)*([A-Z]\w*)\s*>\s*(\w+)"),
)
_AUTO_NEW_DECL = re.compile(r"\bauto\s*\*?\s*(\w+)\s*=\s*new\s+(?:\w+::)*([A-Z]\w*)")
_QT_OBJECT_DECL = re.compile(r"^\s*(?:\w+::)*([A-Z]\w*)\s+(\w+)\s*[;({]", re.MULTILINE)


def _qt_object_names(code: str, qt_classes: set[str] | frozenset[str]) -> tuple[set[str], set[str]]:
    """Names this file declares as pointers to Qt objects, and as Qt objects on the stack (passed as ``&name``)."""
    def is_qt(name: str) -> bool:
        return re.match(r"Q[A-Z]", name) is not None or name in qt_classes

    pointers = {name for pattern in _POINTER_DECLS for kind, name in pattern.findall(code) if is_qt(kind)}
    pointers |= {name for name, kind in _AUTO_NEW_DECL.findall(code) if is_qt(kind)}
    objects = {name for kind, name in _QT_OBJECT_DECL.findall(code) if is_qt(kind)}
    return pointers, objects


# Qt classes whose constructors take the parent first, e.g. QTreeWidgetItem(QTreeWidgetItem* parent, QStringList).
_PARENT_FIRST = frozenset({"QTreeWidgetItem"})


def _constructed_with_parent(line: str, match: re.Match[str], names: tuple[set[str], set[str]],
                             continuation: str = "") -> bool:
    """Whether the constructor call names a parent; ``continuation`` holds the lines an unclosed call goes on to."""
    if match.group(0)[-1] != "(":
        return False
    line = f"{line} {continuation}" if continuation else line

    def is_parent(arg: str) -> bool:
        return (_PARENT_ARG.fullmatch(arg) is not None or arg in names[0]
                or (arg.startswith("&") and arg[1:].strip() in names[1]))

    new_type = _NEW_TYPE.match(line, match.start()).group(1).split("::")[-1]
    if new_type in _PARENT_FIRST:
        leading = _ctor_args(line, match.end() - 1, partial=True)
        if leading and is_parent(leading[0]):
            return True
    args = _ctor_args(line, match.end() - 1)
    return bool(args) and (is_parent(args[-1])
                           or any(_ANY_PARENT_ARG.fullmatch(arg) or arg in names[0] for arg in args))


def _statement_rest(lines: list[str], number: int, limit: int = 12) -> str:
    """The lines after ``lines[number]`` up to the end of the statement (the first ``;``), joined by spaces."""
    rest: list[str] = []
    for line in lines[number + 1:number + 1 + limit]:
        rest.append(line.strip())
        if ";" in line:
            break
    return " ".join(rest)


def _owning_call(line: str, start: int, new_type: str) -> bool:
    """Whether the ``new`` at ``start`` is an argument of an ownership-taking call (its innermost enclosing call)."""
    depth = 0
    for index in range(start - 1, -1, -1):
        char = line[index]
        if char in ")]}":
            depth += 1
        elif char in "[{":
            if depth == 0:
                continue  # braced/array initializer lists, e.g. appendColumn({new QStandardItem(x)})
            depth -= 1
        elif char == "(":
            if depth:
                depth -= 1
                continue
            name = _CALL_NAME.search(line, 0, index)
            if not name:
                return False
            call = name.group(1)
            return call in _OWNING_CALLS or (call == "addAction" and new_type.endswith("Action"))
    return False


def qt_derived_classes(texts: Any) -> set[str]:
    """Names of the classes that derive, directly or transitively, from a ``Q*`` class other than ``QSharedData``."""
    bases: dict[str, set[str]] = {}
    for text in texts:
        for match in _CLASS_DECL.finditer(text):
            names = {re.sub(r"<.*", "", base.split()[-1]).split("::")[-1] for base in match.group(2).split(",") if base.strip()}
            bases.setdefault(match.group(1), set()).update(names)
    derived: set[str] = set()
    grown = True
    while grown:
        grown = False
        for name, parents in bases.items():
            if name not in derived and any((re.match(r"Q[A-Z]", p) and p != "QSharedData") or p in derived for p in parents):
                derived.add(name)
                grown = True
    return derived


def strip_comments_and_strings(text: str) -> str:
    """Blank out comments and literals, keeping line structure and preprocessor lines."""
    out: list[str] = []
    index, length = 0, len(text)
    while index < length:
        char = text[index]
        pair = text[index:index + 2]
        if pair == "//":
            end = text.find("\n", index)
            index = length if end < 0 else end
        elif pair == "/*":
            end = text.find("*/", index + 2)
            end = length if end < 0 else end + 2
            out.append("".join("\n" if c == "\n" else " " for c in text[index:end]))
            index = end
        elif char == '"' and index > 0 and text[index - 1] == "R" and (index < 2 or not (text[index - 2].isalnum() or text[index - 2] == "_")):
            # C++11 raw string literal R"delim( ... )delim" -- may span lines and contain quotes
            open_paren = text.find("(", index)
            closing = ")" + text[index + 1:open_paren] + '"'
            end = text.find(closing, open_paren)
            end = length if end < 0 else end + len(closing)
            out.append(char + "".join("\n" if c == "\n" else " " for c in text[index + 1:end - 1]) + char)
            index = end
        elif char in "\"'":
            if char == "'" and index > 0 and text[index - 1].isalnum():
                out.append(char)  # C++14 digit separator
                index += 1
                continue
            end = index + 1
            while end < length and text[end] != char and text[end] != "\n":
                end += 2 if text[end] == "\\" else 1
            out.append(char + " " * max(end - index - 1, 0) + char)
            index = end + 1 if end < length and text[end] == char else end  # keep the newline of an unterminated literal
        else:
            out.append(char)
            index += 1
    return "".join(out)


def count_file(text: str, qt_classes: set[str] | frozenset[str] = frozenset(), header: str = "") -> dict[str, int]:
    """Counts one file; ``header`` is the file's own header, whose member declarations name parents too."""
    code = strip_comments_and_strings(text)
    counts = {name: len(pattern.findall(code)) for name, pattern in PATTERNS.items()}
    raw = qt = 0
    names = _qt_object_names(code + "\n" + strip_comments_and_strings(header), qt_classes)
    lines = code.splitlines()
    for number, line in enumerate(lines):
        for match in _ANY_NEW.finditer(line):
            bare = match.group(0)[-1] in "),"  # `new T)` without an argument list: only Qt types are tracked
            qualified = _NEW_TYPE.match(line, match.start()).group(1)
            new_type = qualified.split("::")[-1]
            # Designer's Ui::Name is a plain struct even though the widget class it fills shares its name.
            is_qt = bool(_QT_NEW.match(line, match.start())) or (new_type in qt_classes and not qualified.startswith("Ui::"))
            if _OWNED_NEW.search(line[:match.start()] + "("):
                continue
            if re.search(r"^\s*this\s*\)|[(,]\s*this\s*\)|\bparent\w*\s*\)", line[match.end():]):
                continue  # Qt parent-child ownership
            if is_qt:
                if _constructed_with_parent(line, match, names, _statement_rest(lines, number)):
                    continue
                if not _owning_call(line, match.start(), new_type):  # else handed straight to a Qt owner
                    qt += 1
            elif not bare:
                raw += 1
    counts["raw_new"] = raw
    counts["qt_new_unparented"] = qt
    return counts


def scan(repo_root: Path, policy: dict[str, Any]) -> dict[str, dict[str, int]]:
    extensions = set(policy["extensions"])
    excluded = tuple(policy.get("exclude_path_fragments", {}))
    texts: dict[str, str] = {}
    for root in policy["scan_roots"]:
        for path in sorted((repo_root / root).rglob("*")):
            if path.suffix in extensions and path.is_file():
                texts[path.relative_to(repo_root).as_posix()] = path.read_text(encoding="utf-8", errors="replace")
    # Class hierarchies come from every file, including excluded ones such as the Designer form classes.
    qt_classes = qt_derived_classes(strip_comments_and_strings(text) for text in texts.values())
    files: dict[str, dict[str, int]] = {}
    for relative, text in texts.items():
        if any(fragment in f"/{relative}" for fragment in excluded):
            continue
        header = texts.get(relative.rsplit(".", 1)[0] + ".h", "") if not relative.endswith(".h") else ""
        counts = count_file(text, qt_classes, header)
        counts["lines"] = text.count("\n") + 1
        files[relative] = counts
    return files


def build_report(repo_root: Path = REPO_ROOT, policy: dict[str, Any] | None = None) -> dict[str, Any]:
    policy = policy or json.loads(POLICY_FILE.read_text(encoding="utf-8"))
    suppressions = {k: v for k, v in policy.get("suppressions", {}).items() if not k.startswith("$")}
    files = scan(repo_root, policy)

    metrics: dict[str, Any] = {}
    for name in [*PATTERNS, "qt_new_unparented"]:
        suppressed = suppressions.get(name, {})
        per_file: dict[str, int] = {}
        for path, counts in files.items():
            if path in suppressed:
                continue
            value = counts[name]
            if value:
                per_file[path] = value
        by_scope: dict[str, int] = {}
        for path, value in per_file.items():
            scope = path.split("/")[1]
            by_scope[scope] = by_scope.get(scope, 0) + value
        metrics[name] = {
            "total": sum(per_file.values()),
            "files": len(per_file),
            "by_scope": dict(sorted(by_scope.items())),
            "suppressed": sorted(suppressed),
            "top": dict(sorted(per_file.items(), key=lambda item: (-item[1], item[0]))[:15]),
            "all_files": dict(sorted(per_file.items())),
        }
    return {
        "schema_version": 1,
        "generated_by": "scripts/quality/audit_maintainability.py",
        "policy": "scripts/quality/maintainability_policy.json",
        "scanned_files": len(files),
        "scanned_lines": sum(c["lines"] for c in files.values()),
        "metrics": metrics,
    }


def check(report: dict[str, Any], baseline: dict[str, Any]) -> list[str]:
    """The ratchet: no metric may grow, in total or in any single file."""
    failures: list[str] = []
    for name, metric in report["metrics"].items():
        previous = baseline.get("metrics", {}).get(name)
        if previous is None:
            continue
        if metric["total"] > previous["total"]:
            failures.append(f"{name}: {metric['total']} > baseline {previous['total']}")
        for path, value in metric["all_files"].items():
            if value > previous["all_files"].get(path, 0):
                failures.append(f"{name}: {path} has {value}, baseline {previous['all_files'].get(path, 0)}")
    return failures


def check_ceilings(report: dict[str, Any], policy: dict[str, Any]) -> list[str]:
    """Fail any metric above its committed ceiling; a ceiling of 0 means the debt is retired for good."""
    ceilings = {k: v for k, v in policy.get("ceilings", {}).items() if not k.startswith("$")}
    failures = [f"{name}: {report['metrics'][name]['total']} > ceiling {limit}"
                for name, limit in sorted(ceilings.items()) if report["metrics"][name]["total"] > limit]
    failures += [f"{name}: no ceiling" for name in report["metrics"] if name not in ceilings]
    return failures


def render_markdown(report: dict[str, Any]) -> str:
    lines = [
        "# MNE-CPP v2.4.0 maintainability baseline",
        "",
        "Generated by `scripts/quality/audit_maintainability.py`. Do not edit by hand.",
        "Counts are lexical candidates with comments and string literals removed; they rank work, they do not prove defects.",
        "",
        f"Scanned {report['scanned_files']:,} files / {report['scanned_lines']:,} lines in "
        "`src/libs/mne`, `src/apps`, `src/tools`. Compiler warnings: 0 (the build is `-Werror`).",
        "",
        "| Metric | Total | Files | libraries | applications | tools |",
        "|---|---:|---:|---:|---:|---:|",
    ]
    for name, metric in report["metrics"].items():
        scope = metric["by_scope"]
        lines.append(
            f"| `{name}` | {metric['total']:,} | {metric['files']:,} | {scope.get('libs', 0):,} "
            f"| {scope.get('apps', 0):,} | {scope.get('tools', 0):,} |"
        )
    for name, metric in report["metrics"].items():
        if not metric["top"]:
            continue
        lines += ["", f"## `{name}` — top files", "", "| count | File |", "|---:|---|"]
        lines += [f"| {value:,} | `{path}` |" for path, value in metric["top"].items()]
    return "\n".join(lines) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--json", type=Path, dest="json_path", help="write the JSON report")
    parser.add_argument("--markdown", type=Path, help="write the Markdown summary")
    parser.add_argument("--check", type=Path, metavar="BASELINE", help="fail if any metric grew beyond BASELINE")
    parser.add_argument("--ceilings", action="store_true", help="fail if any metric exceeds its policy ceiling")
    args = parser.parse_args(argv)

    report = build_report()
    if args.ceilings:
        failures = check_ceilings(report, json.loads(POLICY_FILE.read_text(encoding="utf-8")))
        for failure in failures:
            print(f"ERROR {failure}")
        print("OK: every maintainability metric is within its ceiling." if not failures else f"{len(failures)} over ceiling")
        return 1 if failures else 0
    if args.check:
        failures = check(report, json.loads(args.check.read_text(encoding="utf-8")))
        for failure in failures:
            print(f"ERROR {failure}")
        print("OK: no maintainability metric grew." if not failures else f"{len(failures)} regression(s)")
        return 1 if failures else 0
    if args.json_path:
        args.json_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if args.markdown:
        args.markdown.write_text(render_markdown(report), encoding="utf-8")
    if not args.json_path and not args.markdown:
        print(render_markdown(report), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
