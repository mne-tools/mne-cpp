#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     October, 2026
#
#
# @brief    Fixtures for the documentation screenshot validator (T5.2).
"""Fixtures for ``tools/quality/validate_screenshots.py``."""

from __future__ import annotations

import importlib.util
import json
import shutil
import struct
import sys
import tempfile
import unittest
import zlib
from pathlib import Path

_QUALITY_DIR = Path(__file__).resolve().parent.parent
_spec = importlib.util.spec_from_file_location("validate_screenshots", _QUALITY_DIR / "validate_screenshots.py")
assert _spec is not None and _spec.loader is not None
shots = importlib.util.module_from_spec(_spec)
sys.modules["validate_screenshots"] = shots
_spec.loader.exec_module(shots)

W, H = 96, 64


def _filter(kind: int, line: bytes, prev: bytes, bpp: int) -> bytes:
    def left(i: int, row: bytes) -> int:
        return row[i - bpp] if i >= bpp else 0

    if kind == 1:
        return bytes((line[i] - left(i, line)) & 0xFF for i in range(len(line)))
    if kind == 2:
        return bytes((a - b) & 0xFF for a, b in zip(line, prev))
    if kind == 3:
        return bytes((line[i] - ((left(i, line) + prev[i]) >> 1)) & 0xFF for i in range(len(line)))
    if kind == 4:
        return bytes((line[i] - shots._paeth(left(i, line), prev[i], left(i, prev))) & 0xFF
                     for i in range(len(line)))
    return line


def chunk(kind: bytes, payload: bytes) -> bytes:
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload))


def png(width: int, height: int, pixel, colour: int = 2) -> bytes:
    """Encode an 8-bit PNG, cycling through all five scanline filters."""
    bpp = shots.CHANNELS[colour]
    prev, raw = bytes(width * bpp), bytearray()
    for y in range(height):
        line = b"".join(bytes(pixel(x, y)) for x in range(width))
        raw += bytes([y % 5]) + _filter(y % 5, line, prev, bpp)
        prev = line
    header = struct.pack(">IIBBBBB", width, height, 8, colour, 0, 0, 0)
    return (shots.PNG_SIGNATURE + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(bytes(raw)))
            + chunk(b"IEND", b""))


def scene(x: int, y: int) -> tuple[int, int, int]:
    return (x * 5) % 256, (y * 9) % 256, (x * y) % 256


VALID = png(W, H, scene)
# Well-formed chunks whose image data is not a zlib stream.
CORRUPT = (shots.PNG_SIGNATURE + chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0))
           + chunk(b"IDAT", b"not deflate data") + chunk(b"IEND", b""))


class DecoderTests(unittest.TestCase):
    def test_all_filters_round_trip(self) -> None:
        image = shots.decode_png(VALID)
        expected = b"".join(bytes(scene(x, y)) for y in range(H) for x in range(W))
        self.assertEqual((image.width, image.height, image.channels), (W, H, 3))
        self.assertEqual(image.pixels, expected)

    def test_rgba_round_trip(self) -> None:
        image = shots.decode_png(png(5, 4, lambda x, y: (x, y, 7, 255), colour=6))
        self.assertEqual(image.pixels[:8], bytes([0, 0, 7, 255, 1, 0, 7, 255]))

    def test_malformed_files_raise(self) -> None:
        corrupt_crc = bytearray(VALID)
        corrupt_crc[40] ^= 0xFF
        cases = {
            b"GIF89a": "not a PNG",
            VALID[:60]: "truncated",
            bytes(corrupt_crc): "bad CRC",
            CORRUPT: "corrupt image data",
        }
        for data, message in cases.items():
            with self.assertRaisesRegex(shots.PngError, message):
                shots.decode_png(data)


