#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     October, 2026
#
#
# @brief    Fixtures for website strictness, the quality page and version routing (T4.3).
"""Fixtures for the Docusaurus configuration, ``render_quality_pages.py`` and ``check_website_build.py``."""

from __future__ import annotations

import importlib.util
import json
import re
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

_QUALITY_DIR = Path(__file__).resolve().parent.parent
_REPO_ROOT = _QUALITY_DIR.parents[1]
_WEBSITE = _REPO_ROOT / "doc" / "website"
_WORKFLOWS = _REPO_ROOT / ".github" / "workflows"


def _load(name: str):
    spec = importlib.util.spec_from_file_location(name, _QUALITY_DIR / f"{name}.py")
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


pages = _load("render_quality_pages")
build_check = _load("check_website_build")


def _table(page: str, header: str) -> dict[str, list[str]]:
    rows = {}
    lines = page.splitlines()
    start = next(i for i, line in enumerate(lines) if line.startswith(header))
    for line in lines[start + 2:]:
        if not line.startswith("|"):
            break
        cells = [cell.strip() for cell in line.strip("|").split("|")]
        rows[cells[0].strip("`")] = cells[1:]
    return rows


class TestStrictConfig(unittest.TestCase):
    def setUp(self) -> None:
        self.config = (_WEBSITE / "docusaurus.config.ts").read_text(encoding="utf-8")

    def test_broken_links_and_anchors_fail(self) -> None:
        """AC-T4.3-1: broken internal links, anchors and Markdown links stop the build."""
        for option in ("onBrokenLinks", "onBrokenAnchors", "onBrokenMarkdownLinks"):
            with self.subTest(option=option):
                self.assertRegex(self.config, rf"{option}:\s*'throw'")
        self.assertNotRegex(self.config, r"onBroken\w+:\s*'(warn|ignore|log)'")

    def test_only_generated_screenshots_may_be_missing(self) -> None:
        """AC-T4.3-1: a missing image fails unless it is a generated manual screenshot."""
        hook = self.config[self.config.index("onBrokenMarkdownImages"):]
        hook = hook[:hook.index("},\n        },")]
        self.assertIn("url.startsWith('/img/manual/auto/')", hook)
        self.assertIn("throw new Error", hook)

    def test_no_workflow_fabricates_placeholder_images(self) -> None:
        for workflow in ("pull-request.yml", "staging.yml", "main.yml"):
            with self.subTest(workflow=workflow):
                text = (_WORKFLOWS / workflow).read_text(encoding="utf-8")
                self.assertNotIn("IEND", text)
                self.assertNotRegex(text, r"(?i)name:.*placeholder")


class TestQualityPage(unittest.TestCase):
    def test_numbers_match_the_source_json(self) -> None:
        """AC-T4.3-2: every gate and library figure on the page equals the quality JSON."""
        page = pages.build()
        release = _REPO_ROOT / "doc" / "release" / "v2.4.0"
        dashboard = json.loads((release / "quality-baseline.json").read_text(encoding="utf-8"))
        coverage = json.loads((release / "coverage-baseline.json").read_text(encoding="utf-8"))
        api = json.loads((release / "api-evidence-baseline.json").read_text(encoding="utf-8"))
        parity = json.loads((release / "parity-baseline.json").read_text(encoding="utf-8"))

        gates = _table(page, "| Gate |")
        self.assertEqual(f"{dashboard['gates']['G2_coverage']['line_combined']:.1f}%",
                         gates["Combined line coverage"][1])
        self.assertEqual(f"{dashboard['gates']['G2_coverage']['branch_libraries']:.1f}%",
                         gates["Library branch coverage"][1])

        libraries = _table(page, "| Library |")
        expected = {key.split("/")[2] for key in coverage["components"] if key.startswith("src/libraries/")}
        self.assertEqual(expected, set(libraries))
        for lib, cells in libraries.items():
            with self.subTest(library=lib):
                totals = coverage["components"][f"src/libraries/{lib}"]
                self.assertEqual(f"{totals['line_percent']:.1f}%", cells[0])
                evidence = api["by_library"].get(lib)
                if evidence and evidence["eligible"]:
                    self.assertTrue(cells[3].startswith(
                        f"{evidence['eligible_with_registry_example']} / {evidence['eligible']} "))

        claims = _table(page, "| Evidence |")
        self.assertEqual({k: str(v) for k, v in parity["summary"]["by_evidence"].items()},
                         {k: v[0] for k, v in claims.items()})

    def test_committed_page_is_current(self) -> None:
        self.assertEqual(0, pages.main(["--check"]))

    def test_page_is_in_the_sidebar(self) -> None:
        self.assertIn("'development/quality'", (_WEBSITE / "sidebars.ts").read_text(encoding="utf-8"))


class TestVersionRouting(unittest.TestCase):
    VERSION = "9.8.7"

    def build(self, base: str, label: str) -> Path:
        root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, root)
        (root / "index.html").write_text(
            f'<link href="{base}assets/css/styles.css"><script src="{base}assets/js/main.js"></script>'
            f'<a class="navbar__link">{label}</a><a>v{self.VERSION} (Stable)</a>', encoding="utf-8")
        return root

    def test_each_channel_needs_its_base_url_and_label(self) -> None:
        """AC-T4.3-3: stable is served from / as vX.Y.Z, dev from /dev/ as 'dev (latest)'."""
        self.assertEqual([], build_check.check(self.build("/", f"v{self.VERSION}"), "stable", self.VERSION))
        self.assertEqual([], build_check.check(self.build("/dev/", "dev (latest)"), "dev", self.VERSION))
        wrong = build_check.check(self.build("/", f"v{self.VERSION}"), "dev", self.VERSION)
        self.assertTrue(any("expected base URL '/dev/'" in problem for problem in wrong))
        self.assertTrue(any("labelled as release" in problem for problem in wrong))
        self.assertTrue(build_check.check(self.build("/dev/", "dev (latest)"), "stable", self.VERSION))

    def test_site_version_matches_the_project(self) -> None:
        config = (_WEBSITE / "docusaurus.config.ts").read_text(encoding="utf-8")
        declared = re.search(r"^const stableVersion = '([^']+)'", config, re.MULTILINE).group(1)
        # Docusaurus validates every named export of the config module as a config field.
        self.assertNotRegex(config, r"(?m)^export (?!default)")
        self.assertEqual(build_check.project_version(), declared)
        self.assertNotRegex(config.replace(f"'{declared}'", ""), r"\b\d+\.\d+\.\d+\b(?!-)")

    def test_workflows_check_the_channel_they_deploy(self) -> None:
        for workflow, channel in (("staging.yml", "dev"), ("main.yml", "stable"), ("pull-request.yml", "stable")):
            with self.subTest(workflow=workflow):
                text = (_WORKFLOWS / workflow).read_text(encoding="utf-8")
                self.assertIn(f"check_website_build.py doc/website/build --channel {channel}", text)
                self.assertIn("render_quality_pages.py --check", text)


if __name__ == "__main__":
    unittest.main()
