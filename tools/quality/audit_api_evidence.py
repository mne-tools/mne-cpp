#!/usr/bin/env python3
# =============================================================================================================
#
# @file     audit_api_evidence.py
# @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
# @since    2.4.0
# @date     September, 2026
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#
# @brief    Classify every exported library class by the evidence that backs it.
#
# =============================================================================================================
"""Audit API evidence for gate G3 (package T0.3).

For every class exported from ``src/libraries`` (``class|struct <LIB>SHARED_EXPORT``)
and every ``doc/api_registry.json`` record, report:

* whether the class is registered, and whether a registry record names a class
  that is no longer exported,
* whether its registry ``test`` resolves to a test directory,
* whether its registry ``example`` resolves to a directory under ``src/examples``
  and whether that example is built by ``src/examples/CMakeLists.txt``,
* whether any example includes the class's header (usage evidence independent
  of the registry),
* whether any Doxygen ``@snippet`` references it,
* whether its generated API page exists under ``doc/website/docs/api``,
* whether it carries a Python equivalent,
* whether it is example-eligible under ``api_evidence_policy.json``.

Eligible-backed / eligible-total is the G3 example metric.  The denominator is
explicit so that exemptions cannot hide debt.

Usage
-----
    python3 tools/quality/audit_api_evidence.py
    python3 tools/quality/audit_api_evidence.py --json doc/release/v2.4.0/api-evidence-baseline.json \\
        --markdown doc/release/v2.4.0/api-evidence-baseline.md
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[2]
LIBRARIES_DIR = REPO_ROOT / "src" / "libraries"
EXAMPLES_DIR = REPO_ROOT / "src" / "examples"
REGISTRY = REPO_ROOT / "doc" / "api_registry.json"
API_DOCS_DIR = REPO_ROOT / "doc" / "website" / "docs" / "api"
POLICY_FILE = Path(__file__).resolve().parent / "api_evidence_policy.json"

sys.path.insert(0, str(REPO_ROOT / "tools" / "doxy2mdx"))
from doxy2mdx import page_slug  # noqa: E402

_EXPORT_RE = re.compile(
    r"^\s*(?:class|struct)\s+([A-Z0-9]+)SHARED_EXPORT\s+([A-Za-z_][A-Za-z0-9_]*)", re.MULTILINE
)
_FREE_FUNCTION_RE = re.compile(r"^\s*([A-Z0-9]+)SHARED_EXPORT\s+(?!class\b|struct\b)[^;(]*?\b(\w+)\s*\(", re.MULTILINE)
_INCLUDE_RE = re.compile(r"#include\s*[<\"]([^>\"]+)[>\"]")
_SNIPPET_RE = re.compile(r"[@\\]snippet\s+(\S+)\s+(\S+)")
_SUBDIR_RE = re.compile(r"^\s*add_subdirectory\s*\(\s*([A-Za-z0-9_./+-]+)", re.MULTILINE)


def exported_symbols(libraries_dir: Path) -> tuple[list[dict[str, str]], list[dict[str, str]]]:
    """Exported classes and exported free functions, keyed by header."""
    classes: list[dict[str, str]] = []
    functions: list[dict[str, str]] = []
    for header in sorted(libraries_dir.rglob("*.h")):
        text = header.read_text(encoding="utf-8", errors="replace")
        relative = header.relative_to(libraries_dir).as_posix()
        for match in _EXPORT_RE.finditer(text):
            classes.append({"name": match.group(2), "header": relative, "library": relative.split("/")[0]})
        for match in _FREE_FUNCTION_RE.finditer(text):
            functions.append({"name": match.group(2), "header": relative, "library": relative.split("/")[0]})
    return classes, functions


def built_examples(examples_dir: Path) -> set[str]:
    cmake = examples_dir / "CMakeLists.txt"
    return set(_SUBDIR_RE.findall(cmake.read_text(encoding="utf-8"))) if cmake.is_file() else set()


def example_includes(examples_dir: Path) -> dict[str, list[str]]:
    """Header -> examples whose sources include it."""
    includes: dict[str, set[str]] = {}
    for source in sorted(examples_dir.rglob("*")):
        if source.suffix not in {".cpp", ".h"}:
            continue
        example = source.relative_to(examples_dir).parts[0]
        for header in _INCLUDE_RE.findall(source.read_text(encoding="utf-8", errors="replace")):
            includes.setdefault(header, set()).add(example)
    return {header: sorted(names) for header, names in includes.items()}


def snippet_references(libraries_dir: Path) -> dict[str, list[str]]:
    """Header -> snippet references found in its Doxygen comments."""
    references: dict[str, list[str]] = {}
    for header in sorted(libraries_dir.rglob("*.h")):
        found = _SNIPPET_RE.findall(header.read_text(encoding="utf-8", errors="replace"))
        if found:
            references[header.relative_to(libraries_dir).as_posix()] = [f"{path}#{region}" for path, region in found]
    return references


def exemption(name: str, header: str, policy: dict[str, Any]) -> str | None:
    for prefix, reason in policy.get("exempt_header_prefixes", {}).items():
        if header.startswith(prefix):
            return f"header prefix '{prefix}': {reason}"
    for suffix, reason in policy.get("exempt_name_suffixes", {}).items():
        if name.endswith(suffix):
            return f"name suffix '{suffix}': {reason}"
    return None


def build_report(repo_root: Path = REPO_ROOT) -> dict[str, Any]:
    libraries_dir = repo_root / "src" / "libraries"
    examples_dir = repo_root / "src" / "examples"
    registry = json.loads((repo_root / "doc" / "api_registry.json").read_text(encoding="utf-8"))
    policy = json.loads(POLICY_FILE.read_text(encoding="utf-8"))
    modules = registry.get("modules", {})

    exported, functions = exported_symbols(libraries_dir)
    built = built_examples(examples_dir)
    example_dirs = {p.name for p in examples_dir.iterdir() if p.is_dir()} if examples_dir.is_dir() else set()
    includes = example_includes(examples_dir)
    snippets = snippet_references(libraries_dir)
    test_dirs = {p.name for p in (repo_root / "src" / "testframes").iterdir() if p.is_dir()}

    by_key = {(c["name"], c["header"]): c for c in registry.get("classes", [])}
    exported_keys = {(c["name"], c["header"]) for c in exported}

    records: list[dict[str, Any]] = []
    keys = sorted(exported_keys | set(by_key), key=lambda key: (key[1], key[0]))
    for name, header in keys:
        entry = by_key.get((name, header))
        example = entry.get("example") if entry else None
        test = entry.get("test") if entry else None
        module = modules.get(entry["module"], {}) if entry else {}
        page = None
        if entry and module.get("dir_slug"):
            candidate = f"doc/website/docs/api/{module['dir_slug']}/{page_slug(name, module['dir_slug'])}.mdx"
            page = candidate if (repo_root / candidate).is_file() else None
        exempt_reason = exemption(name, header, policy)
        example_state = (
            "none" if not example
            else "missing" if example not in example_dirs
            else "not-built" if example not in built
            else "built"
        )
        if (name, header) in exported_keys:
            kind = "exported-class"
        elif entry and entry.get("kind") == "module":
            kind = "function-module"
        elif (libraries_dir / header).is_file():
            kind = "header-only"
        else:
            kind = "stale"
        records.append({
            "name": name,
            "header": header,
            "library": header.split("/")[0],
            "kind": kind,
            "exported": kind == "exported-class",
            "registered": entry is not None,
            "registry_status": entry.get("status") if entry else None,
            "test": test,
            "test_resolves": bool(test) and test in test_dirs,
            "example": example,
            "example_state": example_state,
            "including_examples": includes.get(header, []),
            "snippets": snippets.get(header, []),
            "api_page": page,
            "python_equiv": (entry.get("python_equiv") or entry.get("mne_python")) if entry else None,
            "example_eligible": exempt_reason is None,
            "exempt_reason": exempt_reason,
        })

    def count(predicate) -> int:
        return sum(1 for record in records if predicate(record))

    public = [r for r in records if r["kind"] != "stale"]
    eligible = [r for r in public if r["example_eligible"]]
    by_library: dict[str, dict[str, int]] = {}
    for record in public:
        bucket = by_library.setdefault(record["library"], {
            "public": 0, "registered": 0, "with_test": 0, "eligible": 0,
            "eligible_with_registry_example": 0, "eligible_used_by_example": 0, "with_snippet": 0,
        })
        bucket["public"] += 1
        bucket["registered"] += record["registered"]
        bucket["with_test"] += record["test_resolves"]
        bucket["with_snippet"] += bool(record["snippets"])
        if record["example_eligible"]:
            bucket["eligible"] += 1
            bucket["eligible_with_registry_example"] += record["example_state"] == "built"
            bucket["eligible_used_by_example"] += bool(record["including_examples"])

    summary = {
        "exported_classes": count(lambda r: r["exported"]),
        "exported_free_functions": len(functions),
        "registered_classes": len(registry.get("classes", [])),
        "exported_not_registered": count(lambda r: r["exported"] and not r["registered"]),
        "registered_function_modules": count(lambda r: r["kind"] == "function-module"),
        "registered_header_only": count(lambda r: r["kind"] == "header-only"),
        "registered_stale": count(lambda r: r["kind"] == "stale"),
        "registered_with_test": count(lambda r: r["registered"] and r["test"]),
        "registered_test_unresolved": count(lambda r: r["test"] and not r["test_resolves"]),
        "registered_with_example": count(lambda r: r["registered"] and r["example"]),
        "registered_example_not_built": count(lambda r: r["example_state"] in {"missing", "not-built"}),
        "registered_with_python_equiv": count(lambda r: r["python_equiv"]),
        "registered_without_api_page": count(lambda r: r["registered"] and not r["api_page"]),
        "with_snippet": count(lambda r: r["snippets"]),
        "public_api_units": len(public),
        "example_eligible": len(eligible),
        "example_exempt": len(public) - len(eligible),
        "eligible_backed_by_registry_example": sum(1 for r in eligible if r["example_state"] == "built"),
        "eligible_used_by_some_example": sum(1 for r in eligible if r["including_examples"]),
        "examples_on_disk": len(example_dirs),
        "examples_built": len(built & example_dirs),
    }
    return {
        "schema_version": 1,
        "generated_by": "tools/quality/audit_api_evidence.py",
        "policy": "tools/quality/api_evidence_policy.json",
        "summary": summary,
        "by_library": dict(sorted(by_library.items())),
        "exported_free_functions": functions,
        "classes": records,
    }


def _ratio(part: int, whole: int) -> str:
    return f"{part} / {whole} ({100.0 * part / whole:.1f}%)" if whole else "0 / 0"


def render_markdown(report: dict[str, Any]) -> str:
    s = report["summary"]
    lines = [
        "# MNE-CPP v2.4.0 API evidence baseline",
        "",
        "Generated by `tools/quality/audit_api_evidence.py`. Do not edit by hand.",
        "",
        "## Inventory",
        "",
        f"- Exported library classes: {s['exported_classes']} (plus {s['exported_free_functions']} exported free functions)",
        f"- Registry records: {s['registered_classes']}",
        f"- Exported but not registered: {s['exported_not_registered']}",
        f"- Registered free-function namespaces: {s['registered_function_modules']}",
        f"- Registered header-only templates/interfaces: {s['registered_header_only']}",
        f"- Registered but no longer in the tree (stale): {s['registered_stale']}",
        f"- Registered without a generated API page: {s['registered_without_api_page']}",
        "",
        "## Evidence",
        "",
        f"- Registry test reference: {_ratio(s['registered_with_test'], s['registered_classes'])}"
        f" ({s['registered_test_unresolved']} unresolved)",
        f"- Registry example reference: {_ratio(s['registered_with_example'], s['registered_classes'])}"
        f" ({s['registered_example_not_built']} missing or not built)",
        f"- Python equivalent recorded: {_ratio(s['registered_with_python_equiv'], s['registered_classes'])}",
        f"- Doxygen `@snippet` references: {s['with_snippet']}",
        "",
        "## G3 example metric",
        "",
        f"Public API units (exported classes, registered function namespaces and header-only types): "
        f"{s['public_api_units']}. Example-eligible: **{s['example_eligible']}** "
        f"({s['example_exempt']} exempt under the proposed policy).",
        "",
        f"- Eligible with a built registry example: {_ratio(s['eligible_backed_by_registry_example'], s['example_eligible'])}",
        f"- Eligible whose header is included by any example: {_ratio(s['eligible_used_by_some_example'], s['example_eligible'])}",
        f"- Examples on disk / built: {s['examples_on_disk']} / {s['examples_built']}",
        "",
        "## By library",
        "",
        "| Library | Public | Registered | Test | Eligible | Registry example | Used by example | Snippet |",
        "|---|---:|---:|---:|---:|---:|---:|---:|",
    ]
    for library, b in report["by_library"].items():
        lines.append(
            f"| `{library}` | {b['public']} | {b['registered']} | {b['with_test']} | {b['eligible']} "
            f"| {b['eligible_with_registry_example']} | {b['eligible_used_by_example']} | {b['with_snippet']} |"
        )
    drift = [r for r in report["classes"] if (r["exported"] and not r["registered"]) or r["kind"] == "stale"]
    lines += ["", "## Registry drift", "", "| Class | Header | Problem |", "|---|---|---|"]
    for record in drift:
        problem = "exported, not registered" if record["exported"] else "registered, not in the tree"
        lines.append(f"| `{record['name']}` | `{record['header']}` | {problem} |")
    if not drift:
        lines.append("| - | - | none |")
    return "\n".join(lines) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--json", type=Path, dest="json_path", help="write the JSON report")
    parser.add_argument("--markdown", type=Path, help="write the Markdown summary")
    args = parser.parse_args(argv)

    report = build_report()
    if args.json_path:
        args.json_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if args.markdown:
        args.markdown.write_text(render_markdown(report), encoding="utf-8")
    if not args.json_path and not args.markdown:
        print(render_markdown(report), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
