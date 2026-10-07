#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     October, 2026
#
#
# @brief    Validate the documentation screenshot manifest and the PNGs generated from it (T5.2).
"""Validate ``doc/website/screenshots/manifest.json`` and the screenshots it produces.

Manifest checks
    schema (known keys and types), unique ids (also case-insensitively, as the
    ids become file names), a shot kind that ``mne_doc_shots`` implements, at
    least one documentation page consuming each shot, and no page referencing
    an image without a producer.  Optional ``golden`` metadata must name an
    existing PNG of the declared size.

Image checks (skipped with ``--skip-images``)
    every shot has a PNG, which must decode completely (signature, chunk CRCs,
    zlib stream, scanline filters), have the declared size, be fully opaque
    and not be blank (one colour covering almost the whole frame).  PNGs
    without a manifest entry are orphans.  A shot with ``golden`` metadata
    must match its golden: normalised RMSE <= ``max_rmse`` and at most
    ``MAX_CHANGED_FRACTION`` of the pixels differing by more than
    ``CHANGED_LEVEL`` in any channel.

Only the standard library is used, so the result is identical on every platform.

Usage
-----
    python3 tools/quality/validate_screenshots.py --manifest doc/website/screenshots/manifest.json --root doc/website
    python3 tools/quality/validate_screenshots.py ... --images <dir>   # e.g. a CI artifact
    python3 tools/quality/validate_screenshots.py ... --skip-images    # manifest and references only

Exit codes
----------
    0   valid
    1   problems found (each is listed)
    2   the manifest cannot be read
"""

from __future__ import annotations

import argparse
import json
import re
import struct
import sys
import zlib
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_MANIFEST = REPO_ROOT / "doc" / "website" / "screenshots" / "manifest.json"
DEFAULT_ROOT = REPO_ROOT / "doc" / "website"
DEFAULT_RUNNER = REPO_ROOT / "doc" / "tools" / "doc_shots" / "shot_runner.cpp"

ROOT_KEYS = {"_doc": str, "out_dir": str, "shots": list}
SHOT_KEYS = {"id": str, "kind": str, "size": list, "setup": dict, "requires_sample_data": bool, "golden": dict}
GOLDEN_KEYS = {"path": str, "max_rmse": (int, float), "platform": str}
ID_RE = re.compile(r"^[a-z0-9]+(?:-[a-z0-9]+)*(?:/[a-z0-9]+(?:-[a-z0-9]+)*)*$")
REFERENCE_RE = re.compile(r"/img/manual/auto/([^\s)\"'`]+?)\.png")
KIND_RE = re.compile(r"spec\.kind\s*==\s*QLatin1String\(\"([a-z_0-9]+)\"\)")
MIN_SIDE, MAX_SIDE = 64, 8192
# A real application frame never has one colour on more than this share of its pixels.
BLANK_FRACTION = 0.995
# Golden comparison: antialiasing noise stays below this level; real changes do not.
CHANGED_LEVEL = 16
MAX_CHANGED_FRACTION = 0.005

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
CHANNELS = {0: 1, 2: 3, 4: 2, 6: 4}


class PngError(ValueError):
    pass


@dataclass
class Image:
    width: int
    height: int
    channels: int
    pixels: bytes = field(repr=False)


def _paeth(a: int, b: int, c: int) -> int:
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    return a if pa <= pb and pa <= pc else b if pb <= pc else c


def _unfilter(raw: bytes, height: int, stride: int, bpp: int) -> bytes:
    out = bytearray(height * stride)
    prev = bytearray(stride)
    pos = 0
    for y in range(height):
        kind = raw[pos]
        line = bytearray(raw[pos + 1:pos + 1 + stride])
        pos += stride + 1
        if kind == 1:
            for i in range(bpp, stride):
                line[i] = (line[i] + line[i - bpp]) & 0xFF
        elif kind == 2:
            line = bytearray((a + b) & 0xFF for a, b in zip(line, prev))
        elif kind == 3:
            for i in range(stride):
                left = line[i - bpp] if i >= bpp else 0
                line[i] = (line[i] + ((left + prev[i]) >> 1)) & 0xFF
        elif kind == 4:
            for i in range(stride):
                left = line[i - bpp] if i >= bpp else 0
                corner = prev[i - bpp] if i >= bpp else 0
                line[i] = (line[i] + _paeth(left, prev[i], corner)) & 0xFF
        elif kind != 0:
            raise PngError(f"invalid scanline filter {kind} in row {y}")
        out[y * stride:(y + 1) * stride] = line
        prev = line
    return bytes(out)


