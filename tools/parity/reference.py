#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# @since    2.4.0
# @date     October, 2026
#
#
# @brief    Pinned MNE-Python reference environment for parity fixtures (T7.1).
"""Pinned MNE-Python reference environment for parity fixtures.

``reference_lock.json`` names the versions that CI installs and that every
committed fixture was generated with.  Fixture generators call
:func:`require_reference` first, so a fixture can only be regenerated with the
same major.minor versions; a differing patch version is reported but allowed.

Usage
-----
    python3 tools/parity/reference.py            # check the current environment

Moving the reference
--------------------
1. Edit ``reference_lock.json`` and the matching ``pip install`` pins in
   ``.github/workflows/_reusable-tests.yml`` (a test keeps them equal).
2. Set ``meta.mne_python_ref`` and ``parity.mne_python_pinned`` in
   ``doc/api_registry.json`` and rerun ``tools/parity/extract_mne_python_api.py``
   and ``tools/parity/gap_analysis.py``.
3. In the new environment run every generator listed by
   ``tools/parity/check_fixtures.py --list`` and then
   ``tools/parity/check_fixtures.py --update``; review the fixture diff.
4. Run the ``parity`` CTest label and fix or document every changed result.
"""

from __future__ import annotations

import json
import sys
from importlib import metadata
from pathlib import Path
from typing import Callable

LOCK_FILE = Path(__file__).resolve().parent / "reference_lock.json"


def load_lock(path: Path = LOCK_FILE) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def _minor(version: str) -> tuple[str, ...]:
    return tuple(version.split(".")[:2])


def check_reference(lock: dict | None = None,
                    installed: Callable[[str], str] = metadata.version,
                    python: str | None = None) -> tuple[list[str], list[str]]:
    """Return ``(errors, notes)``: major.minor mismatches are errors, patch mismatches notes."""
    lock = lock or load_lock()
    errors: list[str] = []
    notes: list[str] = []
    python = python or f"{sys.version_info.major}.{sys.version_info.minor}"
    if _minor(python) != _minor(lock["python"]):
        notes.append(f"Python {python} (reference {lock['python']})")
    for package, wanted in lock["packages"].items():
        try:
            have = installed(package)
        except metadata.PackageNotFoundError:
            errors.append(f"{package} is not installed (reference {wanted})")
            continue
        if _minor(have) != _minor(wanted):
            errors.append(f"{package} {have} (reference {wanted})")
        elif have != wanted:
            notes.append(f"{package} {have} (reference {wanted})")
    return errors, notes


def require_reference() -> None:
    """Exit unless the environment matches the reference major.minor versions."""
    errors, notes = check_reference()
    for note in notes:
        print(f"note: {note}", file=sys.stderr)
    if errors:
        sys.exit("Parity fixtures must be generated with the pinned reference "
                 f"({LOCK_FILE.name}): " + "; ".join(errors))


if __name__ == "__main__":
    require_reference()
    print("Reference environment OK:", ", ".join(f"{k} {v}" for k, v in load_lock()["packages"].items()))
