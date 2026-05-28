#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""
doxy2mdx.py — Convert Doxygen XML output to Docusaurus MDX pages for
MNE-CPP's public API reference.

Ported from src/external/skigen/doc/doxygen2mdx.py and adapted to:
  * MNE-CPP's FIFFLIB::, MNELIB::, ... namespaced compound names.
  * Qt-specific Q_SIGNALS: / Q_SLOTS: sections.
  * SPDX-style author headers (TASK 17).
  * Math passthrough for KaTeX (remark-math) in the Docusaurus site.
  * Registry-driven coverage with optional user-guide and Python
    cross-reference admonitions.

CLI
---
    python3 tools/doxy2mdx/doxy2mdx.py \\
        --xml-dir   doc/xml_out/xml \\
        --out-dir   doc/website/docs/api \\
        --registry  doc/api_registry.json \\
        --generate-sidebars \\
        [--classes FIFFLIB::FiffInfo,...] \\
        [--sidebar-out doc/website/sidebars.api.generated.ts] \\
        [--strict]
"""

from __future__ import annotations

import argparse
import html
import json
import logging
import re
import sys
import textwrap
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

LOG = logging.getLogger("doxy2mdx")

# ---------------------------------------------------------------------------
# Registry state (loaded once, cached globally)
# ---------------------------------------------------------------------------

_REGISTRY: Optional[dict] = None
_MODULES: Dict[str, dict] = {}
# Files (qualified class names / module names) for which no @author /
# SPDX-style author line could be located.  Every source file in the
# repository is required to carry at least one author, so we abort
# generation if this list is non-empty at the end of the run.
_FILES_WITHOUT_AUTHOR: List[str] = []
_CLASS_INDEX: Dict[str, List[dict]] = {}   # short-name -> list of entries

# Namespaces we never document, even if they appear in the XML.
_EXCLUDED_NAMESPACES = {
    "std", "Eigen", "boost", "qt", "Qt",
    "QtPrivate", "ANONYMOUS", "anonymous_namespace",
}


def _load_registry(registry_path: Path) -> None:
    """Load the registry from JSON.  Called once; cached globally."""
    global _REGISTRY, _MODULES, _CLASS_INDEX
    with open(registry_path, encoding="utf-8") as f:
        _REGISTRY = json.load(f)
    _MODULES = _REGISTRY["modules"]
    _CLASS_INDEX.clear()
    for entry in _REGISTRY["classes"]:
        _CLASS_INDEX.setdefault(entry["name"], []).append(entry)


def module_for(qualified: str) -> Optional[dict]:
    """Resolve a qualified class name (``FIFFLIB::FiffInfo``) to a
    registry entry.  When the short-name has multiple matches we
    disambiguate by lowercased-namespace ↔ ``module`` key.
    Returns ``None`` if no match is registered.
    """
    parts = qualified.split("::")
    short = parts[-1]
    candidates = _CLASS_INDEX.get(short, [])
    if not candidates:
        return None
    if len(candidates) == 1:
        return candidates[0]
    if len(parts) >= 2:
        ns_lower = parts[-2].lower()
        # FIFFLIB -> "fiff"; strip trailing "lib"
        ns_key = ns_lower[:-3] if ns_lower.endswith("lib") else ns_lower
        for c in candidates:
            if c.get("module") == ns_key or c.get("module") == ns_lower:
                return c
    return candidates[0]


# ---------------------------------------------------------------------------
# URL slug helpers
# ---------------------------------------------------------------------------

def class_slug(qualified: str) -> str:
    """``FIFFLIB::FiffInfo`` -> ``fiff-info`` (kebab-case of leaf)."""
    short = qualified.split("::")[-1]
    slug = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "-", short)
    slug = re.sub(r"(?<=[A-Z])(?=[A-Z][a-z])", "-", slug)
    return slug.lower()


# ---------------------------------------------------------------------------
# SPDX author scraping (TASK 17 source files)
# ---------------------------------------------------------------------------

_SPDX_AUTHOR_RE = re.compile(
    r"^\s*[*/#]*\s*([A-ZÄÖÜ][\w\-.' ]+?)\s*<([^>]+@[^>]+)>\s*$"
)
_SPDX_HEADER_RE = re.compile(r"SPDX-License-Identifier", re.IGNORECASE)


def read_spdx_authors(header_path: Path) -> List[Tuple[str, str]]:
    """Scrape the first 30 lines of *header_path* for SPDX-style author
    lines (``Name <email>``).  Returns ``[(name, email), ...]`` in file
    order.  Returns ``[]`` when the file is missing or has no SPDX block.
    """
    if not header_path or not header_path.exists():
        return []
    authors: List[Tuple[str, str]] = []
    saw_spdx = False
    try:
        with open(header_path, encoding="utf-8", errors="replace") as f:
            for i, line in enumerate(f):
                if i >= 30:
                    break
                if _SPDX_HEADER_RE.search(line):
                    saw_spdx = True
                    continue
                m = _SPDX_AUTHOR_RE.match(line)
                if m:
                    authors.append((m.group(1).strip(), m.group(2).strip()))
    except OSError:
        return []
    return authors if saw_spdx else []


# ---------------------------------------------------------------------------
# XML text extraction
# ---------------------------------------------------------------------------

def text_of(el) -> str:
    if el is None:
        return ""
    parts: List[str] = []
    if el.text:
        parts.append(el.text)
    for child in el:
        tag = child.tag
        if tag == "computeroutput":
            parts.append(f"`{text_of(child)}`")
        elif tag == "bold":
            parts.append(f"**{text_of(child)}**")
        elif tag == "emphasis":
            parts.append(f"*{text_of(child)}*")
        elif tag == "ref":
            parts.append(f"`{text_of(child)}`")
        elif tag == "ulink":
            url = child.get("url", "")
            parts.append(f"[{text_of(child)}]({url})")
        elif tag == "formula":
            formula = text_of(child).strip()
            # Doxygen: @f[...]@f] => display math; @f$...@f$ => inline.
            # The XML emission uses \[ \] / $ $ / \( \).
            if formula.startswith("\\[") and formula.endswith("\\]"):
                parts.append(f"\n\n$$\n{formula[2:-2].strip()}\n$$\n\n")
            elif formula.startswith("\\(") and formula.endswith("\\)"):
                parts.append(f"${formula[2:-2].strip()}$")
            elif formula.startswith("$") and formula.endswith("$"):
                parts.append(formula)
            else:
                parts.append(f"${formula}$")
        elif tag == "simplesect":
            kind = child.get("kind", "")
            body = text_of(child).strip()
            if kind == "return":
                parts.append(f"\n\n**Returns:** {body}\n\n")
            elif kind == "note":
                parts.append(f"\n\n:::note\n{body}\n:::\n\n")
            elif kind == "see":
                parts.append(f"\n\n**See also:** {body}\n\n")
            elif kind == "warning":
                parts.append(f"\n\n:::warning\n{body}\n:::\n\n")
            else:
                parts.append(body)
        elif tag == "parameterlist":
            pass  # handled separately
        elif tag == "para":
            parts.append(text_of(child))
            parts.append("\n\n")
        elif tag in ("sect1", "sect2", "sect3"):
            parts.append(text_of(child))
        elif tag == "title":
            depth = {"sect1": "##", "sect2": "###", "sect3": "####"}.get(el.tag, "##")
            parts.append(f"\n\n{depth} {text_of(child)}\n\n")
        elif tag == "itemizedlist":
            for item in child.findall("listitem"):
                parts.append(f"- {text_of(item).strip()}\n")
            parts.append("\n")
        elif tag == "table":
            parts.append(render_table(child))
        elif tag == "programlisting":
            parts.append(render_code_block(child))
        elif tag == "sp":
            parts.append(" ")
        else:
            parts.append(text_of(child))
        if child.tail:
            parts.append(child.tail)
    return "".join(parts)


def render_table(table_el) -> str:
    rows = table_el.findall("row")
    if not rows:
        return ""
    lines: List[str] = []
    for i, row in enumerate(rows):
        cells = [text_of(e).strip().replace("\n", " ") for e in row.findall("entry")]
        lines.append("| " + " | ".join(cells) + " |")
        if i == 0:
            lines.append("|" + "|".join(["---"] * len(cells)) + "|")
    return "\n" + "\n".join(lines) + "\n\n"


def render_code_block(listing_el) -> str:
    lines: List[str] = []
    for codeline in listing_el.findall("codeline"):
        parts: List[str] = []
        for hl in codeline:
            t = hl.text or ""
            for sub in hl:
                if sub.tag == "sp":
                    t += " "
                else:
                    t += sub.text or ""
                if sub.tail:
                    t += sub.tail
            parts.append(t)
            if hl.tail:
                parts.append(hl.tail)
        lines.append("".join(parts))
    return f"\n```cpp\n" + "\n".join(lines) + "\n```\n\n"


# ---------------------------------------------------------------------------
# C++ type beautifier
# ---------------------------------------------------------------------------

_EIGEN_MATRIX_RE = re.compile(
    r"Eigen::Matrix\s*<\s*(?P<scalar>[\w:]+)\s*,\s*"
    r"(?P<rows>[\w\-]+)\s*,\s*(?P<cols>[\w\-]+)"
    r"(?:\s*,[^<>]*)?\s*>"
)

_SCALAR_SHORT = {
    "double": "d", "float": "f", "int": "i",
    "std::complex<double>": "cd", "std::complex<float>": "cf",
}

_DIM_TOKEN = {"-1": "X", "Dynamic": "X", "1": "1", "2": "2", "3": "3", "4": "4"}


def _shorten_eigen_matrix(m: re.Match) -> str:
    scalar = m.group("scalar")
    rows = _DIM_TOKEN.get(m.group("rows"), m.group("rows"))
    cols = _DIM_TOKEN.get(m.group("cols"), m.group("cols"))
    short_scalar = _SCALAR_SHORT.get(scalar)
    if short_scalar is None:
        return m.group(0)
    if rows == "X" and cols == "X":
        return f"Eigen::MatrixX{short_scalar}"
    if cols == "1":
        if rows == "X":
            return f"Eigen::VectorX{short_scalar}"
        return f"Eigen::Vector{rows}{short_scalar}"
    if rows == "1":
        if cols == "X":
            return f"Eigen::RowVectorX{short_scalar}"
        return f"Eigen::RowVector{cols}{short_scalar}"
    if rows == cols and rows in {"2", "3", "4"}:
        return f"Eigen::Matrix{rows}{short_scalar}"
    return f"Eigen::Matrix<{scalar}, {rows}, {cols}>"


def beautify_type(t: str) -> str:
    """Collapse noisy template forms into something readable."""
    if not t:
        return ""
    t = t.replace("`", "").strip()
    # Strip class/struct keyword prefixes
    t = re.sub(r"\b(class|struct|typename|enum)\s+", "", t)
    # Strip MNE-CPP *_EXPORT macro suffixes ("FIFFSHARED_EXPORT FiffInfo")
    t = re.sub(r"\b\w+SHARED_EXPORT\s+", "", t)
    t = re.sub(r"\b\w+_EXPORT\b", "", t)
    # Collapse Eigen::Matrix<...> down to short forms (best-effort)
    for _ in range(3):
        new_t = _EIGEN_MATRIX_RE.sub(_shorten_eigen_matrix, t)
        if new_t == t:
            break
        t = new_t
    # Normalise spaces inside QSharedPointer<X>
    t = re.sub(r"QSharedPointer\s*<\s*([^<>]+?)\s*>", r"QSharedPointer<\1>", t)
    t = re.sub(r"QSharedPointer<\s*([^<>]+?)\s*>", r"QSharedPointer<\1>", t)
    # Common Eigen::Ref unwrapping
    t = re.sub(r"const\s+Eigen::Ref<\s*const\s+([^<>]+?)\s*>\s*&", r"\1", t)
    t = re.sub(r"Eigen::Ref<\s*const\s+([^<>]+?)\s*>\s*&", r"\1", t)
    t = re.sub(r"Eigen::Ref<\s*([^<>]+?)\s*>\s*&", r"\1", t)
    # Collapse repeated whitespace
    t = re.sub(r"\s{2,}", " ", t).strip()
    return t


# ---------------------------------------------------------------------------
# Param / return / note extraction
# ---------------------------------------------------------------------------

def extract_params(detail_el) -> List[Tuple[str, str]]:
    out: List[Tuple[str, str]] = []
    if detail_el is None:
        return out
    for plist in detail_el.findall(".//parameterlist[@kind='param']"):
        for item in plist.findall("parameteritem"):
            name_el = item.find(".//parametername")
            desc_el = item.find("parameterdescription")
            name = (text_of(name_el).strip() if name_el is not None else "").strip("`")
            desc = re.sub(r"\s*\n\s*", " ",
                          text_of(desc_el).strip() if desc_el is not None else "")
            out.append((name, desc))
    return out


def extract_return(detail_el) -> str:
    if detail_el is None:
        return ""
    for ss in detail_el.findall(".//simplesect[@kind='return']"):
        return text_of(ss).strip()
    return ""


def extract_notes(detail_el) -> List[str]:
    if detail_el is None:
        return []
    return [text_of(ss).strip()
            for ss in detail_el.findall(".//simplesect[@kind='note']")]


def extract_doxygen_authors(detail_el) -> List[str]:
    """Fallback when no SPDX block exists: pull plain ``@author`` lines."""
    if detail_el is None:
        return []
    out: List[str] = []
    for ss in detail_el.findall(".//simplesect[@kind='author']"):
        a = text_of(ss).strip()
        if a:
            out.append(a)
    return out


def extract_body_text(detail_el) -> str:
    if detail_el is None:
        return ""
    parts: List[str] = []
    for child in detail_el:
        if child.tag != "para":
            continue
        text_parts: List[str] = []
        if child.text:
            text_parts.append(child.text)
        has_content = bool(child.text and child.text.strip())
        for sub in child:
            if sub.tag in ("parameterlist", "simplesect"):
                continue
            has_content = True
            if sub.tag == "computeroutput":
                text_parts.append(f"`{text_of(sub)}`")
            elif sub.tag == "bold":
                text_parts.append(f"**{text_of(sub)}**")
            elif sub.tag == "emphasis":
                text_parts.append(f"*{text_of(sub)}*")
            elif sub.tag == "formula":
                f = text_of(sub).strip()
                if f.startswith("\\[") and f.endswith("\\]"):
                    text_parts.append(f"\n\n$$\n{f[2:-2].strip()}\n$$\n\n")
                elif f.startswith("\\(") and f.endswith("\\)"):
                    text_parts.append(f"${f[2:-2].strip()}$")
                else:
                    text_parts.append(f"${f}$")
            elif sub.tag == "ref":
                text_parts.append(f"`{text_of(sub)}`")
            elif sub.tag == "ulink":
                text_parts.append(f"[{text_of(sub)}]({sub.get('url','')})")
            elif sub.tag == "programlisting":
                text_parts.append(render_code_block(sub))
            else:
                text_parts.append(text_of(sub))
            if sub.tail:
                text_parts.append(sub.tail)
        if has_content:
            body = "".join(text_parts).strip()
            if body:
                parts.append(body)
    return "\n\n".join(parts)


# ---------------------------------------------------------------------------
# Section iteration: Qt slots/signals + public methods
# ---------------------------------------------------------------------------

def _public_members(section) -> List:
    return [m for m in section.findall("memberdef[@kind='function']")
            if m.get("prot") == "public"]


# ---------------------------------------------------------------------------
# Method rendering
# ---------------------------------------------------------------------------

def _render_method(func, lines: List[str]) -> None:
    name = func.findtext("name", "")
    param_names = [p.findtext("declname", "") for p in func.findall("param")]
    param_names = [p for p in param_names if p]
    heading = f"{name}({', '.join(param_names)})"
    lines.append(f"### {heading}")
    lines.append("")

    brief = text_of(func.find("briefdescription")).strip()
    detail_el = func.find("detaileddescription")
    body = extract_body_text(detail_el)
    if brief:
        lines.append(brief)
        lines.append("")
    if body:
        lines.append(body)
        lines.append("")

    xml_params: Dict[str, str] = {}
    for p in func.findall("param"):
        pname = p.findtext("declname", "")
        ptype = text_of(p.find("type")).strip()
        xml_params[pname] = ptype

    params = extract_params(detail_el)
    if params:
        lines.append("**Parameters:**")
        lines.append("")
        for pname, pdesc in params:
            display_type = beautify_type(xml_params.get(pname, ""))
            if display_type:
                lines.append(f"- **{pname}** : *{display_type}*")
            else:
                lines.append(f"- **{pname}**")
            if pdesc:
                lines.append(f"  {pdesc}")
            lines.append("")

    ret = extract_return(detail_el)
    if ret:
        display_ret = beautify_type(text_of(func.find("type")).strip())
        lines.append("**Returns:**")
        lines.append("")
        if display_ret:
            lines.append(f"- *{display_ret}* — {ret}")
        else:
            lines.append(f"- {ret}")
        lines.append("")

    for note in extract_notes(detail_el):
        lines.append(f":::note\n{note}\n:::")
        lines.append("")

    lines.append("---")
    lines.append("")


# ---------------------------------------------------------------------------
# MDX generation per class
# ---------------------------------------------------------------------------

def _escape_mdx_bare_lt(content: str) -> str:
    """HTML-escape every ``<`` / ``>`` and backslash-escape every
    ``{`` / ``}`` that appears outside fenced code blocks and inline code
    spans.  We never emit raw JSX/HTML tags or MDX expressions from
    Doxygen content, so this prevents MDX from misreading C++ snippets
    such as ``std::pair<X, Y>`` or prose like ``{0 -> lh, 1 -> rh}`` as
    JSX/JS."""
    out: List[str] = []
    i, n = 0, len(content)
    while i < n:
        if content.startswith("```", i):
            end = content.find("```", i + 3)
            end = n if end == -1 else end + 3
            out.append(content[i:end])
            i = end
            continue
        if content[i] == "`":
            end = content.find("`", i + 1)
            end = n if end == -1 else end + 1
            out.append(content[i:end])
            i = end
            continue
        ch = content[i]
        if ch == "<":
            out.append("&lt;")
        elif ch == ">":
            out.append("&gt;")
        elif ch == "{":
            out.append("\\{")
        elif ch == "}":
            out.append("\\}")
        else:
            out.append(ch)
        i += 1
    return "".join(out)


def _render_example_section(reg_entry: dict,
                            repo_root: Path,
                            lines: List[str]) -> None:
    """Append an ``## Example`` section sourced from
    ``src/examples/<example>/main.cpp`` when the registry entry has a
    non-null ``example`` field.  Always emits the section header so the
    right-hand table of contents stays consistent across pages."""
    example = reg_entry.get("example")
    if not example:
        return
    ex_dir = repo_root / "src" / "examples" / example
    main_cpp = ex_dir / "main.cpp"
    gh_url = (f"https://github.com/mne-tools/mne-cpp/tree/staging/"
              f"src/examples/{example}")
    lines.append("## Example")
    lines.append("")
    if not main_cpp.exists():
        lines.append(f":::warning[Example `{example}` not found]")
        lines.append(f"The registry references `src/examples/{example}/` but no")
        lines.append("`main.cpp` was found at that path.")
        lines.append(":::")
        lines.append("")
        return
    try:
        body = main_cpp.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return
    # Strip the SPDX/license header block (everything up to and
    # including the first blank line after the leading comment block).
    snippet = _strip_leading_comment(body)
    lines.append(f"Source: [`src/examples/{example}/main.cpp`]({gh_url}/main.cpp)")
    lines.append("")
    lines.append("```cpp")
    lines.append(snippet.rstrip())
    lines.append("```")
    lines.append("")


def _strip_leading_comment(src: str) -> str:
    """Remove leading SPDX / license / banner comments from a C++ source
    file so the inlined example focuses on the runnable code. Loops over
    contiguous blank lines, ``//`` line comments and ``/* ... */`` block
    comments at the very top of the file.

    Stops when a non-empty, non-comment line is reached or when an
    ``#include`` directive appears (whichever comes first).
    """
    lines = src.splitlines()
    i, n = 0, len(lines)
    while i < n:
        stripped = lines[i].lstrip()
        if not stripped:
            i += 1
            continue
        if stripped.startswith("//"):
            i += 1
            continue
        if stripped.startswith("/*"):
            # Consume the whole block comment.
            while i < n and "*/" not in lines[i]:
                i += 1
            i += 1  # past the closing */
            continue
        # First real code line.
        break
    return "\n".join(lines[i:])


def generate_class_mdx(compounddef,
                       compound_name: str,
                       out_dir: Path,
                       reg_entry: dict,
                       repo_root: Path) -> Path:
    short_name = compound_name.split("::")[-1]
    brief = text_of(compounddef.find("briefdescription")).strip()
    detail_el = compounddef.find("detaileddescription")
    body_text = extract_body_text(detail_el) if detail_el is not None else ""

    # Resolve source path for SPDX scraping.  Doxygen is configured
    # with ``INPUT = src/libraries`` (relative to ``doc/``), so the
    # ``<location file=...>`` values are relative to ``src/libraries``,
    # e.g. ``fiff/fiff_id.h``.
    loc_el = compounddef.find("location")
    loc_file = loc_el.get("file", "") if loc_el is not None else ""
    src_path: Optional[Path] = None
    if loc_file:
        candidate = Path(loc_file)
        if candidate.is_absolute():
            src_path = candidate
        else:
            src_path = repo_root / "src" / "libraries" / loc_file
    authors = read_spdx_authors(src_path) if src_path else []
    if not authors:
        for a in extract_doxygen_authors(detail_el):
            authors.append((a, ""))
    if not authors:
        _FILES_WITHOUT_AUTHOR.append(
            f"{compound_name}  ({loc_file or 'unknown location'})"
        )

    # Module info & output folder
    module_key = reg_entry.get("module", "core")
    mod = _MODULES.get(module_key, {})
    dir_slug = mod.get("dir_slug", module_key)
    out_path = out_dir / dir_slug
    out_path.mkdir(parents=True, exist_ok=True)

    slug = class_slug(compound_name)
    out_file = out_path / f"{slug}.mdx"

    sidebar_position = reg_entry.get("sidebar_position")
    guide = reg_entry.get("guide")
    python_equiv = reg_entry.get("python_equiv")
    python_url = reg_entry.get("python_url")
    include_dir = mod.get("include", module_key)

    # ---- assemble ----
    lines: List[str] = []
    lines.append("---")
    lines.append(f"id: {slug}")
    lines.append(f'title: "{compound_name}"')
    lines.append(f"sidebar_label: {short_name}")
    if sidebar_position is not None:
        lines.append(f"sidebar_position: {sidebar_position}")
    lines.append("---")
    lines.append("")
    lines.append(f"# {short_name}")
    lines.append("")

    # Prominent namespace / library banner so every page tells the
    # reader where the symbol lives without scrolling.  The header path
    # is shown by the ``#include`` line a few rows below, so we don't
    # duplicate it here.
    ns = mod.get("namespace") or compound_name.split("::", 1)[0]
    mod_label = mod.get("sidebar_label", module_key)
    lines.append(
        f"**Namespace:** `{ns}` &nbsp;·&nbsp; **Library:** {mod_label}"
    )
    lines.append("")

    if guide:
        lines.append(":::tip[See also]")
        lines.append(f"User guide: [{short_name} guide]({guide})")
        lines.append(":::")
        lines.append("")
    if python_equiv:
        if python_url:
            lines.append(":::info[Python equivalent]")
            lines.append(f"[`{python_equiv}`]({python_url}) in MNE-Python.")
            lines.append(":::")
        else:
            lines.append(":::info[Python equivalent]")
            lines.append(f"`{python_equiv}` in MNE-Python.")
            lines.append(":::")
        lines.append("")

    lines.append(f"`#include <{include_dir}/{Path(loc_file).name}>`"
                 if loc_file else f"`#include <{include_dir}>`")
    lines.append("")

    # Class signature (with template parameters)
    tparams: List[Tuple[str, str]] = []
    for tp in compounddef.findall("templateparamlist/param"):
        tp_type = text_of(tp.find("type")).strip()
        tp_defval = text_of(tp.find("defval")).strip()
        tparams.append((tp_type, tp_defval))

    lines.append("```cpp")
    if tparams:
        tps = [f"{a} = {b}" if b else a for a, b in tparams]
        lines.append(f"template <{', '.join(tps)}>")
    lines.append(f"class {compound_name}")
    lines.append("```")
    lines.append("")

    if brief:
        lines.append(brief)
        lines.append("")
    if body_text:
        lines.append(body_text)
        lines.append("")
    lines.append("---")
    lines.append("")

    # --- Q_SIGNALS first (Qt convention: read top-down) ---
    signal_sections = compounddef.findall("sectiondef[@kind='signal']")
    signals = [m for sec in signal_sections for m in _public_members(sec)]
    if signals:
        lines.append("## Signals")
        lines.append("")
        for f in signals:
            _render_method(f, lines)

    # --- Public slots ---
    pub_slots = []
    for sec in compounddef.findall("sectiondef[@kind='public-slot']"):
        pub_slots.extend(_public_members(sec))
    if pub_slots:
        lines.append("## Public Slots")
        lines.append("")
        for f in pub_slots:
            _render_method(f, lines)

    # --- Generic public methods ---
    public_funcs: List = []
    for sec in compounddef.findall("sectiondef[@kind='public-func']"):
        public_funcs.extend(_public_members(sec))
    if public_funcs:
        lines.append("## Public Methods")
        lines.append("")
        for f in public_funcs:
            _render_method(f, lines)

    # --- Static methods ---
    static_funcs: List = []
    for sec in compounddef.findall("sectiondef[@kind='public-static-func']"):
        static_funcs.extend(_public_members(sec))
    if static_funcs:
        lines.append("## Static Methods")
        lines.append("")
        for f in static_funcs:
            _render_method(f, lines)

    # --- Example (sourced from src/examples/<ex_id>/main.cpp) ---
    _render_example_section(reg_entry, repo_root, lines)

    # --- Authors footer (every source file must list its authors;
    #     the generator aborts at the end if any were missed). ---
    lines.append("## Authors of this file")
    lines.append("")
    if authors:
        for name, email in authors:
            if email:
                lines.append(f"- {name} &lt;{email}&gt;")
            else:
                lines.append(f"- {name}")
    lines.append("")

    content = "\n".join(lines)
    content = re.sub(r"\n{4,}", "\n\n\n", content)
    content = _escape_mdx_bare_lt(content)
    out_file.write_text(content, encoding="utf-8")
    LOG.info("generated %s", out_file)
    return out_file


# ---------------------------------------------------------------------------
# Module (header-as-namespace) MDX generation
# ---------------------------------------------------------------------------

def _render_free_function(func, lines: List[str]) -> None:
    """Same as _render_method but uses a fully-qualified call signature."""
    name = func.findtext("name", "")
    param_names = [p.findtext("declname", "") for p in func.findall("param")]
    param_names = [p for p in param_names if p]
    heading = f"{name}({', '.join(param_names)})"
    lines.append(f"### {heading}")
    lines.append("")

    ret_type = beautify_type(text_of(func.find("type")).strip())
    if ret_type:
        param_types = [beautify_type(text_of(p.find("type")).strip())
                       for p in func.findall("param")]
        zipped = ", ".join(
            f"{pt} {pn}".strip() for pt, pn in zip(param_types, param_names)
        )
        lines.append("```cpp")
        lines.append(f"{ret_type} {name}({zipped});")
        lines.append("```")
        lines.append("")

    brief = text_of(func.find("briefdescription")).strip()
    detail_el = func.find("detaileddescription")
    body = extract_body_text(detail_el)
    if brief:
        lines.append(brief)
        lines.append("")
    if body:
        lines.append(body)
        lines.append("")

    xml_params: Dict[str, str] = {}
    for p in func.findall("param"):
        pname = p.findtext("declname", "")
        ptype = text_of(p.find("type")).strip()
        xml_params[pname] = ptype

    params = extract_params(detail_el)
    if params:
        lines.append("**Parameters:**")
        lines.append("")
        for pname, pdesc in params:
            display_type = beautify_type(xml_params.get(pname, ""))
            if display_type:
                lines.append(f"- **{pname}** : *{display_type}*")
            else:
                lines.append(f"- **{pname}**")
            if pdesc:
                lines.append(f"  {pdesc}")
            lines.append("")

    ret = extract_return(detail_el)
    if ret:
        lines.append("**Returns:**")
        lines.append("")
        if ret_type:
            lines.append(f"- *{ret_type}* — {ret}")
        else:
            lines.append(f"- {ret}")
        lines.append("")

    for note in extract_notes(detail_el):
        lines.append(f":::note\n{note}\n:::")
        lines.append("")

    lines.append("---")
    lines.append("")


def _collect_module_functions(xml_dir: Path,
                              file_compounddef,
                              header_rel: str) -> List:
    """Walk every innernamespace referenced by *file_compounddef* and
    return the public free functions whose ``<location file>`` matches
    *header_rel* (e.g. ``dsp/sphara.h``)."""
    out: List = []
    seen_ids = set()
    for ns_ref in file_compounddef.findall("innernamespace"):
        refid = ns_ref.get("refid")
        if not refid:
            continue
        ns_xml = xml_dir / f"{refid}.xml"
        if not ns_xml.exists():
            continue
        ns_tree = ET.parse(ns_xml)
        ns_def = ns_tree.find(".//compounddef")
        if ns_def is None:
            continue
        ns_name = ns_def.findtext("compoundname", "")
        if ns_name.split("::")[0] in _EXCLUDED_NAMESPACES:
            continue
        for sec in ns_def.findall("sectiondef"):
            for m in sec.findall("memberdef[@kind='function']"):
                if m.get("prot") != "public":
                    continue
                loc = m.find("location")
                if loc is None:
                    continue
                if loc.get("file", "") != header_rel:
                    continue
                mid = m.get("id")
                if mid in seen_ids:
                    continue
                seen_ids.add(mid)
                out.append(m)
    return out


def generate_module_mdx(xml_dir: Path,
                        file_compounddef,
                        reg_entry: dict,
                        out_dir: Path,
                        repo_root: Path) -> Path:
    """Render a header-level MDX page for a free-function module."""
    short_name = reg_entry["name"]
    header_rel = reg_entry["header"]
    brief = text_of(file_compounddef.find("briefdescription")).strip()
    detail_el = file_compounddef.find("detaileddescription")
    body_text = extract_body_text(detail_el) if detail_el is not None else ""

    src_path = repo_root / "src" / "libraries" / header_rel
    authors = read_spdx_authors(src_path)
    if not authors:
        for a in extract_doxygen_authors(detail_el):
            authors.append((a, ""))
    if not authors:
        _FILES_WITHOUT_AUTHOR.append(f"{short_name}  ({header_rel})")

    module_key = reg_entry.get("module", "core")
    mod = _MODULES.get(module_key, {})
    dir_slug = mod.get("dir_slug", module_key)
    out_path = out_dir / dir_slug
    out_path.mkdir(parents=True, exist_ok=True)
    slug = class_slug(short_name)
    out_file = out_path / f"{slug}.mdx"

    sidebar_position = reg_entry.get("sidebar_position")
    guide = reg_entry.get("guide")
    python_equiv = reg_entry.get("python_equiv")
    python_url = reg_entry.get("python_url")
    include_dir = mod.get("include", module_key)

    funcs = _collect_module_functions(xml_dir, file_compounddef, header_rel)

    lines: List[str] = []
    lines.append("---")
    lines.append(f"id: {slug}")
    lines.append(f'title: "{short_name}"')
    lines.append(f"sidebar_label: {short_name}")
    if sidebar_position is not None:
        lines.append(f"sidebar_position: {sidebar_position}")
    lines.append("---")
    lines.append("")
    lines.append(f"# {short_name}")
    lines.append("")
    ns = mod.get("namespace") or module_key.upper() + "LIB"
    mod_label = mod.get("sidebar_label", module_key)
    lines.append(
        f"**Namespace:** `{ns}` &nbsp;·&nbsp; **Library:** {mod_label}"
    )
    lines.append("")
    lines.append(":::info[Module]")
    lines.append("This page documents a **header-level module** — a collection of free")
    lines.append("functions that share an algorithmic topic. There is no enclosing C++")
    lines.append("class; the functions live directly in the library namespace.")
    lines.append(":::")
    lines.append("")
    if guide:
        lines.append(":::tip[See also]")
        lines.append(f"User guide: [{short_name} guide]({guide})")
        lines.append(":::")
        lines.append("")
    if python_equiv:
        lines.append(":::info[Python equivalent]")
        if python_url:
            lines.append(f"[`{python_equiv}`]({python_url}) in MNE-Python.")
        else:
            lines.append(f"`{python_equiv}` in MNE-Python.")
        lines.append(":::")
        lines.append("")

    lines.append(f"`#include <{include_dir}/{Path(header_rel).name}>`")
    lines.append("")

    if brief:
        lines.append(brief)
        lines.append("")
    if body_text:
        lines.append(body_text)
        lines.append("")
    lines.append("---")
    lines.append("")

    if funcs:
        lines.append("## Functions")
        lines.append("")
        for f in funcs:
            _render_free_function(f, lines)
    else:
        lines.append(":::warning[No public functions found]")
        lines.append("Doxygen did not report any public free functions for this")
        lines.append("header. Verify the header is parsed and the functions have")
        lines.append("`/** ... */` blocks.")
        lines.append(":::")
        lines.append("")

    # --- Example (sourced from src/examples/<ex_id>/main.cpp) ---
    _render_example_section(reg_entry, repo_root, lines)

    # --- Authors footer (every source file must list its authors). ---
    lines.append("## Authors of this file")
    lines.append("")
    if authors:
        for name, email in authors:
            if email:
                lines.append(f"- {name} &lt;{email}&gt;")
            else:
                lines.append(f"- {name}")
    lines.append("")

    content = "\n".join(lines)
    content = re.sub(r"\n{4,}", "\n\n\n", content)
    content = _escape_mdx_bare_lt(content)
    out_file.write_text(content, encoding="utf-8")
    LOG.info("generated %s", out_file)
    return out_file


# ---------------------------------------------------------------------------
# Sidebar fragment
# ---------------------------------------------------------------------------

def _short_module_label(mod: dict, fallback: str) -> str:
    """Return the human-readable sidebar label for a module. The label
    is taken verbatim from the registry (e.g. ``"FIFF Library"``); the
    namespace (``FIFFLIB``) is exposed separately in the page header so
    the sidebar can read like ``FIFF Library > FiffStream``."""
    return mod.get("sidebar_label", fallback)


# Mapping from module key -> source file under doc/website/docs/development/
# that documents that library at a conceptual level (architecture diagram,
# class inventory, MNE-Python / MNE-C cross-reference table).  When present,
# the contents of that file are embedded as the library's API landing page
# (``docs/api/<module>/index.mdx``) and the sidebar category links to it.
_LIBRARY_OVERVIEW_DEV_FILES: Dict[str, str] = {
    "fiff": "api-fiff.mdx",
    "mne": "api-mne.mdx",
    "fwd": "api-fwd.mdx",
    "inv": "api-inverse.mdx",
    "dsp": "api-dsp.mdx",
    "conn": "api-connectivity.mdx",
    "ml": "api-ml.mdx",
    "sts": "api-sts.mdx",
    "mna": "api-mna.mdx",
    "disp3D": "api-disp3d.mdx",
}


def generate_library_overview(module_key: str,
                              mod: dict,
                              out_dir: Path,
                              repo_root: Path) -> Optional[Path]:
    """Write ``docs/api/<module>/index.mdx`` for *module_key* by
    embedding the matching ``docs/development/api-*.mdx`` page when
    available.  Returns the written path, or ``None`` when no overview
    source exists for this library."""
    dev_name = _LIBRARY_OVERVIEW_DEV_FILES.get(module_key)
    label = mod.get("sidebar_label", module_key)
    namespace = mod.get("namespace") or module_key.upper() + "LIB"
    dir_slug = mod.get("dir_slug", module_key)
    out_path = out_dir / dir_slug
    out_path.mkdir(parents=True, exist_ok=True)
    out_file = out_path / "index.mdx"

    front = [
        "---",
        "id: index",
        f'title: "{label}"',
        f"sidebar_label: Overview",
        "sidebar_position: 0",
        "---",
        "",
        f"# {label}",
        "",
        f"**Namespace:** `{namespace}` &nbsp;·&nbsp; **Source:** "
        f"`src/libraries/{mod.get('include', module_key)}`",
        "",
    ]

    body: str = ""
    if dev_name:
        dev_path = repo_root / "doc" / "website" / "docs" / "development" / dev_name
        if dev_path.exists():
            raw = dev_path.read_text(encoding="utf-8")
            # Strip the dev-page frontmatter and the leading H1 (we
            # render our own title above).
            if raw.startswith("---\n"):
                end = raw.find("\n---", 4)
                if end != -1:
                    raw = raw[end + 4:].lstrip("\n")
            raw = re.sub(r"^#\s+[^\n]+\n+", "", raw, count=1)
            body = raw.rstrip() + "\n"
        else:
            LOG.warning("library overview source missing: %s", dev_path)

    if not body:
        body = (
            f"_This library landing page is generated automatically._\n\n"
            f"Use the sidebar to browse every public class in **{label}** "
            f"(`{namespace}`).  A high-level architectural description for "
            f"this library has not been written yet; contributions are "
            f"welcome under "
            f"[`doc/website/docs/development/`]"
            f"(https://github.com/mne-tools/mne-cpp/tree/staging/doc/website/docs/development).\n"
        )

    out_file.write_text("\n".join(front) + body, encoding="utf-8")
    LOG.info("generated library overview %s", out_file)
    return out_file


def generate_sidebar_fragment(sidebar_out: Path,
                              out_dir: Path,
                              repo_root: Path) -> None:
    groups: Dict[str, List[dict]] = defaultdict(list)
    for entry in _REGISTRY["classes"]:
        if not entry.get("documented", False):
            continue
        groups[entry["module"]].append(entry)
    for ents in groups.values():
        ents.sort(key=lambda e: (e.get("sidebar_position", 999), e["name"]))

    # Module order from JSON insertion order
    mod_items = sorted(
        ((k, v) for k, v in _MODULES.items() if k in groups),
        key=lambda kv: kv[1].get("sidebar_position", 999),
    )

    categories: List[str] = []
    missing_mdx: List[str] = []
    for mod_name, mod in mod_items:
        items = []
        dir_slug = mod.get('dir_slug', mod_name)
        # Generate the per-library landing page first so it can be
        # added as the first item / category link.
        overview_path = generate_library_overview(mod_name, mod, out_dir, repo_root)
        for entry in groups[mod_name]:
            slug = class_slug(entry["name"])
            mdx_path = out_dir / dir_slug / f"{slug}.mdx"
            if not mdx_path.exists():
                missing_mdx.append(f"{entry['name']} (expected {mdx_path})")
                continue
            items.append(f"        'api/{dir_slug}/{slug}'")
        if not items:
            continue
        label_json = json.dumps(_short_module_label(mod, mod_name),
                                ensure_ascii=False)
        overview_id = (
            f"api/{dir_slug}/index" if overview_path is not None else None
        )
        category_lines = [
            "    {",
            "      type: 'category',",
            f"      label: {label_json},",
            "      collapsible: true,",
            "      collapsed: true,",
        ]
        if overview_id:
            category_lines.append(
                f"      link: {{type: 'doc', id: '{overview_id}'}},"
            )
        category_lines.append("      items: [")
        category_lines.append(",\n".join(items))
        category_lines.append("      ],")
        category_lines.append("    }")
        categories.append("\n".join(category_lines))

    if missing_mdx:
        raise SystemExit(
            "ERROR: sidebar generation aborted — "
            f"{len(missing_mdx)} registered class(es) have no generated MDX page. "
            "Every entry in api_registry.json with documented=true must produce "
            "an MDX file (i.e. Doxygen must emit XML for it). Fix the missing "
            "documentation or set documented=false in the registry:\n  - "
            + "\n  - ".join(sorted(missing_mdx))
        )

    body = (
        "// Auto-generated by tools/doxy2mdx/doxy2mdx.py — do not edit by hand.\n"
        "// Regenerate with `python tools/doxy2mdx/doxy2mdx.py "
        "--generate-sidebars ...`\n\n"
        "import type {SidebarsConfig} from '@docusaurus/plugin-content-docs';\n\n"
        "const apiSidebar: SidebarsConfig['apiSidebar'] = [\n"
        "    'api/index',\n"
        + ",\n".join(categories) + "\n"
        "];\n\n"
        "export default apiSidebar;\n"
    )
    sidebar_out.parent.mkdir(parents=True, exist_ok=True)
    sidebar_out.write_text(body, encoding="utf-8")
    LOG.info("generated sidebar fragment %s", sidebar_out)


# ---------------------------------------------------------------------------
# Discovery
# ---------------------------------------------------------------------------

def _is_excluded_namespace(qualified: str) -> bool:
    parts = qualified.split("::")
    if len(parts) <= 1:
        return False
    return parts[0] in _EXCLUDED_NAMESPACES


def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--xml-dir", required=True, type=Path)
    parser.add_argument("--out-dir", required=True, type=Path)
    parser.add_argument("--registry", required=True, type=Path)
    parser.add_argument("--generate-sidebars", action="store_true")
    parser.add_argument("--sidebar-out", type=Path, default=None,
                        help="Override sidebar fragment path (default: "
                             "<out-dir>/../../sidebars.api.generated.ts).")
    parser.add_argument("--classes", default=None,
                        help="Comma-separated list of qualified names to filter to.")
    parser.add_argument("--repo-root", type=Path, default=None,
                        help="Repository root for SPDX header scraping "
                             "(default: parent of --registry).")
    parser.add_argument("--strict", action="store_true",
                        help="Treat unregistered public classes as a fatal error.")
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args(argv)

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(levelname)s %(name)s: %(message)s",
    )

    _load_registry(args.registry)
    repo_root = (args.repo_root or args.registry.resolve().parent.parent).resolve()

    if not args.xml_dir.exists():
        LOG.error("XML directory not found: %s", args.xml_dir)
        return 2

    only: Optional[set] = None
    if args.classes:
        only = {c.strip() for c in args.classes.split(",") if c.strip()}

    index_path = args.xml_dir / "index.xml"
    if not index_path.exists():
        LOG.error("Doxygen index.xml not found at %s", index_path)
        return 2

    index_tree = ET.parse(index_path)
    index_root = index_tree.getroot()

    found_qualified: set = set()
    generated: List[Path] = []
    warnings = 0

    for compound in index_root.findall("compound"):
        if compound.get("kind") != "class":
            continue
        name = compound.findtext("name", "").strip()
        if not name:
            continue
        if only is not None and name not in only:
            continue
        if _is_excluded_namespace(name):
            continue
        if "internal" in name.lower() or "::detail::" in name:
            continue

        reg_entry = module_for(name)
        if reg_entry is None:
            LOG.warning("class %s found in XML but NOT registered in api_registry.json",
                        name)
            warnings += 1
            continue
        if reg_entry.get("kind", "class") != "class":
            # Registered as a non-class kind (module/namespace); handled below.
            continue
        if not reg_entry.get("documented", True):
            LOG.info("skipping %s (documented=false in registry)", name)
            continue

        found_qualified.add(reg_entry["name"])

        refid = compound.get("refid")
        xml_file = args.xml_dir / f"{refid}.xml"
        if not xml_file.exists():
            LOG.warning("XML file %s missing for %s", xml_file, name)
            warnings += 1
            continue

        tree = ET.parse(xml_file)
        compounddef = tree.find(".//compounddef")
        if compounddef is None:
            continue

        path = generate_class_mdx(compounddef, name, args.out_dir, reg_entry, repo_root)
        generated.append(path)

    # --- Module entries (header full of free functions) -------------------
    # Map header basename -> file-compound refid from index.xml.
    file_refid_by_basename: Dict[str, str] = {}
    for compound in index_root.findall("compound"):
        if compound.get("kind") != "file":
            continue
        fname = compound.findtext("name", "").strip()
        if fname:
            file_refid_by_basename[fname] = compound.get("refid", "")

    for entry in _REGISTRY["classes"]:
        if entry.get("kind", "class") != "module":
            continue
        if not entry.get("documented", False):
            continue
        if only is not None and entry["name"] not in only:
            continue
        header_rel = entry.get("header", "")
        basename = Path(header_rel).name
        refid = file_refid_by_basename.get(basename, "")
        if not refid:
            LOG.warning("module %s: no Doxygen file compound for header %s",
                        entry["name"], header_rel)
            warnings += 1
            continue
        xml_file = args.xml_dir / f"{refid}.xml"
        if not xml_file.exists():
            LOG.warning("module %s: missing XML %s", entry["name"], xml_file)
            warnings += 1
            continue
        tree = ET.parse(xml_file)
        file_def = tree.find(".//compounddef")
        if file_def is None:
            continue
        found_qualified.add(entry["name"])
        path = generate_module_mdx(args.xml_dir, file_def, entry, args.out_dir, repo_root)
        generated.append(path)

    # Reverse check: registered with documented:true but never seen in XML.
    missing: List[str] = []
    for entry in _REGISTRY["classes"]:
        if not entry.get("documented", False):
            continue
        if only is not None:
            continue
        if entry["name"] not in found_qualified:
            missing.append(entry["name"])
    if missing and only is None:
        msg = ("registered classes with documented=true not found in "
               "Doxygen XML: " + ", ".join(sorted(missing)))
        if args.strict:
            raise SystemExit(f"ERROR: {msg}")
        LOG.warning(msg)
        warnings += 1

    LOG.info("generated %d MDX page(s)", len(generated))

    if args.generate_sidebars:
        if args.sidebar_out:
            sidebar_out = args.sidebar_out
        else:
            sidebar_out = args.out_dir.resolve().parent.parent / "sidebars.api.generated.ts"
        generate_sidebar_fragment(sidebar_out, args.out_dir, repo_root)

    if _FILES_WITHOUT_AUTHOR:
        raise SystemExit(
            "ERROR: every source file must declare at least one author "
            "(SPDX-style ``* Name <email>`` line or an @author Doxygen "
            f"tag).  {len(_FILES_WITHOUT_AUTHOR)} file(s) had none:\n  - "
            + "\n  - ".join(sorted(_FILES_WITHOUT_AUTHOR))
        )

    if args.strict and warnings:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
