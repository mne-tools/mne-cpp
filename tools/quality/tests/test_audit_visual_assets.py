#!/usr/bin/env python3
# =============================================================================================================
#
# @file     test_audit_visual_assets.py
# @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
# @since    2.4.0
# @date     September, 2026
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#
# @brief    Tests for the documentation screenshot audit (T0.4).
#
# =============================================================================================================
"""Tests for ``tools/quality/audit_visual_assets.py``."""

from __future__ import annotations

import importlib.util
import struct
import sys
import tempfile
import unittest
import zlib
from pathlib import Path

_SCRIPT_PATH = Path(__file__).resolve().parents[1] / "audit_visual_assets.py"
_spec = importlib.util.spec_from_file_location("audit_visual_assets", _SCRIPT_PATH)
assert _spec is not None and _spec.loader is not None
audit = importlib.util.module_from_spec(_spec)
sys.modules["audit_visual_assets"] = audit
_spec.loader.exec_module(audit)

# The exact bytes the website workflows write as a placeholder; its IDAT checksum is wrong.
CI_PLACEHOLDER = (
    b"\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x01\x00\x00\x00\x01\x08\x02\x00\x00\x00\x90wS"
    b"\xde\x00\x00\x00\x0cIDATx\x9cc\xf8\x0f\x00\x00\x01\x01\x00\x05\x18\xd8N\x00\x00\x00\x00IEND\xaeB`\x82"
)


def _png(width: int, height: int, pixel) -> bytes:
    def chunk(kind: bytes, payload: bytes) -> bytes:
        return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload))

    rows = b"".join(b"\x00" + b"".join(bytes(pixel(x, y)) for x in range(width)) for y in range(height))
    header = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b"")


class PngTests(unittest.TestCase):
    def validate(self, files: dict[str, bytes], declared: dict[str, list[int]]) -> dict[str, list[str]]:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for name, data in files.items():
                (root / f"{name}.png").parent.mkdir(parents=True, exist_ok=True)
                (root / f"{name}.png").write_bytes(data)
            manifest = {"shots": [{"id": key, "size": size} for key, size in declared.items()]}
            return {r["id"]: r["problems"] for r in audit.validate_images(root, manifest)}

    def test_real_image_passes(self) -> None:
        image = _png(32, 20, lambda x, y: (x * 7 % 256, y * 11 % 256, 90))
        self.assertEqual(self.validate({"app/ok": image}, {"app/ok": [32, 20]}), {"app/ok": []})

    def test_ci_placeholder_is_rejected(self) -> None:
        problems = self.validate({"app/p": CI_PLACEHOLDER}, {"app/p": [1280, 800]})["app/p"]
        self.assertIn("1x1 placeholder", problems)
        self.assertIn("corrupt image data", problems)

    def test_blank_wrong_size_orphan_and_non_png_are_rejected(self) -> None:
        blank = _png(16, 16, lambda x, y: (255, 255, 255))
        problems = self.validate(
            {"a/blank": blank, "a/size": _png(8, 8, lambda x, y: (x * 30, y * 30, 0)), "a/orphan": blank,
             "a/text": b"not an image"},
            {"a/blank": [16, 16], "a/size": [16, 16], "a/text": [16, 16]},
        )
        self.assertEqual(problems["a/blank"], ["uniform colour"])
        self.assertEqual(problems["a/size"], ["size [8, 8] differs from declared [16, 16]"])
        self.assertIn("no manifest producer", problems["a/orphan"])
        self.assertEqual(problems["a/text"], ["not a PNG"])


class RepositoryTests(unittest.TestCase):
    def test_every_referenced_image_is_classified(self) -> None:
        report = audit.build_report()
        referenced = set(audit.consumers(audit.DOCS_DIR))
        self.assertTrue(referenced <= {record["id"] for record in report["images"]})
        self.assertIn("mne_inspect_app", report["summary"]["implemented_kinds"])


if __name__ == "__main__":
    unittest.main()
