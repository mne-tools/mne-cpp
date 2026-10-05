#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Deterministic inventory of the public MNE-Python API (v2.4.0 TASK T7.2).

Walks the documented public namespaces of the installed, pinned ``mne``
package and writes one record per public class or function: qualified name,
kind, domain, call signature and deprecation flag. Symbols that are
deliberately out of scope are kept in the output with ``excluded`` and the
reason, so nothing disappears silently. The namespace list, domain mapping and
exclusions are the ones ``gap_analysis.py`` uses, so both tools agree on the
denominator.

Usage::

    python3 tools/parity/extract_mne_python_api.py            # write the snapshot
    python3 tools/parity/extract_mne_python_api.py --check    # fail if it is stale
"""

from __future__ import annotations

import argparse
import inspect
import json
import re
import sys
import types
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gap_analysis as ga  # noqa: E402  (shares the inventory scope)

OUT_PATH = ga.REPO_ROOT / "doc" / "release" / "v2.4.0" / "mne-python-api.json"
SCHEMA_VERSION = 1


def _signature(obj: Any) -> str:
    """Call signature with defaults, without the memory addresses of default objects."""
    try:
        sig = str(inspect.signature(obj))
    except (TypeError, ValueError):
        return ""
    return re.sub(r" at 0x[0-9a-fA-F]+", "", sig)


def _exclusion(sub: str, name: str) -> str | None:
    if sub == "" and name in ga.SKIP_SUBMODULES:
        return "framework submodule"
    if name in ga.SKIP_SYMBOLS:
        return "Python-ecosystem plumbing (config, logging, docs)"
    return None


def extract(mne: types.ModuleType) -> list[dict[str, Any]]:
    """One record per public class/function, sorted by qualified name."""
    records: dict[str, dict[str, Any]] = {}
    for sub in ga.INVENTORY_SUBMODULES:
        module = mne if sub == "" else getattr(mne, sub, None)
        if module is None:
            continue
        prefix = "mne" if sub == "" else f"mne.{sub}"
        for name in sorted(ga._public_names(module)):
            try:
                obj = getattr(module, name)
            except Exception:  # lazy loaders may fail on optional dependencies
                continue
            if isinstance(obj, types.ModuleType):
                continue
            if not (inspect.isclass(obj) or inspect.isfunction(obj) or inspect.isbuiltin(obj)):
                continue
            qualname = f"{prefix}.{name}"
            if qualname in records:
                continue
            reason = _exclusion(sub, name)
            records[qualname] = {
                "python": qualname,
                "kind": "class" if inspect.isclass(obj) else "function",
                "submodule": sub or "(top-level)",
                "domain": ga._domain_for(qualname, sub, obj),
                "signature": _signature(obj),
                "deprecated": hasattr(obj, "_deprecated_original"),
                "excluded": reason is not None,
                "exclusion_reason": reason,
            }
    return [records[k] for k in sorted(records)]


def build_snapshot(mne: types.ModuleType, pinned: str) -> dict[str, Any]:
    apis = extract(mne)
    return {
        "schema_version": SCHEMA_VERSION,
        "generated_by": "tools/parity/extract_mne_python_api.py",
        "mne_python": mne.__version__,
        "mne_python_pinned": pinned,
        "namespaces": [("mne." + s) if s else "mne" for s in ga.INVENTORY_SUBMODULES],
        "excluded_namespaces": sorted("mne." + s for s in ga.SKIP_SUBMODULES),
        "summary": {
            "total": len(apis),
            "in_scope": sum(not a["excluded"] for a in apis),
            "excluded": sum(a["excluded"] for a in apis),
            "deprecated": sum(a["deprecated"] for a in apis),
            "classes": sum(a["kind"] == "class" for a in apis),
            "functions": sum(a["kind"] == "function" for a in apis),
        },
        "apis": apis,
    }


def dump(snapshot: dict[str, Any]) -> str:
    return json.dumps(snapshot, indent=2, ensure_ascii=False) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--check", action="store_true", help="fail if the committed snapshot is stale")
    parser.add_argument("--out", type=Path, default=OUT_PATH)
    args = parser.parse_args(argv)

    import mne  # lazy so --help works without mne

    pinned = ga.pinned_version(ga.load_registry())
    if not ga._pinned_ok(mne.__version__, pinned):
        sys.stderr.write(f"ERROR: registry pins MNE-Python {pinned}.x but {mne.__version__} is installed.\n")
        return 2

    text = dump(build_snapshot(mne, pinned))
    if args.check:
        if not args.out.exists() or args.out.read_text(encoding="utf-8") != text:
            sys.stderr.write(f"ERROR: {args.out} is stale; rerun tools/parity/extract_mne_python_api.py\n")
            return 1
        print(f"{args.out.name} is up to date.")
        return 0
    args.out.write_text(text, encoding="utf-8")
    summary = json.loads(text)["summary"]
    print(f"Wrote {args.out}: {summary['total']} APIs, {summary['excluded']} excluded, {summary['deprecated']} deprecated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