def decode_png(data: bytes) -> Image:
    """Decode an 8-bit, non-interlaced PNG completely; raise PngError on anything malformed."""
    if data[:8] != PNG_SIGNATURE:
        raise PngError("not a PNG")
    offset, header, idat, ended = 8, None, bytearray(), False
    while offset + 12 <= len(data):
        length, kind = struct.unpack(">I4s", data[offset:offset + 8])
        payload = data[offset + 8:offset + 8 + length]
        if len(payload) != length or offset + 12 + length > len(data):
            raise PngError("truncated chunk")
        (crc,) = struct.unpack(">I", data[offset + 8 + length:offset + 12 + length])
        if zlib.crc32(kind + payload) != crc:
            raise PngError(f"bad CRC in {kind.decode('latin-1')} chunk")
        if kind == b"IHDR":
            header = struct.unpack(">IIBBBBB", payload)
        elif kind == b"IDAT":
            idat += payload
        elif kind == b"IEND":
            ended = True
            break
        offset += 12 + length
    if header is None or not ended:
        raise PngError("missing IHDR or IEND chunk")
    width, height, depth, colour, _, _, interlace = header
    if depth != 8 or colour not in CHANNELS or interlace:
        raise PngError(f"unsupported PNG format (depth {depth}, colour type {colour}, interlace {interlace}); "
                       "expected 8-bit non-interlaced grey/RGB(A)")
    if width == 0 or height == 0:
        raise PngError("empty image")
    channels = CHANNELS[colour]
    stride = width * channels
    try:
        raw = zlib.decompress(bytes(idat))
    except zlib.error as exc:
        raise PngError(f"corrupt image data ({exc})") from None
    if len(raw) != height * (stride + 1):
        raise PngError(f"image data has {len(raw)} bytes, expected {height * (stride + 1)}")
    return Image(width, height, channels, _unfilter(raw, height, stride, channels))


def image_problems(image: Image, declared: list[int] | None) -> list[str]:
    problems = []
    if declared and [image.width, image.height] != declared:
        problems.append(f"size {image.width}x{image.height} differs from declared {declared[0]}x{declared[1]}")
    total = image.width * image.height
    if image.channels in (2, 4):
        alpha = image.pixels[image.channels - 1::image.channels]
        translucent = total - alpha.count(255)
        if translucent:
            problems.append(f"not opaque: {translucent / total:.1%} of pixels have alpha < 255")
    colour = image.channels - 1 if image.channels in (2, 4) else image.channels
    planes = [image.pixels[c::image.channels] for c in range(colour)]
    dominant = Counter(zip(*planes)).most_common(1)[0][1]
    if dominant / total >= BLANK_FRACTION:
        problems.append(f"blank: one colour covers {dominant / total:.1%} of the frame")
    return problems


def golden_problems(image: Image, golden: Image, max_rmse: float) -> list[str]:
    if (image.width, image.height, image.channels) != (golden.width, golden.height, golden.channels):
        return [f"differs from golden format {golden.width}x{golden.height}x{golden.channels}"]
    diff = bytes(abs(a - b) for a, b in zip(image.pixels, golden.pixels))
    rmse = (sum(d * d for d in diff) / len(diff)) ** 0.5 / 255
    mask = diff.translate(bytes(int(level > CHANGED_LEVEL) for level in range(256)))
    changed_bits = 0
    for c in range(image.channels):
        changed_bits |= int.from_bytes(mask[c::image.channels], "big")
    changed = bin(changed_bits).count("1") / (image.width * image.height)
    if rmse <= max_rmse and changed <= MAX_CHANGED_FRACTION:
        return []
    return [f"regression vs golden: RMSE {rmse:.4f} (max {max_rmse}), "
            f"{changed:.2%} of pixels changed (max {MAX_CHANGED_FRACTION:.2%})"]


def implemented_kinds(runner: Path) -> set[str]:
    return set(KIND_RE.findall(runner.read_text(encoding="utf-8"))) if runner.is_file() else set()


def references(docs_dir: Path) -> dict[str, list[str]]:
    found: dict[str, set[str]] = {}
    for page in sorted(docs_dir.rglob("*.md*")):
        for shot_id in REFERENCE_RE.findall(page.read_text(encoding="utf-8", errors="replace")):
            found.setdefault(shot_id, set()).add(page.relative_to(docs_dir).as_posix())
    return {shot_id: sorted(pages) for shot_id, pages in found.items()}


def _type_problems(where: str, obj: dict[str, Any], schema: dict[str, Any], required: tuple[str, ...]) -> list[str]:
    problems = [f"{where}: missing '{key}'" for key in required if key not in obj]
    for key, value in obj.items():
        expected = schema.get(key)
        if expected is None:
            problems.append(f"{where}: unknown key '{key}'")
        elif not isinstance(value, expected) or (isinstance(value, bool) and expected is not bool):
            problems.append(f"{where}: '{key}' has the wrong type")
    return problems


