#!/usr/bin/env python3
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

    print("[1/5] JSON well-formed...")
    if not validate_json_wellformed(registry):
        all_passed = False
    else:
        print("  OK")

    print("[2/5] Header paths under src/libraries/...")
    ok, missing = validate_headers(repo_root, registry)
    if not ok:
        all_passed = False
        for m in missing:
            print(f"  MISSING: {m}")
    else:
        print(f"  OK ({total} entries)")

    print("[3/5] Test directories under src/testframes/...")
    ok, missing = validate_tests(repo_root, registry)
    if not ok:
        all_passed = False
        for m in missing:
            print(f"  MISSING: {m}")
    else:
        with_tests = sum(1 for c in registry.get("classes", []) if c.get("test"))
        print(f"  OK ({with_tests} entries with test)")

    print("[4/5] skigen_target presence on done candidates...")
    ok, issues = validate_skigen_targets(registry)
    if not ok:
        all_passed = False
        for i in issues:
            print(f"  VIOLATION: {i}")
    else:
        print("  OK")

    print("[5/5] Cross-reference with skigen registry...")
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