class TreeCase(unittest.TestCase):
    """A website root with one guide page, a manifest and generated images."""

    def setUp(self) -> None:
        self.root = Path(tempfile.mkdtemp())
        (self.root / "docs").mkdir()
        (self.root / "screenshots").mkdir()
        self.images = self.root / "static" / "img" / "manual" / "auto"
        self.runner = self.root / "shot_runner.cpp"
        self.runner.write_text('if (spec.kind == QLatin1String("app")) {}\n', encoding="utf-8")
        self.shots: list[dict] = []

    def tearDown(self) -> None:
        shutil.rmtree(self.root)

    def add(self, shot_id: str, data: bytes | None = VALID, reference: bool = True, **extra) -> None:
        self.shots.append({"id": shot_id, "kind": "app", "size": [W, H], **extra})
        if reference:
            with (self.root / "docs" / "guide.mdx").open("a", encoding="utf-8") as page:
                page.write(f"![{shot_id}](/img/manual/auto/{shot_id}.png)\n")
        if data is not None:
            self.write(shot_id, data)

    def write(self, shot_id: str, data: bytes) -> None:
        path = self.images / f"{shot_id}.png"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)

    def problems(self, manifest: dict | None = None) -> list[str]:
        manifest_path = self.root / "screenshots" / "manifest.json"
        manifest = manifest or {"out_dir": "../static/img/manual/auto", "shots": self.shots}
        manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
        return shots.validate(manifest_path, self.root, self.runner, None, True)


class ImageFixtureTests(TreeCase):
    def test_valid_fixture_passes(self) -> None:
        self.add("app/overview")
        self.add("app/rgba", png(W, H, lambda x, y: (*scene(x, y), 255), colour=6))
        self.assertEqual(self.problems(), [])
        self.assertEqual(shots.main(["--manifest", str(self.root / "screenshots" / "manifest.json"),
                                     "--root", str(self.root), "--runner", str(self.runner)]), 0)

    def test_bad_images_fail(self) -> None:
        cases = {
            "app/one-pixel": (png(1, 1, lambda x, y: (0, 0, 0)), "size 1x1 differs"),
            "app/corrupt": (CORRUPT, "corrupt image data"),
            "app/blank": (png(W, H, lambda x, y: (255, 255, 255)), "blank"),
            "app/transparent": (png(W, H, lambda x, y: (*scene(x, y), 0), colour=6), "not opaque"),
            "app/missing": (None, "missing"),
        }
        for shot_id, (data, _) in cases.items():
            self.add(shot_id, data)
        self.write("app/orphan", VALID)
        problems = "\n".join(self.problems())
        for shot_id, (_, message) in cases.items():
            self.assertRegex(problems, rf"image '{shot_id}': .*({message})")
        self.assertIn("image 'app/orphan': orphan", problems)

    def test_mostly_blank_frame_fails_but_sparse_ui_passes(self) -> None:
        self.add("app/dot", png(W, H, lambda x, y: (0, 0, 0) if (x, y) == (3, 3) else (240, 240, 240)))
        self.add("app/sparse", png(W, H, lambda x, y: (0, 0, 0) if y < 4 else (240, 240, 240)))
        problems = self.problems()
        self.assertEqual(len(problems), 1)
        self.assertIn("image 'app/dot': blank", problems[0])