def validate_manifest(manifest: Any, manifest_dir: Path, kinds: set[str],
                      consumers: dict[str, list[str]]) -> tuple[list[str], dict[str, dict[str, Any]]]:
    """Return (problems, valid shots by id)."""
    if not isinstance(manifest, dict):
        return ["manifest: root must be an object"], {}
    problems = _type_problems("manifest", manifest, ROOT_KEYS, ("out_dir", "shots"))
    shots: dict[str, dict[str, Any]] = {}
    folded: dict[str, str] = {}
    entries = manifest.get("shots") if isinstance(manifest.get("shots"), list) else []
    if not entries:
        problems.append("manifest: 'shots' must be a non-empty list")
    for index, shot in enumerate(entries):
        if not isinstance(shot, dict):
            problems.append(f"shots[{index}]: must be an object")
            continue
        shot_id = shot.get("id") if isinstance(shot.get("id"), str) else f"shots[{index}]"
        where = f"shot '{shot_id}'"
        own = _type_problems(where, shot, SHOT_KEYS, ("id", "kind", "size"))
        if isinstance(shot.get("id"), str) and not ID_RE.match(shot_id):
            own.append(f"{where}: id must be lower-case kebab-case path segments")
        size = shot.get("size")
        if isinstance(size, list) and not (len(size) == 2 and all(type(v) is int and MIN_SIDE <= v <= MAX_SIDE
                                                                  for v in size)):
            own.append(f"{where}: size must be [width, height] with {MIN_SIDE}..{MAX_SIDE} px each")
        if isinstance(shot.get("kind"), str) and shot["kind"] not in kinds:
            own.append(f"{where}: kind '{shot['kind']}' is not implemented by mne_doc_shots")
        if shot_id.lower() in folded:
            own.append(f"{where}: duplicate output name (also '{folded[shot_id.lower()]}')")
        if isinstance(shot.get("id"), str) and shot_id not in consumers:
            own.append(f"{where}: no documentation page references /img/manual/auto/{shot_id}.png")
        golden = shot.get("golden")
        if isinstance(golden, dict):
            own += _type_problems(f"{where} golden", golden, GOLDEN_KEYS, ("path", "max_rmse", "platform"))
            rmse = golden.get("max_rmse")
            if isinstance(rmse, (int, float)) and not isinstance(rmse, bool) and not 0 <= rmse <= 1:
                own.append(f"{where} golden: max_rmse must be within 0..1")
            if isinstance(golden.get("path"), str) and not own:
                own += [f"{where} golden: {p}" for p in _file_problems(manifest_dir / golden["path"], size)]
        problems += own
        folded.setdefault(shot_id.lower(), shot_id)
        if not own:
            shots[shot_id] = shot
    for shot_id, pages in sorted(consumers.items()):
        if shot_id not in {s.get("id") for s in entries if isinstance(s, dict)}:
            problems.append(f"image '{shot_id}': referenced by {', '.join(pages)} but no manifest shot produces it")
    return problems, shots


def _file_problems(path: Path, declared: list[int] | None) -> list[str]:
    if not path.is_file():
        return [f"missing {path.name}"]
    try:
        return image_problems(decode_png(path.read_bytes()), declared)
    except PngError as exc:
        return [str(exc)]


def validate_images(images_dir: Path, shots: dict[str, dict[str, Any]], declared_ids: set[str],
                    manifest_dir: Path) -> list[str]:
    problems = []
    for shot_id, shot in sorted(shots.items()):
        path = images_dir / f"{shot_id}.png"
        if not path.is_file():
            problems.append(f"image '{shot_id}': missing ({path})")
            continue
        own = _file_problems(path, shot["size"])
        golden = shot.get("golden")
        if golden and not own:
            own = golden_problems(decode_png(path.read_bytes()),
                                  decode_png((manifest_dir / golden["path"]).read_bytes()), golden["max_rmse"])
        problems += [f"image '{shot_id}': {p}" for p in own]
    for path in sorted(images_dir.rglob("*.png")) if images_dir.is_dir() else []:
        shot_id = path.relative_to(images_dir).with_suffix("").as_posix()
        if shot_id not in declared_ids:
            problems.append(f"image '{shot_id}': orphan file without a manifest shot")
    return problems


def validate(manifest_path: Path, root: Path, runner: Path, images_dir: Path | None,
             check_images: bool) -> list[str]:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    problems, shots = validate_manifest(manifest, manifest_path.parent, implemented_kinds(runner),
                                        references(root / "docs"))
    if check_images:
        if images_dir is None:
            out_dir = manifest.get("out_dir") if isinstance(manifest, dict) else None
            if not isinstance(out_dir, str):
                return problems
            images_dir = (manifest_path.parent / out_dir).resolve()
        declared_ids = {shot.get("id") for shot in manifest.get("shots") or [] if isinstance(shot, dict)}
        problems += validate_images(images_dir, shots, declared_ids, manifest_path.parent)
    return problems


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--root", type=Path, default=DEFAULT_ROOT, help="website root containing docs/")
    parser.add_argument("--runner", type=Path, default=DEFAULT_RUNNER, help="mne_doc_shots kind dispatcher")
    parser.add_argument("--images", type=Path, help="directory with the generated PNGs (default: manifest out_dir)")
    parser.add_argument("--skip-images", action="store_true", help="check the manifest and references only")
    args = parser.parse_args(argv)

    try:
        problems = validate(args.manifest, args.root, args.runner, args.images, not args.skip_images)
    except (OSError, json.JSONDecodeError) as exc:
        print(f"error: cannot read manifest {args.manifest}: {exc}", file=sys.stderr)
        return 2
    if problems:
        print("\n".join(problems))
        print(f"FAIL: {len(problems)} screenshot problem(s).")
        return 1
    print(f"PASS: screenshot manifest {'and images ' if not args.skip_images else ''}valid.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
