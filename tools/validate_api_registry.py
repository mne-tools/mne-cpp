#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2010-2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

"""
Validator for doc/api_registry.json.

Enforces the following invariants:
1. JSON is well-formed.
2. Every 'header' path exists under 'src/libraries/' (relative).
3. Every 'test' (when not null) corresponds to a directory under 'src/testframes/'.
4. No entry with skigen_candidate: true AND status: "done" lacks a skigen_target.
5. Example evidence (v2.4.0 T3.2): example targets, snippets, modes, data,
   screenshots and exemptions; eligible entries without an example are errors
   unless listed in the shrink-only example_debt of api_evidence_policy.json.
6. Cross-reference with src/external/skigen/doc/api_registry.json if present.

Exit codes:
  0 - all validations passed
  1 - validation failure
"""

import argparse
import json
import re
import sys
from collections import Counter
from pathlib import Path
from typing import Any, Dict, List, Optional, Set, Tuple

EXAMPLE_MODES = {"run", "compile"}
DATASETS = {"mne-cpp-test-data", "MNE-sample-data"}
EVIDENCE_POLICY = Path("tools") / "quality" / "api_evidence_policy.json"
SCREENSHOT_MANIFEST = Path("doc") / "website" / "screenshots" / "manifest.json"
_SNIPPET_RE = re.compile(r"[@\\]snippet\s+(\S+)\s+(\S+)")
_MARKER_RE = re.compile(r"^\s*//!\s*\[([^\]]+)\]\s*$")
_SUBDIR_RE = re.compile(r"^\s*add_subdirectory\s*\(\s*([A-Za-z0-9_./+-]+)", re.MULTILINE)
_PROJECT_RE = re.compile(r"^\s*project\s*\(\s*([A-Za-z0-9_]+)", re.MULTILINE | re.IGNORECASE)


def load_json(path: Path) -> Optional[Dict[str, Any]]:
    if not path.exists():
        return None
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except (json.JSONDecodeError, IOError) as e:
        print(f"ERROR: Failed to parse {path}: {e}")
        return False  # type: ignore[return-value]


def validate_json_wellformed(registry: Dict[str, Any]) -> bool:
    if not isinstance(registry, dict):
        print("ERROR: Registry root must be a JSON object.")
        return False
    if "classes" not in registry or not isinstance(registry["classes"], list):
        print("ERROR: Registry must have 'classes' key with list value.")
        return False
    return True


def validate_headers(repo_root: Path, registry: Dict[str, Any]) -> Tuple[bool, List[str]]:
    missing: List[str] = []
    for cls in registry.get("classes", []):
        if "header" not in cls:
            continue
        header_rel = cls["header"]
        header_full = repo_root / "src" / "libraries" / header_rel
        if not header_full.exists():
            missing.append(f"{header_rel} (class: {cls.get('name', 'UNNAMED')})")
    return (len(missing) == 0, missing)


def validate_tests(repo_root: Path, registry: Dict[str, Any]) -> Tuple[bool, List[str]]:
    missing: List[str] = []
    for cls in registry.get("classes", []):
        test_name = cls.get("test")
        if not test_name:
            continue
        test_dir = repo_root / "src" / "testframes" / test_name
        if not test_dir.is_dir():
            missing.append(f"{test_name} (class: {cls.get('name', 'UNNAMED')})")
    return (len(missing) == 0, missing)


def validate_skigen_targets(registry: Dict[str, Any]) -> Tuple[bool, List[str]]:
    issues: List[str] = []
    for cls in registry.get("classes", []):
        skigen_cand = cls.get("skigen_candidate", False)
        status = cls.get("status")
        has_target = bool(cls.get("skigen_target"))
        if skigen_cand and status == "done" and not has_target:
            issues.append(
                f"'{cls.get('name', 'UNNAMED')}': skigen_candidate=true, "
                f"status=done, but no skigen_target"
            )
    return (len(issues) == 0, issues)


