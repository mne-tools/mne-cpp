#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2010-2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

"""Reconcile the Doxygen XML class list with doc/api_registry.json.

Every public class Doxygen extracts must be registered, or matched by a reasoned
entry in tools/doxy2mdx/api_docs_exclusions.json.  Every registered class with
``documented: true`` must appear in the XML.  Namespaces come from the
registry's ``modules[*].namespace``, so a module whose namespace does not exist
in the code is reported instead of silently matching nothing.

Usage:
    python3 tools/doxy2mdx/audit_registry.py --check          # CI: exit 1 on drift
    python3 tools/doxy2mdx/audit_registry.py --apply          # add stubs for unregistered classes
"""
from __future__ import annotations

import argparse
import fnmatch
import json
import sys
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path

EXCLUSIONS = Path(__file__).resolve().parent / "api_docs_exclusions.json"
TYPE_KINDS = ("class", "struct")


def xml_classes(xml_dir: Path) -> dict[str, str]:
    """Public, non-nested classes and structs in the XML: qualified name -> header path."""
    root = ET.parse(xml_dir / "index.xml").getroot()
    names = {c.findtext("name") or "" for c in root.findall("compound") if c.get("kind") in TYPE_KINDS}
    out: dict[str, str] = {}
    for comp in root.findall("compound"):
        name = comp.findtext("name") or ""
        if comp.get("kind") not in TYPE_KINDS or "::" in name and name.rsplit("::", 1)[0] in names:
            continue
        leaf = name.split("::")[-1]
        if leaf.startswith("_") or leaf.lower().endswith("private") or "::detail::" in name:
            continue
        location = ET.parse(xml_dir / f"{comp.get('refid')}.xml").getroot().find(".//location")
        out[name] = location.get("file", "") if location is not None else ""
    return out


def xml_namespaces(xml_dir: Path) -> set[str]:
    root = ET.parse(xml_dir / "index.xml").getroot()
    return {c.findtext("name") or "" for c in root.findall("compound") if c.get("kind") == "namespace"}


def load_exclusions(path: Path) -> dict:
    data = json.loads(path.read_text(encoding="utf-8"))
    problems = [f"namespace '{k}' has no reason" for k, v in data.get("namespaces", {}).items() if not str(v).strip()]
    problems += [f"class '{k}' has no reason" for k, v in data.get("classes", {}).items() if not str(v).strip()]
    unnamespaced = data.get("unnamespaced", {})
    if unnamespaced.get("header_prefixes") and not str(unnamespaced.get("reason", "")).strip():
        problems.append("unnamespaced exclusion has no reason")
    if problems:
        raise ValueError(f"{path}: " + "; ".join(problems))
    return data


def exclusion_for(name: str, header: str, exclusions: dict) -> str | None:
    namespace = name.split("::")[0] if "::" in name else ""
    if namespace in exclusions.get("namespaces", {}):
        return f"namespace:{namespace}"
    if name in exclusions.get("classes", {}):
        return f"class:{name}"
    unnamespaced = exclusions.get("unnamespaced", {})
    if not namespace and any(header.startswith(p) for p in unnamespaced.get("header_prefixes", [])):
        return "unnamespaced"
    return None


def audit(registry: dict, classes: dict[str, str], namespaces: set[str], exclusions: dict) -> list[str]:
    """Every problem that makes the registry and the XML disagree."""
    problems: list[str] = []
    module_ns = {key: mod.get("namespace") for key, mod in registry.get("modules", {}).items()}
    for key, ns in sorted(module_ns.items()):
        if ns and ns not in namespaces:
            problems.append(f"module '{key}': namespace '{ns}' does not occur in the Doxygen XML")

    registered = {entry["name"] for entry in registry.get("classes", [])}
    leaves = defaultdict(list)
    for name in classes:
        leaves[name.split("::")[-1]].append(name)

    used = set()
    for name, header in sorted(classes.items()):
        reason = exclusion_for(name, header, exclusions)
        if reason:
            used.add(reason)
        elif name.split("::")[-1] not in registered:
            problems.append(f"class '{name}' ({header}) is in the Doxygen XML but not in the registry")

    for entry in registry.get("classes", []):
        if entry.get("documented") and entry.get("kind", "class") == "class" and entry["name"] not in leaves:
            problems.append(f"registry class '{entry['name']}' is documented but not in the Doxygen XML")

    for ns in exclusions.get("namespaces", {}):
        if f"namespace:{ns}" not in used and ns not in namespaces:
            problems.append(f"exclusion: namespace '{ns}' matches nothing")
    for name in exclusions.get("classes", {}):
        if f"class:{name}" not in used:
            problems.append(f"exclusion: class '{name}' matches nothing")
    if exclusions.get("unnamespaced", {}).get("header_prefixes") and "unnamespaced" not in used:
        problems.append("exclusion: unnamespaced header prefixes match nothing")
    return problems


def make_stub(name: str, module: str, header: str, position: int, origin: str) -> dict:
    return {
        "name": name, "module": module, "header": header, "origin": origin,
        "mne_c_tool": None, "mne_python": None, "sklearn": None, "test": None, "example": None,
        "status": "documented", "skigen_candidate": False, "documented": True,
        "sidebar_position": position, "guide": None, "python_equiv": None, "python_url": None,
    }


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--xml-dir", type=Path, default=Path("doc/xml_out/xml"))
    ap.add_argument("--registry", type=Path, default=Path("doc/api_registry.json"))
    ap.add_argument("--exclusions", type=Path, default=EXCLUSIONS)
    mode = ap.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true", help="exit 1 if the registry and the XML disagree")
    mode.add_argument("--apply", action="store_true", help="add stub entries for unregistered classes")
    args = ap.parse_args(argv)

    registry = json.loads(args.registry.read_text(encoding="utf-8"))
    try:
        exclusions = load_exclusions(args.exclusions)
    except ValueError as exc:
        print(f"FAIL: {exc}")
        return 1
    classes = xml_classes(args.xml_dir)
    namespaces = xml_namespaces(args.xml_dir)

    if args.apply:
        module_of_ns = {mod.get("namespace"): key for key, mod in registry["modules"].items()}
        registered = {entry["name"] for entry in registry["classes"]}
        # Several namespaces span directories (UTILSLIB covers dsp/ and math/); siblings in the header decide.
        module_of_header = {entry["header"]: entry["module"] for entry in registry["classes"]}
        last = defaultdict(int)
        for entry in registry["classes"]:
            last[entry["module"]] = max(last[entry["module"]], int(entry.get("sidebar_position") or 0))
        added = 0
        for name, header in sorted(classes.items()):
            module = module_of_header.get(header) or (module_of_ns.get(name.split("::")[0]) if "::" in name else None)
            if module is None or exclusion_for(name, header, exclusions) or name.split("::")[-1] in registered:
                continue
            last[module] += 1
            origin = registry["modules"][module].get("origin", "custom")
            registry["classes"].append(make_stub(name.split("::")[-1], module, header, last[module], origin))
            registered.add(name.split("::")[-1])
            added += 1
            print(f"+ {module}::{name.split('::')[-1]}  ({header})")
        if added:
            args.registry.write_text(json.dumps(registry, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        print(f"{added} entries added ({len(registry['classes'])} total).")

    problems = audit(registry, classes, namespaces, exclusions)
    print(f"Doxygen classes: {len(classes)}, registry classes: {len(registry['classes'])}, problems: {len(problems)}")
    for problem in problems:
        print(f"  {problem}")
    if args.check and problems:
        print("FAIL: register the classes, add a reasoned exclusion to "
              f"{args.exclusions.name}, or fix the registry.")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
