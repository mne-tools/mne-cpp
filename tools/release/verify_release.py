#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Verify the published assets of a release on a clean machine (v2.4.0 T10.4).

Download the assets first (``gh release download TAG -D assets``), then::

    python3 tools/release/verify_release.py assets --prefix mne-cpp-2.4.0 \\
        [--platform linux|macos|windows] [--require-signed [--unsigned-macos]]

Every required asset must be present and match ``SHA256SUMS``, every asset must
be listed there, ``release-evidence.json`` must record the signature status of
the signable ones (signed and notarized with ``--require-signed``; with
``--unsigned-macos`` the ``.dmg`` images must be recorded as unsigned), and with
``--platform`` the dynamic archive is unpacked and one command-line tool must
start.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import struct
import subprocess
import sys
import tarfile
import tempfile
import zipfile
from pathlib import Path

REQUIRED = {
    "linux": ["linux-dynamic-x86_64.tar.gz", "linux-static-x86_64.tar.gz", "linux-installer-x86_64.run"],
    "macos": ["macos-dynamic-arm64.tar.gz", "macos-static-arm64.tar.gz", "macos-dynamic-arm64.dmg",
              "macos-installer-arm64.dmg"],
    "windows": ["windows-dynamic-x86_64.zip", "windows-static-x86_64.zip", "windows-installer-x86_64.exe"],
}
# Release signature status every signed asset must reach with --require-signed
SIGNED = {".exe": "signed", ".zip": "signed", ".dmg": "signed-notarized"}
SMOKE_TOOL = "mne_check_surface"
# Oldest macOS the release runs on: CMAKE_OSX_DEPLOYMENT_TARGET in src/CMakeLists.txt and the download page
MACOS_MINIMUM = "14.4"
_LC_BUILD_VERSION = 0x32


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def check_assets(assets: Path, prefix: str, platforms: list[str], require_signed: bool,
                 unsigned_macos: bool = False) -> list[str]:
    problems = []
    names = {p.name for p in assets.iterdir() if p.is_file()}
    for platform in platforms:
        problems += [f"{prefix}-{suffix} is missing" for suffix in REQUIRED[platform]
                     if f"{prefix}-{suffix}" not in names]
    if "SHA256SUMS" not in names:
        return problems + ["SHA256SUMS is missing"]
    sums = dict(reversed(line.split(maxsplit=1)) for line in (assets / "SHA256SUMS").read_text().splitlines()
                if line.strip())
    for name in sorted(names - {"SHA256SUMS", "release-evidence.json"}):
        if name not in sums:
            problems.append(f"{name} is not listed in SHA256SUMS")
        elif _sha256(assets / name) != sums[name]:
            problems.append(f"{name} does not match SHA256SUMS")
    if "release-evidence.json" not in names:
        return problems + ["release-evidence.json is missing"]
    evidence = {r["name"]: r["signature"] for r in json.loads((assets / "release-evidence.json").read_text())}
    for platform in platforms:
        for suffix in REQUIRED[platform]:
            name = f"{prefix}-{suffix}"
            wanted = "unsigned" if unsigned_macos and name.endswith(".dmg") else SIGNED.get(Path(name).suffix)
            if wanted and require_signed and name in names and evidence.get(name) != wanted:
                problems.append(f"{name} is {evidence.get(name, 'not recorded')}, expected {wanted}")
    return problems


def _slice_minos(stream, offset: int) -> list[tuple[int, int, int]]:
    stream.seek(offset)
    magic, _, _, _, ncmds, _, _ = struct.unpack("<7I", stream.read(28))
    if magic != 0xFEEDFACF:
        return []
    stream.seek(offset + 32)
    found = []
    for _ in range(ncmds):
        cmd, size = struct.unpack("<2I", stream.read(8))
        body = stream.read(size - 8)
        if cmd == _LC_BUILD_VERSION:
            minos = struct.unpack_from("<I", body, 4)[0]
            found.append((minos >> 16, (minos >> 8) & 0xFF, minos & 0xFF))
    return found


