#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2010-2026 MNE-CPP Authors

"""
Validator for doc/api_registry.json.

Enforces the following invariants:
1. JSON is well-formed.
2. Every 'header' path exists under 'src/libraries/' (relative).
3. Every 'test' (when not null) corresponds to a directory under 'src/testframes/'.
4. No entry with skigen_candidate: true AND status: "done" lacks a skigen_target.
5. Cross-reference with src/external/skigen/doc/api_registry.json if present.

Exit codes:
  0 - all validations passed
  1 - validation failure
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple


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


def validate_parity_block(registry: Dict[str, Any]) -> Tuple[bool, List[str]]:
    """Enforce invariants on the ``parity`` block (v2.4.0 TASK T7.0/T7.3).

    * ``parity`` (when present) must be an object with a ``records`` list.
    * ``mne_python_pinned`` must be a non-empty string.
    * Every record must have a non-empty string ``python`` and a ``status`` in
      the allowed enum.
    * ``python`` keys must be unique.
    * ``implemented``/``partial`` records must name a non-empty ``mne_cpp``
      equivalent; ``missing``/``not-applicable`` must leave ``mne_cpp`` empty.
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
    return (len(issues) == 0, issues)


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

    print("[1/7] JSON well-formed...")
    if not validate_json_wellformed(registry):
        all_passed = False
    else:
        print("  OK")

    print("[2/7] Header paths under src/libraries/...")
    ok, missing = validate_headers(repo_root, registry)
    if not ok:
        all_passed = False
        for m in missing:
            print(f"  MISSING: {m}")
    else:
        print(f"  OK ({total} entries)")

    print("[3/7] Test directories under src/testframes/...")
    ok, missing = validate_tests(repo_root, registry)
    if not ok:
        all_passed = False
        for m in missing:
            print(f"  MISSING: {m}")
    else:
        with_tests = sum(1 for c in registry.get("classes", []) if c.get("test"))
        print(f"  OK ({with_tests} entries with test)")

    print("[4/7] skigen_target presence on done candidates...")
    ok, issues = validate_skigen_targets(registry)
    if not ok:
        all_passed = False
        for i in issues:
            print(f"  VIOLATION: {i}")
    else:
        print("  OK")

    print("[5/7] documented flag + sidebar invariants (TASK 18.3)...")
    ok, issues = validate_documented_flag(registry)
    if not ok:
        all_passed = False
        for i in issues:
            print(f"  VIOLATION: {i}")
    else:
        doc_count = sum(1 for c in registry.get("classes", [])
                        if c.get("documented", False))
        print(f"  OK ({doc_count} documented entries)")

    print("[6/7] Parity block invariants (TASK T7.0/T7.3)...")
    ok, issues = validate_parity_block(registry)
    if not ok:
        all_passed = False
        for i in issues:
            print(f"  VIOLATION: {i}")
    else:
        parity_count = len(registry.get("parity", {}).get("records", []))
        print(f"  OK ({parity_count} parity records)")

    print("[7/7] Cross-reference with skigen registry...")
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
