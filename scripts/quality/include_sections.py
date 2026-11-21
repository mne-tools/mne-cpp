#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# @since    3.0.0
# @date     November 2026
#
# @brief    Check (or fix) that every include sits under the banner of its kind.
"""Include sections of the C/C++ sources.

The includes at the top of a file are grouped under banners, in this order::

    //=====================...
    // INCLUDES                 the file's own header, MNE-CPP and other headers
    //=====================...
    // QT INCLUDES              <QString>, <QtCore/...>
    // EIGEN INCLUDES           <Eigen/...>
    // STL INCLUDES             the C++ standard library

A section without includes has no banner. A preprocessor-conditional group of
includes moves as a whole: to STL INCLUDES if it holds only standard headers,
to INCLUDES otherwise. A file whose include block defines a macro is reported
but not rewritten, because the macro may have to come before a particular
include.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field

SECTIONS = ("INCLUDES", "QT INCLUDES", "EIGEN INCLUDES", "STL INCLUDES")

STL_HEADERS = frozenset(
    """algorithm any array atomic barrier bit bitset cassert cctype cerrno cfenv cfloat charconv chrono cinttypes
    climits clocale cmath codecvt compare complex concepts condition_variable coroutine csetjmp csignal cstdarg
    cstddef cstdint cstdio cstdlib cstring ctime cuchar cwchar cwctype deque exception execution expected filesystem
    flat_map flat_set format forward_list fstream functional future generator initializer_list iomanip ios iosfwd
    iostream istream iterator latch limits list locale map mdspan memory memory_resource mutex new numbers numeric
    optional ostream print queue random ranges ratio regex scoped_allocator semaphore set shared_mutex
    source_location span spanstream sstream stack stacktrace stdexcept stop_token streambuf string string_view
    syncstream system_error thread tuple type_traits typeindex typeinfo unordered_map unordered_set utility valarray
    variant vector version""".split()
)

_SEPARATOR = re.compile(r"^//={20,}\s*$")
_BANNER = re.compile(r"^//\s*([A-Z0-9 _/+-]*INCLUDES)\s*$")
_INCLUDE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]')
_IF = re.compile(r"^\s*#\s*if")
_ELSE = re.compile(r"^\s*#\s*(else|elif)")
_ENDIF = re.compile(r"^\s*#\s*endif")
_MACRO = re.compile(r"^\s*#\s*(define|undef|pragma)\b")

# Banners that older files use for the same sections
_ALIASES = {"STD INCLUDES": "STL INCLUDES", "C++ INCLUDES": "STL INCLUDES"}


def include_kind(delimiter: str, name: str) -> str:
    """The section an include belongs to."""
    if delimiter == "<" and name in STL_HEADERS:
        return "STL INCLUDES"
    if name.startswith(("Eigen/", "unsupported/Eigen/")):
        return "EIGEN INCLUDES"
    if delimiter == "<" and (re.fullmatch(r"Q[A-Z]\w*", name) or name.startswith(("Qt", "QtCore/", "QtGui/"))):
        return "QT INCLUDES"
    return "INCLUDES"


def section_of(banner: str) -> str:
    """The canonical section of a banner; MNE INCLUDES and the like are INCLUDES."""
    banner = _ALIASES.get(banner.strip(), banner.strip())
    return banner if banner in SECTIONS else "INCLUDES"


@dataclass
class Item:
    """One include, or one conditional group of includes, with the comment lines above it."""

    lines: list[str]
    kind: str
    first_line: int


@dataclass
class Block:
    """The include block at the top of a file."""

    start: int
    end: int  # exclusive
    items: list[Item] = field(default_factory=list)
    banners: list[tuple[int, str]] = field(default_factory=list)
    placed: list[tuple[Item, str]] = field(default_factory=list)  # item and the section it sits in
    has_macro: bool = False


def _is_other_banner(lines: list[str], i: int) -> bool:
    """Whether line @p i opens a banner that is not an include banner."""
    if not _SEPARATOR.match(lines[i].strip()) or i + 1 >= len(lines):
        return False
    following = lines[i + 1].strip()
    return following.startswith("//") and not _SEPARATOR.match(following) and not _BANNER.match(following)


def find_block(lines: list[str]) -> Block | None:
    """The include block: from the first include banner to the last include before code or another banner."""
    first = next((i for i, line in enumerate(lines) if _BANNER.match(line.strip())), None)
    if first is None:
        return None
    start = first - 1 if first > 0 and _SEPARATOR.match(lines[first - 1].strip()) else first
    block = Block(start=start, end=start)
    section = "INCLUDES"
    pending: list[str] = []
    i = start
    while i < len(lines):
        stripped = lines[i].strip()
        if _is_other_banner(lines, i):
            break
        banner = _BANNER.match(stripped)
        if banner:
            section = section_of(banner.group(1))
            block.banners.append((i, banner.group(1).strip()))
            i += 1
            continue
        if not stripped or _SEPARATOR.match(stripped):
            i += 1
            continue
        include = _INCLUDE.match(lines[i])
        if include:
            item = Item(pending + [lines[i]], include_kind(include.group(1), include.group(2)), i + 1)
            block.items.append(item)
            block.placed.append((item, section))
            pending = []
            block.end = i + 1
            i += 1
            continue
        if _IF.match(lines[i]):
            depth, j, kinds = 0, i, set()
            while j < len(lines):
                if _IF.match(lines[j]):
                    depth += 1
                elif _ENDIF.match(lines[j]):
                    depth -= 1
                    if depth == 0:
                        break
                inner = _INCLUDE.match(lines[j])
                if inner:
                    kinds.add(include_kind(inner.group(1), inner.group(2)))
                j += 1
            group = lines[i : j + 1]
            if not kinds:  # a conditional block without includes ends the include block
                break
            item = Item(pending + group, "STL INCLUDES" if kinds == {"STL INCLUDES"} else "INCLUDES", i + 1)
            block.items.append(item)
            block.placed.append((item, section))
            pending = []
            block.end = j + 1
            i = j + 1
            continue
        if _MACRO.match(lines[i]):
            block.has_macro = True
            pending.append(lines[i])
            i += 1
            continue
        if stripped.startswith("//"):
            pending.append(lines[i])
            i += 1
            continue
        break
    # Empty banners after the last include belong to the block too
    j = block.end
    while j < len(lines) and not _is_other_banner(lines, j):
        stripped = lines[j].strip()
        if stripped and not _SEPARATOR.match(stripped) and not _BANNER.match(stripped):
            break
        if _BANNER.match(stripped):
            block.end = j + 2 if j + 1 < len(lines) and _SEPARATOR.match(lines[j + 1].strip()) else j + 1
        j += 1
    return block if block.items else None


def check(text: str) -> list[tuple[int, str]]:
    """Findings (line, message) for includes outside their section, non-canonical banners and section order."""
    lines = text.splitlines()
    block = find_block(lines)
    if block is None:
        return []
    findings: list[tuple[int, str]] = []
    for line, banner in block.banners:
        if banner not in SECTIONS:
            findings.append((line + 1, f"banner '{banner}' is not one of {', '.join(SECTIONS)}"))
    order = [section_of(b) for _, b in block.banners]
    if order != sorted(order, key=SECTIONS.index) or len(set(order)) != len(order):
        findings.append((block.banners[0][0] + 1, f"sections are not in the order {' > '.join(SECTIONS)}, each once"))
    used = {section for _, section in block.placed}
    for line, banner in block.banners:
        if section_of(banner) not in used:
            findings.append((line + 1, f"banner '{banner}' has no includes"))
    for item, section in block.placed:
        if item.kind != section:
            name = _INCLUDE.search("\n".join(item.lines))
            what = f"<{name.group(2)}>" if name else "include"
            findings.append((item.first_line, f"{what} belongs under {item.kind}, not {section}"))
    return findings


def _is_quoted(item: Item) -> bool:
    include = _INCLUDE.match(item.lines[-1])
    return bool(include) and include.group(1) == '"'


def _banner(section: str) -> list[str]:
    rule = "//" + "=" * 109
    return [rule, f"// {section}", rule, ""]


def own_header(path: str) -> str | None:
    """The header a source file includes first: foo.h for foo.cpp."""
    stem, _, extension = path.rpartition(".")
    return f"{stem.rsplit('/', 1)[-1]}.h" if extension in ("c", "cc", "cpp", "cxx") else None


def fix(text: str, own_header: str | None = None) -> str | None:
    """The text with its include block regrouped, or None when it cannot be fixed safely."""
    lines = text.splitlines()
    block = find_block(lines)
    if block is None or block.has_macro:
        return None
    if not check(text):
        return text
    grouped: dict[str, list[Item]] = {section: [] for section in SECTIONS}
    seen: set[str] = set()
    for item in block.items:
        key = item.lines[-1].strip() if len(item.lines) == 1 else "\n".join(item.lines)
        if key in seen:  # the same include twice
            continue
        seen.add(key)
        grouped[item.kind].append(item)
    # The file's own header comes first, set apart by an empty line; then the headers next to the file ("...")
    # and then the others (<...>), each in their original order
    own = [item for item in grouped["INCLUDES"] if own_header and f'"{own_header}"' in item.lines[-1]]
    others = [item for item in grouped["INCLUDES"] if item not in own]
    rest = [item for item in others if _is_quoted(item)] + [item for item in others if not _is_quoted(item)]
    out: list[str] = []
    for section in SECTIONS:
        items = own + rest if section == "INCLUDES" else grouped[section]
        if not items:
            continue
        if out:
            out.append("")
        out += _banner(section)
        for item in items:
            out += item.lines
            if item in own and rest:
                out.append("")
    rest_of_file = lines[block.end :]
    while rest_of_file and not rest_of_file[0].strip():
        rest_of_file.pop(0)
    trailing = "\n" if text.endswith("\n") else ""
    return "\n".join(lines[: block.start] + out + ([""] + rest_of_file if rest_of_file else [])) + trailing