def _macho_minos(path: Path) -> list[tuple[int, int, int]]:
    """Minimum macOS of every 64-bit slice of a thin or universal Mach-O file."""
    with path.open("rb") as stream:
        head = stream.read(8)
        if len(head) < 8:
            return []
        if struct.unpack(">I", head[:4])[0] == 0xCAFEBABE:
            # Java class files share this magic; their "count" is a class file version (>= 45)
            count = struct.unpack(">I", head[4:])[0]
            if count > 8:
                return []
            offsets = [struct.unpack(">5I", stream.read(20))[2] for _ in range(count)]
            return [v for offset in offsets for v in _slice_minos(stream, offset)]
        return _slice_minos(stream, 0) if struct.unpack("<I", head[:4])[0] == 0xFEEDFACF else []


def macos_minimum_problems(root: Path, minimum: str) -> list[str]:
    """Report every Mach-O file under @p root that needs a newer macOS than @p minimum."""
    wanted = tuple(int(x) for x in minimum.split(".")) + (0,) * (3 - minimum.count(".") - 1)
    problems = []
    for path in sorted(p for p in root.rglob("*") if p.is_file() and not p.is_symlink()):
        newest = max(_macho_minos(path), default=None)
        if newest and newest > wanted:
            shown = ".".join(map(str, newest[:3] if newest[2] else newest[:2]))
            problems.append(f"{path.relative_to(root).as_posix()} needs macOS {shown}, documented minimum is {minimum}")
    return problems


def smoke(archive: Path, workdir: Path) -> list[str]:
    """Unpack @p archive and run one bundled command-line tool with --help."""
    if archive.suffix == ".zip":
        with zipfile.ZipFile(archive) as z:
            z.extractall(workdir)
    else:
        with tarfile.open(archive) as t:
            escaping = [f"{archive.name}: {m.name} links outside the archive ({m.linkname})" for m in t.getmembers()
                        if m.issym() and (os.path.isabs(m.linkname) or os.path.normpath(
                            os.path.join(os.path.dirname(m.name), m.linkname)).startswith(".."))]
            if escaping:
                return escaping
            t.extractall(workdir, filter="tar")
    tools = [p for p in workdir.rglob(f"{SMOKE_TOOL}*") if p.is_file() and p.stem == SMOKE_TOOL]
    if not tools:
        return [f"{archive.name} contains no {SMOKE_TOOL}"]
    tool = tools[0]
    tool.chmod(tool.stat().st_mode | 0o111)
    env = dict(os.environ, QT_QPA_PLATFORM="offscreen")
    result = subprocess.run([str(tool), "--help"], capture_output=True, text=True, env=env, timeout=120, check=False)
    if result.returncode != 0:
        return [f"{tool.relative_to(workdir)} --help exited with {result.returncode}: {result.stderr.strip()[:500]}"]
    return []


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("assets", type=Path, help="directory holding the downloaded release assets")
    parser.add_argument("--prefix", required=True, help="asset name prefix, e.g. mne-cpp-2.4.0")
    parser.add_argument("--platform", choices=sorted(REQUIRED), help="also unpack and start this platform's build")
    parser.add_argument("--require-signed", action="store_true", help="fail unless signable assets are signed")
    parser.add_argument("--unsigned-macos", action="store_true",
                        help="the release ships macOS without a Developer ID (MACOS_SIGNING=unavailable)")
    args = parser.parse_args(argv)

    problems = check_assets(args.assets, args.prefix, sorted(REQUIRED), args.require_signed, args.unsigned_macos)
    if args.platform and not problems:
        archive = args.assets / f"{args.prefix}-{REQUIRED[args.platform][0]}"
        workdir = Path(tempfile.mkdtemp())
        try:
            problems += smoke(archive, workdir)
            if args.platform == "macos":
                problems += macos_minimum_problems(workdir, MACOS_MINIMUM)
        finally:
            shutil.rmtree(workdir, ignore_errors=True)
    for problem in problems:
        print(f"FAIL: {problem}")
    if not problems:
        print(f"PASS: {args.prefix} assets are complete, match SHA256SUMS"
              + (f" and the {args.platform} build starts." if args.platform else "."))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
