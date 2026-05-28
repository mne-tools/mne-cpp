#!/usr/bin/env python3
"""Audit Doxygen XML and extend doc/api_registry.json with any public
classes that are present in the C++ sources but missing from the
registry. Adds minimal stub entries so the API site stays in lock-step
with the actual public surface.

Usage:
    python3 tools/doxy2mdx/audit_registry.py [--apply]

Without --apply the script only reports missing entries.
"""
from __future__ import annotations
import argparse
import json
import re
import sys
from collections import defaultdict
from pathlib import Path
import xml.etree.ElementTree as ET


# Map Doxygen C++ namespace -> registry module key.
NS_TO_MODULE: dict[str, str] = {
    "FIFFLIB": "fiff",
    "MNELIB": "mne",
    "FWDLIB": "fwd",
    "INVERSELIB": "inv",
    "RTPROCESSINGLIB": "dsp",
    "DISPLIB": "disp",
    "DISP3DLIB": "disp3D",
    "UTILSLIB": "utils",
    "COMMUNICATIONLIB": "com",
    "LSLLIB": "lsl",
    "MNALIB": "mna",
    "BIDSLIB": "bids",
    "DECODINGLIB": "decoding",
    "MATHLIB": "math",
    "MLLIB": "ml",
    "CONNECTIVITYLIB": "conn",
    "CONNLIB": "conn",
    "DSPLIB": "dsp",
    "STATSLIB": "sts",
    "STSLIB": "sts",
    "MRILIB": "mri",
    "FSLIB": "fs",
}


def collect_public_classes(xml_dir: Path) -> dict[str, list[tuple[str, str]]]:
    """Return ``{module_key: [(class_name, header_rel_path), ...]}``."""
    idx = ET.parse(xml_dir / "index.xml").getroot()
    out: dict[str, list[tuple[str, str]]] = defaultdict(list)
    for comp in idx.findall("compound"):
        if comp.get("kind") != "class":
            continue
        full = comp.findtext("name") or ""
        if "::" not in full:
            continue
        ns, rest = full.split("::", 1)
        if "::" in rest:
            continue  # nested
        if rest.startswith("_") or rest.lower().endswith("private"):
            continue
        if ns not in NS_TO_MODULE:
            continue
        mod = NS_TO_MODULE[ns]
        refid = comp.get("refid") or ""
        header = ""
        try:
            cxml = ET.parse(xml_dir / f"{refid}.xml").getroot()
            loc = cxml.find(".//location")
            if loc is not None:
                header = loc.get("file") or ""
        except Exception:
            pass
        out[mod].append((rest, header))
    return out


def make_stub(name: str, module: str, header: str, position: int,
              origin: str) -> dict:
    return {
        "name": name,
        "module": module,
        "header": header,
        "origin": origin,
        "mne_c_tool": None,
        "mne_python": None,
        "sklearn": None,
        "test": None,
        "example": None,
        "status": "documented",
        "skigen_candidate": False,
        "documented": True,
        "sidebar_position": position,
        "guide": None,
        "python_equiv": None,
        "python_url": None,
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--xml-dir", type=Path, default=Path("doc/xml_out/xml"))
    ap.add_argument("--registry", type=Path,
                    default=Path("doc/api_registry.json"))
    ap.add_argument("--apply", action="store_true",
                    help="Write registry changes back to disk.")
    args = ap.parse_args()

    reg = json.loads(args.registry.read_text(encoding="utf-8"))
    classes = reg["classes"]
    modules = reg["modules"]
    # Dedupe by class name AND by (module, name) — many UTILSLIB classes
    # ship from dsp/ headers and are already registered under module=dsp,
    # so adding them again as module=utils would create duplicate pages.
    registered_names = {c["name"] for c in classes}
    registered = {(c["module"], c["name"]) for c in classes}
    by_mod_max_pos: dict[str, int] = defaultdict(int)
    for c in classes:
        by_mod_max_pos[c["module"]] = max(
            by_mod_max_pos[c["module"]], int(c.get("sidebar_position") or 0)
        )

    discovered = collect_public_classes(args.xml_dir)

    added = 0
    for mod_key, items in sorted(discovered.items()):
        mod_origin = modules.get(mod_key, {}).get("origin", "custom")
        for name, header in sorted(items):
            if (mod_key, name) in registered or name in registered_names:
                continue
            by_mod_max_pos[mod_key] += 1
            stub = make_stub(name, mod_key, header,
                             by_mod_max_pos[mod_key], mod_origin)
            classes.append(stub)
            registered.add((mod_key, name))
            registered_names.add(name)
            added += 1
            print(f"+ {mod_key}::{name}  ({header})")

    print(f"\n{added} new entries added to registry "
          f"({len(classes)} total).")

    if args.apply and added:
        args.registry.write_text(
            json.dumps(reg, indent=2, ensure_ascii=False) + "\n",
            encoding="utf-8",
        )
        print(f"wrote {args.registry}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