def validate_documented_flag(registry: Dict[str, Any]) -> Tuple[bool, List[str]]:
    """Enforce the TASK 18.3 ``documented`` flag invariants.

    * ``documented`` (when present) must be a boolean.
    * For every ``documented: true`` entry, ``name`` and ``module``
      must be non-empty strings.
    * The referenced module must exist in ``modules`` and have
      ``dir_slug`` set (added in TASK 18.3 alongside the flag).
    * Within a module, ``(module, sidebar_position)`` tuples must be
      unique across ``documented: true`` entries.
    """
    issues: List[str] = []
    modules = registry.get("modules", {})
    if not isinstance(modules, dict):
        return (False, ["'modules' must be a JSON object"])

    seen_positions: Dict[Tuple[str, int], str] = {}
    for cls in registry.get("classes", []):
        name = cls.get("name", "UNNAMED")
        if "documented" in cls and not isinstance(cls["documented"], bool):
            issues.append(f"'{name}': 'documented' must be boolean, got "
                          f"{type(cls['documented']).__name__}")
            continue
        if not cls.get("documented", False):
            continue
        if not isinstance(cls.get("name"), str) or not cls["name"]:
            issues.append(f"documented entry has empty/missing 'name'")
            continue
        mod_key = cls.get("module")
        if not isinstance(mod_key, str) or not mod_key:
            issues.append(f"'{name}': documented entry has empty/missing 'module'")
            continue
        if mod_key not in modules:
            issues.append(f"'{name}': module '{mod_key}' not declared in "
                          f"registry 'modules'")
            continue
        mod = modules[mod_key]
        if not isinstance(mod, dict) or not mod.get("dir_slug"):
            issues.append(f"'{name}': module '{mod_key}' is missing required "
                          f"'dir_slug' field")
            continue
        pos = cls.get("sidebar_position")
        if pos is not None:
            if not isinstance(pos, int):
                issues.append(f"'{name}': 'sidebar_position' must be an int")
                continue
            key = (mod_key, pos)
            if key in seen_positions:
                issues.append(
                    f"'{name}': sidebar_position collision in module "
                    f"'{mod_key}' (position {pos} already used by "
                    f"'{seen_positions[key]}')"
                )
                continue
            seen_positions[key] = name
    return (len(issues) == 0, issues)


PARITY_STATUS_VALUES = {"implemented", "partial", "missing", "not-applicable"}


def validate_parity_block(registry: Dict[str, Any], repo_root: Path) -> Tuple[bool, List[str]]:
    """Enforce invariants on the ``parity`` block (v2.4.0 TASK T7.0/T7.3).

    * ``parity`` (when present) must be an object with a ``records`` list.
    * ``mne_python_pinned`` must be a non-empty string.
    * Every record must have a non-empty string ``python`` and a ``status`` in
      the allowed enum.
    * ``python`` keys must be unique.
    * ``implemented``/``partial`` records must name a non-empty ``mne_cpp``
      equivalent; ``missing``/``not-applicable`` must leave ``mne_cpp`` empty.
    * An optional ``test`` (the evidence for an implemented/partial claim)
      must name a ``src/testframes`` directory.
    """
    issues: List[str] = []
    parity = registry.get("parity")
    if parity is None:
        return (True, [])  # optional block
    if not isinstance(parity, dict):
        return (False, ["'parity' must be a JSON object"])
    if not str(parity.get("mne_python_pinned", "")).strip():
        issues.append("'parity.mne_python_pinned' must be a non-empty string")
    records = parity.get("records")
    if not isinstance(records, list):
        return (False, ["'parity.records' must be a list"])

    seen: Dict[str, int] = {}
    for idx, rec in enumerate(records):
        if not isinstance(rec, dict):
            issues.append(f"parity record #{idx} is not an object")
            continue
        py = rec.get("python")
        if not isinstance(py, str) or not py.strip():
            issues.append(f"parity record #{idx} has empty/missing 'python'")
            continue
        if py in seen:
            issues.append(f"duplicate parity 'python' key: '{py}'")
            continue
        seen[py] = idx
        status = rec.get("status")
        if status not in PARITY_STATUS_VALUES:
            issues.append(
                f"'{py}': status '{status}' not in "
                f"{sorted(PARITY_STATUS_VALUES)}"
            )
            continue
        mne_cpp = (rec.get("mne_cpp") or "").strip()
        if status in ("implemented", "partial") and not mne_cpp:
            issues.append(
                f"'{py}': status '{status}' requires a non-empty 'mne_cpp'"
            )
        if status in ("missing", "not-applicable") and mne_cpp:
            issues.append(
                f"'{py}': status '{status}' must not name an 'mne_cpp' "
                f"equivalent (found '{mne_cpp}')"
            )
        test = rec.get("test")
        if test is not None and (status not in ("implemented", "partial") or not isinstance(test, str)
                                 or not (repo_root / "src" / "testframes" / test).is_dir()):
            issues.append(f"'{py}': 'test' must name an existing src/testframes directory "
                          f"on an implemented/partial record (found '{test}')")
    return (len(issues) == 0, issues)


