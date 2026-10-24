#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write doc/website/src/data/contributors.json from the git history.

Commits, merges included as on GitHub's contributor graph, are counted over the
whole history of the checked-out branch, with every author's identities merged
by ``.mailmap``, so the website ranks people by their actual commits rather
than by the GitHub accounts that happen to be linked to their e-mail addresses.
Needs a full clone (``fetch-depth: 0``)::

    python3 scripts/generate_contributors.py [--out FILE]
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
OUT_FILE = REPO_ROOT / "doc" / "website" / "src" / "data" / "contributors.json"

# Canonical .mailmap name -> GitHub login (for the avatar and profile link)
GITHUB_LOGINS = {
    "Christoph Dinh": "chdinh",
    "Lorenz Esch": "LorenzE",
    "Gabriel Motta": "gabrielbmotta",
    "Juan Garcia-Prieto": "juangpc",
    "Ruben Dörfel": "RDoerfel",
    "Lars Debor": "1DIce",
    "Viktor Klüber": "ViktorKL",
    "Ricky Tjen": "rickytjen",
    "Jana Kiesel": "JanaKiesel",
    "Martin Henfling": "LostSign",
    "Simon Heinke": "sheinke",
    "Limin Sun": "limin-sun",
    "Florian Schlembach": "floschl",
    "Daniel Knobl": "GBeret",
    "Daniel Strohmeier": "joewalter",
    "Andrey Parfenov": "Andrey1994",
    "Christof Pieloth": "cpieloth",
    "Tim Kunze": "TiKunze",
    "Wayne Mead": "imsorryk",
    "Felix Arndt": "farndt",
    "Louis Eichhorst": "louiseichhorst",
    "Johannes Vorwerk": "johaenns",
    "Johannes Behrens": "jobehrens",
    "Chiran Doshi": "cdoshi",
    "Felix Griesau": "femigr",
    "Andreas Griesshammer": "ag-fieldline",
    "Robert Dicamillo": "buildqa",
    "Alex Rockhill": "alexrockhill",
    "Erik Hornberger": "er06645810",
    "Sugandha Sachdeva": "SachdevaS",
    "Faris Yahya": "mfarisyahya",
    "Blerta Hamzallari": "betaha",
    "Eric Larson": "larsoner",
    "Julius Lerm": "Julius-L",
    "Marco Klamke": "MKlamke",
    "Petros Simidyan": "PetrosSimidyan",
    "Benjamin Kay": "benkay86",
    "Franco Polo": "fjpolo",
    "Mainak Jas": "jasmainak",
    "Martin Luessi": "mluessi",
}
# Bots and machine accounts are not people
EXCLUDED = re.compile(r"\[bot\]|^MNE CPP$|^GitHub$|^web-flow$")


def contributors(repo: Path, rev: str = "HEAD") -> list[dict]:
    log = subprocess.run(["git", "shortlog", "-sn", rev], cwd=repo, capture_output=True,
                         text=True, check=True).stdout
    people = []
    for line in log.splitlines():
        count, name = line.strip().split("\t", 1)
        if EXCLUDED.search(name):
            continue
        people.append({"name": name, "login": GITHUB_LOGINS.get(name), "contributions": int(count)})
    return sorted(people, key=lambda p: (-p["contributions"], p["name"]))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--out", type=Path, default=OUT_FILE)
    args = parser.parse_args(argv)
    if subprocess.run(["git", "rev-parse", "--is-shallow-repository"], cwd=REPO_ROOT, capture_output=True,
                      text=True, check=True).stdout.strip() == "true":
        print("error: shallow clone; fetch the full history (fetch-depth: 0)", file=sys.stderr)
        return 1
    people = contributors(REPO_ROOT)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(people, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"wrote {len(people)} contributors to {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
