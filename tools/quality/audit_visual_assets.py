#!/usr/bin/env python3
# =============================================================================================================
#
# @file     audit_visual_assets.py
# @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
# @since    2.4.0
# @date     September, 2026
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#
# @brief    Reconcile documentation screenshots: producers, consumers, placeholders, and goldens.
#
# =============================================================================================================
"""Audit documentation screenshots for gate G4 (package T0.4).

For every image referenced as ``/img/manual/auto/<id>.png`` from the website
docs, and every entry of ``doc/website/screenshots/manifest.json``, report

* the producer: the manifest entry and whether ``mne_doc_shots`` implements its kind,
* the consumers: the pages that reference it,
* the declared size,

and for the repository as a whole

* which CI workflows fabricate placeholder PNGs instead of generating images,
* whether CI builds and runs ``mne_doc_shots``,
* whether the screenshot regression job compares against a golden (``--ref``).

Generated PNGs are not versioned (they live under an ignored directory), so the
committed report is derived from sources and workflows only.  ``--images``
additionally validates whatever PNGs exist locally or in a CI artifact:
decodable, declared size, not 1x1, not a single colour.

Usage
-----
    python3 tools/quality/audit_visual_assets.py
    python3 tools/quality/audit_visual_assets.py --images doc/website/static/img/manual/auto
"""

from __future__ import annotations

import argparse
import json
import re
import struct
import zlib
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[2]
MANIFEST = REPO_ROOT / "doc" / "website" / "screenshots" / "manifest.json"
DOCS_DIR = REPO_ROOT / "doc" / "website" / "docs"
SHOT_RUNNER = REPO_ROOT / "doc" / "tools" / "doc_shots" / "shot_runner.cpp"
WORKFLOWS_DIR = REPO_ROOT / ".github" / "workflows"

_REFERENCE_RE = re.compile(r"/img/manual/auto/([^\s)\"']+)\.png")
_KIND_RE = re.compile(r"spec\.kind\s*==\s*QLatin1String\(\"([a-z_0-9]+)\"\)")
_PLACEHOLDER_RE = re.compile(r"placeholder", re.IGNORECASE)
_DOC_SHOTS_RUN_RE = re.compile(r"mne_doc_shots|--target\s+doc-shots")
_REGRESSION_BIN_RE = re.compile(r"mne_screenshot_regression")


def consumers(docs_dir: Path) -> dict[str, list[str]]:
    found: dict[str, set[str]] = {}
    for page in sorted(docs_dir.rglob("*.md*")):
        for shot_id in _REFERENCE_RE.findall(page.read_text(encoding="utf-8", errors="replace")):
            found.setdefault(shot_id, set()).add(page.relative_to(REPO_ROOT).as_posix())
    return {shot_id: sorted(pages) for shot_id, pages in sorted(found.items())}


def implemented_kinds(runner: Path) -> list[str]:
    return sorted(set(_KIND_RE.findall(runner.read_text(encoding="utf-8")))) if runner.is_file() else []


def workflow_findings(workflows_dir: Path) -> dict[str, Any]:
    placeholders: list[str] = []
    doc_shots: list[str] = []
    regression: list[dict[str, Any]] = []
    for workflow in sorted(workflows_dir.glob("*.yml")):
        text = workflow.read_text(encoding="utf-8")
        name = workflow.relative_to(REPO_ROOT).as_posix()
        # Only step names and commands count; the comment explaining a placeholder is not one.
        if any(_PLACEHOLDER_RE.search(line) and not line.strip().startswith("#") for line in text.splitlines()):
            placeholders.append(name)
        if _DOC_SHOTS_RUN_RE.search(text):
            doc_shots.append(name)
        if _REGRESSION_BIN_RE.search(text):
            invocations = [block for block in re.split(r"\n\s*- name:", text) if "_BIN\"" in block or "_BIN \\" in block]
            regression.append({
                "workflow": name,
                "invocations": len(invocations),
                "compares_to_golden": any("--ref" in block for block in invocations),
            })
    return {"placeholder_workflows": placeholders, "doc_shots_workflows": doc_shots, "regression": regression}