def example_markers(examples_dir: Path) -> Tuple[Dict[str, Counter], List[str]]:
    """Region-marker counts per example file, and every marker not used exactly twice."""
    markers: Dict[str, Counter] = {}
    issues: List[str] = []
    for source in sorted(examples_dir.rglob("*")):
        if source.suffix not in {".cpp", ".h"}:
            continue
        lines = source.read_text(encoding="utf-8", errors="replace").splitlines()
        counts = Counter(m.group(1) for m in map(_MARKER_RE.match, lines) if m)
        relative = source.relative_to(examples_dir).as_posix()
        markers[relative] = counts
        issues += [f"src/examples/{relative}: marker [{name}] appears {n} times, expected 2"
                   for name, n in sorted(counts.items()) if n != 2]
    return markers, issues


def header_snippets(libraries_dir: Path) -> Dict[str, Set[str]]:
    """Header (relative to src/libraries) -> '<path>#<region>' @snippet references."""
    refs: Dict[str, Set[str]] = {}
    for header in sorted(libraries_dir.rglob("*.h")):
        found = _SNIPPET_RE.findall(header.read_text(encoding="utf-8", errors="replace"))
        if found:
            refs[header.relative_to(libraries_dir).as_posix()] = {f"{p}#{r}" for p, r in found}
    return refs


def example_targets(examples_dir: Path) -> Dict[str, str]:
    """Example directory added by src/examples/CMakeLists.txt -> its CMake project (target) name."""
    cmake = examples_dir / "CMakeLists.txt"
    targets: Dict[str, str] = {}
    if not cmake.is_file():
        return targets
    for directory in _SUBDIR_RE.findall(cmake.read_text(encoding="utf-8")):
        leaf = examples_dir / directory / "CMakeLists.txt"
        match = _PROJECT_RE.search(leaf.read_text(encoding="utf-8")) if leaf.is_file() else None
        if match:
            targets[directory] = match.group(1)
    return targets


def policy_exemption(name: str, header: str, policy: Dict[str, Any]) -> Optional[str]:
    for prefix, reason in policy.get("exempt_header_prefixes", {}).items():
        if header.startswith(prefix):
            return reason
    for suffix, reason in policy.get("exempt_name_suffixes", {}).items():
        if name.endswith(suffix):
            return reason
    return None


