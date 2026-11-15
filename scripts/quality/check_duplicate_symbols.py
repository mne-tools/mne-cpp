#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.5.0
# @date     November, 2026
#
#
# @brief    Fail when two MNE-CPP shared libraries define the same mne:: symbol.
"""Check that no function or variable in namespace ``mne`` is defined by more than one library.

All libraries share the namespace ``mne``, so two libraries can define the same function by accident.
A dynamic build still links (each client binds to whichever library it finds first), but a static build
fails and the dynamic one silently runs whichever copy the loader picks. This lists the exported, strong
(non-weak) ``mne::`` symbols of every built ``libmne_*`` shared library and reports those defined twice.
Inline functions and template instantiations are weak and are not reported.

Usage
-----
    python3 scripts/quality/check_duplicate_symbols.py out/Release/lib

Exit codes
----------
    0   every mne:: symbol is defined once
    1   symbols are defined by more than one library (they are listed)
    2   no libraries found or nm failed
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

PREFIXES = ("_ZN3mne", "__ZN3mne", "_ZNK3mne", "__ZNK3mne")


def strong_symbols_darwin(nm_output: str) -> set[str]:
    """Strong external definitions from ``nm -gUm`` output (Mach-O)."""
    symbols = set()
    for line in nm_output.splitlines():
        if "weak" in line or " external " not in line:
            continue
        name = line.rsplit(" ", 1)[-1]
        if name.startswith(PREFIXES):
            symbols.add(name)
    return symbols


def strong_symbols_elf(nm_output: str) -> set[str]:
    """Strong external definitions from ``nm -D --defined-only`` output (ELF): types T, D, B, R."""
    symbols = set()
    for line in nm_output.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] in "TDBR" and parts[2].startswith(PREFIXES):
            symbols.add(parts[2])
    return symbols


def library_symbols(library: Path) -> set[str]:
    if sys.platform == "darwin":
        out = subprocess.run(["nm", "-gUm", str(library)], capture_output=True, text=True, check=True).stdout
        return strong_symbols_darwin(out)
    out = subprocess.run(["nm", "-D", "--defined-only", str(library)], capture_output=True, text=True, check=True).stdout
    return strong_symbols_elf(out)


def duplicates(per_library: dict[str, set[str]]) -> dict[str, list[str]]:
    owners: dict[str, list[str]] = defaultdict(list)
    for library, symbols in per_library.items():
        for symbol in symbols:
            owners[symbol].append(library)
    return {symbol: sorted(libs) for symbol, libs in owners.items() if len(libs) > 1}


def demangle(symbols: list[str]) -> list[str]:
    try:
        out = subprocess.run(["c++filt"], input="\n".join(symbols), capture_output=True, text=True, check=True).stdout
        return out.splitlines()
    except (OSError, subprocess.CalledProcessError):
        return symbols


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("lib_dir", type=Path, help="directory with the built libmne_* shared libraries")
    args = ap.parse_args(argv)

    libraries = sorted(p for p in args.lib_dir.glob("libmne_*") if p.suffix in (".so", ".dylib") and not p.is_symlink())
    if not libraries:
        print(f"no libmne_* shared libraries in {args.lib_dir}")
        return 2
    try:
        per_library = {lib.name: library_symbols(lib) for lib in libraries}
    except (OSError, subprocess.CalledProcessError) as exc:
        print(f"nm failed: {exc}")
        return 2

    found = duplicates(per_library)
    if not found:
        print(f"PASS: {len(libraries)} libraries, every mne:: symbol defined once.")
        return 0
    names = sorted(found)
    for name, readable in zip(names, demangle(names)):
        print(f"  {readable}: {', '.join(found[name])}")
    print(f"FAIL: {len(found)} mne:: symbol(s) defined by more than one library; keep one definition.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
