#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Release checksums and signing evidence (v2.4.0 T10.7c).

Signatures can only be checked on the platform that produced them, so each
installer job records its own artifact::

    python3 tools/release/artifact_evidence.py inspect FILE... --out evidence.json

and one final job, with every release asset downloaded, publishes the result::

    python3 tools/release/artifact_evidence.py publish --assets DIR \\
        --evidence evidence-*.json --sums SHA256SUMS --report release-evidence.json \\
        --notes release-notes.md

``publish`` hashes every asset, fails when a recorded installer is not the
asset that was published, and replaces the verification section of the
release notes so that reruns do not stack copies.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

NOTES_BEGIN = "<!-- release-evidence:begin -->"
NOTES_END = "<!-- release-evidence:end -->"

_DESCRIPTION = {
    "signed-notarized": "Developer ID signed, notarized and stapled",
    "signed": "signed",
    "unsigned": "**unsigned**",
    "unverified": "**signature not verified**",
    "checksum-only": "SHA-256 checksum",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def _ok(command: list[str]) -> bool:
    return subprocess.run(command, capture_output=True, check=False).returncode == 0


_WINDOWS_SIGNED = (".exe", ".msi")
_MACOS_SIGNED = (".dmg", ".pkg", ".app")


def signature(path: Path) -> str:
    """Return the signature status of one artifact, checked on this machine."""
    suffix = path.suffix.lower()
    if suffix in _WINDOWS_SIGNED:
        if not shutil.which("powershell"):
            return "unverified"
        quoted = str(path).replace("'", "''")
        script = (
            f"$s = Get-AuthenticodeSignature -LiteralPath '{quoted}'; "
            "if ($s.Status -ne 'Valid' -or -not $s.TimeStamperCertificate) { exit 1 }"
        )
        return "signed" if _ok(["powershell", "-NoProfile", "-Command", script]) else "unsigned"
    if suffix in _MACOS_SIGNED:
        if not shutil.which("codesign"):
            return "unverified"
        if not _ok(["codesign", "--verify", "--strict", str(path)]):
            return "unsigned"
        notarized = _ok(["xcrun", "stapler", "validate", str(path)]) and _ok(
            ["spctl", "--assess", "--type", "open", "--context",
             "context:primary-signature", str(path)])
        return "signed-notarized" if notarized else "signed"
    return "checksum-only"


def inspect(files: list[Path]) -> list[dict]:
    return [{"name": f.name, "sha256": sha256(f), "size": f.stat().st_size,
             "signature": signature(f)} for f in files]


def publish(assets: Path, evidence: list[dict]) -> list[dict]:
    """Hash every asset and merge the per-platform signing records.

    A signable asset that no platform job inspected is ``unverified``.
    """
    recorded = {entry["name"]: entry for entry in evidence}
    records = []
    for path in sorted(p for p in assets.iterdir() if p.is_file()):
        signable = path.suffix.lower() in _WINDOWS_SIGNED + _MACOS_SIGNED
        record = {"name": path.name, "sha256": sha256(path), "size": path.stat().st_size,
                  "signature": "unverified" if signable else "checksum-only"}
        entry = recorded.pop(path.name, None)
        if entry is not None:
            if entry["sha256"] != record["sha256"]:
                raise ValueError(f"{path.name}: published asset differs from the "
                                 "inspected one")
            record["signature"] = entry["signature"]
        records.append(record)
    if recorded:
        raise ValueError("inspected but not published: " + ", ".join(sorted(recorded)))
    return records


def notes_section(records: list[dict]) -> str:
    lines = [NOTES_BEGIN, "### Verifying downloads", "",
             "Every asset is listed with its SHA-256 in `SHA256SUMS` "
             "(`sha256sum -c SHA256SUMS --ignore-missing`, or `shasum -a 256 -c` on "
             "macOS).", "", "| Asset | Verification |", "| --- | --- |"]
    lines += [f"| `{r['name']}` | {_DESCRIPTION[r['signature']]} |" for r in records
              if r["signature"] != "checksum-only"]
    lines += ["| all other assets | SHA-256 checksum |", NOTES_END]
    return "\n".join(lines)


def splice_notes(body: str, section: str) -> str:
    pattern = re.compile(re.escape(NOTES_BEGIN) + ".*?" + re.escape(NOTES_END), re.S)
    if pattern.search(body):
        return pattern.sub(lambda _: section, body)
    return (body.rstrip() + "\n\n" + section).lstrip() + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    commands = parser.add_subparsers(dest="command", required=True)
    inspect_parser = commands.add_parser("inspect", help="record artifacts here")
    inspect_parser.add_argument("files", nargs="+", type=Path)
    inspect_parser.add_argument("--out", type=Path, required=True)
    publish_parser = commands.add_parser("publish", help="checksum all assets")
    publish_parser.add_argument("--assets", type=Path, required=True)
    publish_parser.add_argument("--evidence", type=Path, nargs="*", default=[])
    publish_parser.add_argument("--sums", type=Path, required=True)
    publish_parser.add_argument("--report", type=Path, required=True)
    publish_parser.add_argument("--notes", type=Path, required=True,
                                help="release notes, updated in place")
    args = parser.parse_args(argv)

    if args.command == "inspect":
        args.out.write_text(json.dumps(inspect(args.files), indent=2) + "\n")
        return 0
    evidence = [entry for path in args.evidence for entry in json.loads(path.read_text())]
    try:
        records = publish(args.assets, evidence)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    args.sums.write_text("".join(f"{r['sha256']}  {r['name']}\n" for r in records))
    args.report.write_text(json.dumps(records, indent=2) + "\n")
    body = args.notes.read_text() if args.notes.exists() else ""
    args.notes.write_text(splice_notes(body, notes_section(records)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