def validate_examples(repo_root: Path, registry: Dict[str, Any],
                      policy: Dict[str, Any], manifest: Dict[str, Any]) -> Tuple[bool, List[str], Dict[str, int]]:
    """Enforce the example-evidence fields of v2.4.0 T3.2.

    Optional per-class fields: ``example`` (directory under src/examples),
    ``example_snippet`` (list of ``<example>/<file>#<region>``), ``example_mode``
    (run|compile), ``required_data`` (dataset names), ``example_exempt`` with
    ``example_exempt_reason``, and ``screenshots`` (manifest ids).
    """
    examples_dir = repo_root / "src" / "examples"
    markers, issues = example_markers(examples_dir)
    snippets_in_headers = header_snippets(repo_root / "src" / "libraries")
    targets = example_targets(examples_dir)
    shots = {shot.get("id") for shot in manifest.get("shots", [])}
    debt = policy.get("example_debt", {}).get("names", [])
    ceiling = policy.get("example_debt", {}).get("ceiling", 0)
    if len(debt) != len(set(debt)):
        issues.append("example_debt lists a name twice")
    if len(debt) > ceiling:
        issues.append(f"example_debt has {len(debt)} names, above its ceiling of {ceiling}; the list may only shrink")
    debt_set = set(debt)
    claimed: Set[Tuple[str, str]] = set()
    stats = {"eligible": 0, "backed": 0, "exempt": 0, "debt": 0}

    for cls in registry.get("classes", []):
        name, header = cls.get("name", "UNNAMED"), cls.get("header", "")
        example = cls.get("example")
        snippets = cls.get("example_snippet", [])
        exempt = cls.get("example_exempt", False)
        reason = cls.get("example_exempt_reason")
        where = f"'{name}'"

        if not isinstance(exempt, bool):
            issues.append(f"{where}: 'example_exempt' must be boolean")
        if exempt and (not isinstance(reason, str) or not reason.strip()):
            issues.append(f"{where}: 'example_exempt' requires a non-empty 'example_exempt_reason'")
        if reason is not None and not exempt:
            issues.append(f"{where}: 'example_exempt_reason' without 'example_exempt: true'")
        if exempt and example:
            issues.append(f"{where}: exempt but names example '{example}'")

        if example:
            if example not in targets:
                issues.append(f"{where}: example '{example}' is not a CMake target added by src/examples/CMakeLists.txt")
            if cls.get("example_mode", "compile") not in EXAMPLE_MODES:
                issues.append(f"{where}: 'example_mode' must be one of {sorted(EXAMPLE_MODES)}")
        else:
            for key in ("example_snippet", "example_mode", "required_data"):
                if key in cls:
                    issues.append(f"{where}: '{key}' requires 'example'")

        if not isinstance(snippets, list):
            issues.append(f"{where}: 'example_snippet' must be a list")
            snippets = []
        for ref in snippets:
            path, _, region = str(ref).partition("#")
            if not region or path.split("/")[0] != example or "/" not in path:
                issues.append(f"{where}: snippet '{ref}' must be '{example}/<file>#<region>'")
            elif markers.get(path, Counter())[region] != 2:
                issues.append(f"{where}: snippet '{ref}' has no balanced region in src/examples/{path}")
            elif ref not in snippets_in_headers.get(header, set()):
                issues.append(f"{where}: snippet '{ref}' is not referenced by @snippet in {header}")
            claimed.add((header, ref))

        data = cls.get("required_data", [])
        if not isinstance(data, list) or any(item not in DATASETS for item in data):
            issues.append(f"{where}: 'required_data' must list datasets from {sorted(DATASETS)}")
        for shot in cls.get("screenshots", []):
            if shot not in shots:
                issues.append(f"{where}: screenshot '{shot}' is not in {SCREENSHOT_MANIFEST.as_posix()}")

        if exempt or policy_exemption(name, header, policy):
            stats["exempt"] += 1
            if name in debt_set:
                issues.append(f"{where}: exempt, remove it from example_debt")
            continue
        stats["eligible"] += 1
        backed = bool(example) and example in targets and bool(snippets)
        if backed:
            stats["backed"] += 1
            if name in debt_set:
                issues.append(f"{where}: now has an example and snippet, remove it from example_debt")
        elif name in debt_set:
            stats["debt"] += 1
        else:
            issues.append(f"{where}: example-eligible but has no built example with an example_snippet")

    registered = {cls.get("name") for cls in registry.get("classes", [])}
    issues += [f"example_debt: '{name}' is not a registry class" for name in sorted(debt_set - registered)]
    for header, refs in sorted(snippets_in_headers.items()):
        for ref in sorted(refs):
            if (header, ref) not in claimed:
                issues.append(f"{header}: @snippet '{ref}' is not listed in any registry 'example_snippet'")
    return (len(issues) == 0, issues, stats)