class ManifestFixtureTests(TreeCase):
    def test_schema_and_producer_problems(self) -> None:
        self.add("app/ok")
        self.add("App/Ok", reference=False)
        self.add("app/kind", kind="unknown_kind")
        self.add("app/size", size=[1, 1])
        self.add("app/typo", setpu={})
        self.add("app/unused", reference=False)
        (self.root / "docs" / "other.md").write_text("![x](/img/manual/auto/app/ghost.png)\n", encoding="utf-8")
        problems = "\n".join(self.problems())
        self.assertIn("shot 'App/Ok': id must be lower-case", problems)
        self.assertIn("shot 'app/kind': kind 'unknown_kind' is not implemented", problems)
        self.assertIn("shot 'app/size': size must be", problems)
        self.assertIn("shot 'app/typo': unknown key 'setpu'", problems)
        self.assertIn("shot 'app/unused': no documentation page references", problems)
        self.assertIn("image 'app/ghost': referenced by other.md but no manifest shot", problems)
        self.assertNotIn("shot 'app/ok'", problems)
        self.assertNotIn("orphan", problems)

    def test_duplicate_output_names(self) -> None:
        self.add("app/twin")
        self.shots.append(dict(self.shots[0]))
        self.assertIn("shot 'app/twin': duplicate output name", "\n".join(self.problems()))

    def test_golden_metadata(self) -> None:
        (self.root / "screenshots" / "goldens").mkdir()
        (self.root / "screenshots" / "goldens" / "ok.png").write_bytes(VALID)
        self.add("app/golden", golden={"path": "goldens/ok.png", "max_rmse": 0.01, "platform": "ubuntu-24.04"})
        self.add("app/no-golden", golden={"path": "goldens/none.png", "max_rmse": 2, "platform": "x"})
        problems = "\n".join(self.problems())
        self.assertNotIn("'app/golden'", problems)
        self.assertIn("shot 'app/no-golden' golden: max_rmse must be within 0..1", problems)

    def test_golden_comparison(self) -> None:
        (self.root / "screenshots" / "goldens").mkdir()
        (self.root / "screenshots" / "goldens" / "g.png").write_bytes(VALID)
        golden = {"path": "goldens/g.png", "max_rmse": 0.01, "platform": "ubuntu-24.04"}
        # One pixel off by 200 levels: below both thresholds (antialiasing-like noise).
        self.add("app/noise", png(W, H, lambda x, y: (200, 0, 0) if (x, y) == (3, 3) else scene(x, y)),
                 golden=golden)
        # A 10x10 block changed: 1.6% of the pixels.
        self.add("app/block", png(W, H, lambda x, y: (255, 255, 255) if x < 10 and y < 10 else scene(x, y)),
                 golden=golden)
        # Every pixel off by 8 levels: below the changed level, but RMSE 0.031.
        self.add("app/shift", png(W, H, lambda x, y: tuple(min(c + 8, 255) for c in scene(x, y))), golden=golden)
        problems = "\n".join(self.problems())
        self.assertNotIn("app/noise", problems)
        self.assertIn("image 'app/block': regression vs golden", problems)
        self.assertIn("1.63% of pixels changed", problems)
        self.assertRegex(problems, r"image 'app/shift': regression vs golden: RMSE 0\.0[23]\d+ \(max 0\.01\), 0\.00%")

    def test_golden_size_mismatch(self) -> None:
        golden = shots.decode_png(png(W // 2, H, scene))
        self.assertEqual(shots.golden_problems(shots.decode_png(VALID), golden, 0.01),
                         [f"differs from golden format {W // 2}x{H}x3"])

    def test_missing_golden_file(self) -> None:
        self.add("app/g", golden={"path": "goldens/none.png", "max_rmse": 0.01, "platform": "ubuntu-24.04"})
        self.assertIn("shot 'app/g' golden: missing none.png", "\n".join(self.problems()))

    def test_unreadable_manifest_exits_2(self) -> None:
        (self.root / "screenshots" / "manifest.json").write_text("{", encoding="utf-8")
        self.assertEqual(shots.main(["--manifest", str(self.root / "screenshots" / "manifest.json")]), 2)


class RepositoryTests(unittest.TestCase):
    def test_committed_manifest_schema(self) -> None:
        manifest = json.loads(shots.DEFAULT_MANIFEST.read_text(encoding="utf-8"))
        problems, valid = shots.validate_manifest(manifest, shots.DEFAULT_MANIFEST.parent,
                                                  shots.implemented_kinds(shots.DEFAULT_RUNNER),
                                                  shots.references(shots.DEFAULT_ROOT / "docs"))
        self.assertEqual(len(valid), len(manifest["shots"]), problems)


if __name__ == "__main__":
    unittest.main()