def png_facts(path: Path) -> dict[str, Any]:
    """Decode enough of a PNG to reject placeholders and blank frames."""
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        return {"valid": False, "problem": "not a PNG"}
    width, height, depth, colour = struct.unpack(">IIBB", data[16:26])
    facts: dict[str, Any] = {"valid": True, "size": [width, height], "bytes": len(data)}
    idat = b""
    offset = 8
    while offset < len(data):
        length, chunk = struct.unpack(">I4s", data[offset:offset + 8])
        if chunk == b"IDAT":
            idat += data[offset + 8:offset + 8 + length]
        offset += 12 + length
    try:
        raw = zlib.decompress(idat)
    except zlib.error:
        return {**facts, "valid": False, "problem": "corrupt image data"}
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}.get(colour, 3)
    stride = width * channels * max(depth // 8, 1)
    rows = [raw[i * (stride + 1) + 1:(i + 1) * (stride + 1)] for i in range(0, height, max(height // 16, 1))]
    # Sampling filtered scanlines is enough to tell a uniform frame from a real one.
    facts["uniform"] = len({row for row in rows}) <= 1 and len(set(rows[0][channels:])) <= 1 if rows else True
    return facts


def validate_images(images_dir: Path, manifest: dict[str, Any]) -> list[dict[str, Any]]:
    declared = {shot["id"]: shot.get("size") for shot in manifest.get("shots", [])}
    results: list[dict[str, Any]] = []
    for path in sorted(images_dir.rglob("*.png")):
        shot_id = path.relative_to(images_dir).with_suffix("").as_posix()
        facts = png_facts(path)
        problems = [facts["problem"]] if not facts["valid"] else []
        if facts.get("size") == [1, 1]:
            problems.append("1x1 placeholder")
        elif facts["valid"]:
            if facts.get("uniform"):
                problems.append("uniform colour")
            if declared.get(shot_id) and facts["size"] != declared[shot_id]:
                problems.append(f"size {facts['size']} differs from declared {declared[shot_id]}")
        if shot_id not in declared:
            problems.append("no manifest producer")
        results.append({"id": shot_id, **facts, "problems": problems})
    return results


def build_report(images_dir: Path | None = None) -> dict[str, Any]:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    kinds = implemented_kinds(SHOT_RUNNER)
    pages = consumers(DOCS_DIR)
    shots = {shot["id"]: shot for shot in manifest.get("shots", [])}

    records = []
    for shot_id in sorted(set(shots) | set(pages)):
        shot = shots.get(shot_id)
        producer = "none"
        if shot:
            producer = "implemented" if shot.get("kind") in kinds else "unknown-kind"
        records.append({
            "id": shot_id,
            "kind": shot.get("kind") if shot else None,
            "size": shot.get("size") if shot else None,
            "producer": producer,
            "consumers": pages.get(shot_id, []),
        })

    workflows = workflow_findings(WORKFLOWS_DIR)
    report: dict[str, Any] = {
        "schema_version": 1,
        "generated_by": "tools/quality/audit_visual_assets.py",
        "summary": {
            "manifest_shots": len(shots),
            "referenced_images": len(pages),
            "referenced_without_producer": sum(1 for r in records if r["consumers"] and r["producer"] == "none"),
            "produced_without_consumer": sum(1 for r in records if r["producer"] != "none" and not r["consumers"]),
            "implemented_kinds": kinds,
            "placeholder_workflows": workflows["placeholder_workflows"],
            "doc_shots_run_in_ci": bool(workflows["doc_shots_workflows"]),
            "regression_jobs": workflows["regression"],
            "golden_references": sum(1 for job in workflows["regression"] if job["compares_to_golden"]),
        },
        "images": records,
    }
    if images_dir is not None:
        report["validated_images"] = validate_images(images_dir, manifest)
    return report


def render_markdown(report: dict[str, Any]) -> str:
    s = report["summary"]
    lines = [
        "# MNE-CPP v2.4.0 visual baseline",
        "",
        "Generated by `tools/quality/audit_visual_assets.py`. Do not edit by hand.",
        "",
        f"- Manifest shots: {s['manifest_shots']} (kinds implemented by `mne_doc_shots`: "
        f"{', '.join(f'`{k}`' for k in s['implemented_kinds'])})",
        f"- Images referenced by documentation: {s['referenced_images']}",
        f"- Referenced but no producer: **{s['referenced_without_producer']}**",
        f"- Produced but never referenced: {s['produced_without_consumer']}",
        f"- CI builds and runs `mne_doc_shots`: {'yes' if s['doc_shots_run_in_ci'] else '**no**'}",
        f"- Workflows fabricating placeholder PNGs: {', '.join(f'`{w}`' for w in s['placeholder_workflows']) or 'none'}",
        f"- Screenshot regression jobs comparing to a golden: {s['golden_references']} of {len(s['regression_jobs'])}",
        "",
        "| Image | Producer | Kind | Size | Consumers |",
        "|---|---|---|---|---|",
    ]
    for record in report["images"]:
        size = "x".join(map(str, record["size"])) if record["size"] else "-"
        pages = ", ".join(f"`{Path(p).name}`" for p in record["consumers"]) or "-"
        lines.append(f"| `{record['id']}` | {record['producer']} | {record['kind'] or '-'} | {size} | {pages} |")
    if "validated_images" in report:
        lines += ["", "## Image validation", "", "| Image | Size | Problems |", "|---|---|---|"]
        for image in report["validated_images"]:
            size = "x".join(map(str, image.get("size", []))) or "-"
            lines.append(f"| `{image['id']}` | {size} | {'; '.join(image['problems']) or 'ok'} |")
    return "\n".join(lines) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--json", type=Path, dest="json_path", help="write the JSON report")
    parser.add_argument("--markdown", type=Path, help="write the Markdown summary")
    parser.add_argument("--images", type=Path, help="validate PNGs under this directory (not part of the committed baseline)")
    args = parser.parse_args(argv)

    report = build_report(args.images)
    if args.json_path:
        args.json_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if args.markdown:
        args.markdown.write_text(render_markdown(report), encoding="utf-8")
    if not args.json_path and not args.markdown:
        print(render_markdown(report), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