def validate_skigen_cross_reference(
    repo_root: Path,
    mne_cpp_registry: Dict[str, Any],
    skigen_path: Optional[Path],
    strict: bool,
) -> Tuple[bool, List[str]]:
    issues: List[str] = []
    if skigen_path is None:
        skigen_path = repo_root / "src" / "external" / "skigen" / "doc" / "api_registry.json"
    if not skigen_path.exists():
        print(f"NOTE: skigen registry not found at {skigen_path}; skipping cross-reference.")
        return (True, [])
    skigen_reg = load_json(skigen_path)
    if skigen_reg is False:
        return (False, [f"Failed to parse skigen registry at {skigen_path}"])
    if skigen_reg is None:
        return (True, [])
    skigen_classes = {c.get("name", "") for c in skigen_reg.get("classes", [])}
    for cls in mne_cpp_registry.get("classes", []):
        skigen_target = cls.get("skigen_target")
        if not skigen_target:
            continue
        parts = skigen_target.split("::")
        if len(parts) >= 2:
            target_class = parts[-1].split()[0]
            if target_class not in skigen_classes:
                issues.append(
                    f"Drift: '{cls.get('name')}' references skigen_target "
                    f"'{skigen_target}', but '{target_class}' not in skigen registry"
                )
    if issues and strict:
        return (False, issues)
    for issue in issues:
        print(f"WARNING: {issue}")
    return (True, issues)


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate doc/api_registry.json")
    parser.add_argument("--registry", type=Path, default=None)
    parser.add_argument("--skigen-registry", type=Path, default=None)
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--strict", action="store_true")
    args = parser.parse_args()

    repo_root = args.repo_root.resolve()
    registry_path = (args.registry or (repo_root / "doc" / "api_registry.json")).resolve()

    print(f"Validating {registry_path}")
    print(f"Repository root: {repo_root}\n")

    registry = load_json(registry_path)
    if registry is None:
        print(f"ERROR: Registry file not found: {registry_path}")
        return 1
    if registry is False:
        return 1

    all_passed = True
    total = len(registry.get("classes", []))

    print("[1/8] JSON well-formed...")
    if not validate_json_wellformed(registry):
        all_passed = False
    else:
        print("  OK")

    print("[2/8] Header paths under src/libraries/...")
    ok, missing = validate_headers(repo_root, registry)
    if not ok:
        all_passed = False
        for m in missing:
            print(f"  MISSING: {m}")
    else:
        print(f"  OK ({total} entries)")

    print("[3/8] Test directories under src/testframes/...")
    ok, missing = validate_tests(repo_root, registry)
    if not ok:
        all_passed = False
        for m in missing:
            print(f"  MISSING: {m}")
    else:
        with_tests = sum(1 for c in registry.get("classes", []) if c.get("test"))
        print(f"  OK ({with_tests} entries with test)")

    print("[4/8] skigen_target presence on done candidates...")
    ok, issues = validate_skigen_targets(registry)
    if not ok:
        all_passed = False
        for i in issues:
            print(f"  VIOLATION: {i}")
    else:
        print("  OK")

    print("[5/8] documented flag + sidebar invariants (TASK 18.3)...")
    ok, issues = validate_documented_flag(registry)
    if not ok:
        all_passed = False
        for i in issues:
            print(f"  VIOLATION: {i}")
    else:
        doc_count = sum(1 for c in registry.get("classes", [])
                        if c.get("documented", False))
        print(f"  OK ({doc_count} documented entries)")

    print("[6/8] Parity block invariants (TASK T7.0/T7.3)...")
    ok, issues = validate_parity_block(registry, repo_root)
    if not ok:
        all_passed = False
        for i in issues:
            print(f"  VIOLATION: {i}")
    else:
        parity_count = len(registry.get("parity", {}).get("records", []))
        print(f"  OK ({parity_count} parity records)")

    print("[7/8] Example evidence (TASK T3.2)...")
    policy = load_json(repo_root / EVIDENCE_POLICY) or {}
    manifest = load_json(repo_root / SCREENSHOT_MANIFEST) or {}
    ok, issues, stats = validate_examples(repo_root, registry, policy, manifest)
    if not ok:
        all_passed = False
        for i in issues:
            print(f"  VIOLATION: {i}")
    else:
        print(f"  OK ({stats['backed']} of {stats['eligible']} eligible entries backed, "
              f"{stats['debt']} in example_debt, {stats['exempt']} exempt)")

    print("[8/8] Cross-reference with skigen registry...")
    ok, _ = validate_skigen_cross_reference(repo_root, registry, args.skigen_registry, args.strict)
    if not ok:
        all_passed = False
    print()

    if all_passed:
        print("=" * 60)
        print("ALL VALIDATIONS PASSED")
        print("=" * 60)
        return 0
    print("=" * 60)
    print("VALIDATION FAILED")
    print("=" * 60)
    return 1


if __name__ == "__main__":
    sys.exit(main())
