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

    def test_relative_doc_links_point_to_files(self) -> None:
        """With trailingSlash, a link like ](workflow) resolves below the current page."""
        link = re.compile(r"\]\(([^)\s#]+)(?:#[^)\s]*)?\)")
        docs = _WEBSITE / "docs"
        for page in sorted(docs.rglob("*.md*")):
            if "api" in page.relative_to(docs).parts:
                continue
            for target in link.findall(page.read_text(encoding="utf-8")):
                if re.match(r"([a-z]+:|/)", target) or Path(target.rstrip("/")).suffix:
                    continue
                with self.subTest(page=str(page.relative_to(docs)), target=target):
                    self.fail("link to the .md/.mdx file instead of the URL slug")

    def test_missing_images_fail(self) -> None:
        """AC-T4.3-1, AC-T5.3-3: a missing image fails; only a local opt-in skips generated screenshots."""
        hook = self.config[self.config.index("onBrokenMarkdownImages"):]
        hook = hook[:hook.index("},\n        },")]
        self.assertIn("process.env.MNECPP_ALLOW_MISSING_SCREENSHOTS === '1'", hook)
        self.assertIn("throw new Error", hook)
        for workflow in _WORKFLOWS.glob("*.yml"):
            with self.subTest(workflow=workflow.name):
                self.assertNotIn("MNECPP_ALLOW_MISSING_SCREENSHOTS", workflow.read_text(encoding="utf-8"))

    def test_site_builds_wait_for_generated_screenshots(self) -> None:
        """AC-T5.3-2, AC-T4.4-1: the docs gate builds the site from the screenshots of the DocShots job."""
        docs = (_WORKFLOWS / "_reusable-docs.yml").read_text(encoding="utf-8")
        self.assertIn("uses: ./.github/workflows/_reusable-doc-shots.yml", docs)
        body = docs[docs.index("\n  Docs:\n"):]
        self.assertRegex(body[:200], r"needs: DocShots")
        self.assertLess(body.index("name: doc-screenshots"), body.index("doc/check-docs.sh"))
        shots = (_WORKFLOWS / "_reusable-doc-shots.yml").read_text(encoding="utf-8")
        self.assertLess(shots.index("--target doc-shots"), shots.index("validate_screenshots.py"))
        script = (_REPO_ROOT / "doc" / "check-docs.sh").read_text(encoding="utf-8")
        script = script[script.index("set -euo pipefail"):]
        order = ["build-api-docs.sh --check", "render_quality_pages.py --check", "validate_screenshots.py",
                 "npm run build", "check_website_build.py"]
        self.assertEqual(order, sorted(order, key=script.index))

    @unittest.skipUnless(shutil.which("doxygen") and shutil.which("bash"), "needs Doxygen and bash")
    def test_gate_script_fails_on_a_broken_screenshot(self) -> None:
        """AC-T4.4-2: a missing or blank screenshot injected into the generated set fails doc/check-docs.sh."""
        import subprocess

        manifest = json.loads((_WEBSITE / "screenshots" / "manifest.json").read_text(encoding="utf-8"))
        with tempfile.TemporaryDirectory() as temp:
            shots = Path(temp)
            for shot in manifest["shots"]:
                (shots / f"{shot['id']}.png").parent.mkdir(parents=True, exist_ok=True)
            # Every image is missing except one 1x1 placeholder.
            (shots / f"{manifest['shots'][0]['id']}.png").write_bytes(b"\x89PNG\r\n\x1a\n")
            run = subprocess.run(["bash", str(_REPO_ROOT / "doc" / "check-docs.sh"), "--channel", "dev",
                                  "--screenshots", str(shots), "--skip-site"],
                                 capture_output=True, text=True)
        self.assertEqual(run.returncode, 1, run.stdout + run.stderr)
        self.assertIn("FAIL", run.stdout)

    def test_deployment_waits_for_the_docs_gate(self) -> None:
        """AC-T5.3-3: a failing documentation gate blocks deployment; the deployed site is the checked build."""
        for workflow, channel in (("staging.yml", "dev"), ("main.yml", "stable")):
            with self.subTest(workflow=workflow):
                text = (_WORKFLOWS / workflow).read_text(encoding="utf-8")
                body = text[text.index("\n  Website:\n"):]
                self.assertRegex(body[:300], r"needs: \[.*\bDocs\b.*\]")
                self.assertIn(f"name: website-{channel}", body)
                self.assertNotIn("npm run build", body[:body.index("\n  # ", 1) if "\n  # " in body[1:] else None])

    def test_dev_website_deploys_on_every_green_docs_gate(self) -> None:
        """The dev site follows staging: a push that only changes sources or docs still deploys it."""
        text = (_WORKFLOWS / "staging.yml").read_text(encoding="utf-8")
        job = text[text.index("\n  Website:\n"):text.index("\n  # ── WebAssembly")]
        self.assertNotIn("CreateDevRelease", job)
        self.assertRegex(job, r"concurrency:\s*\n\s+group: website-dev")

    def test_no_node20_actions(self) -> None:
        """GitHub deprecated Node.js 20 actions; these majors still declare it."""
        node20 = re.compile(r"uses: actions/(checkout@v4|cache(/\w+)?@v4|upload-artifact@v4|download-artifact@v[4-6]|"
                            r"setup-python@v5|setup-node@v4)\b")
        for workflow in sorted(_WORKFLOWS.glob("*.yml")):
            with self.subTest(workflow=workflow.name):
                self.assertIsNone(node20.search(workflow.read_text(encoding="utf-8")))

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

    def build(self, base: str, label: str, site_base: str | None = None, css: str | None = None) -> Path:
        root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, root)
        url = f"https://mne-cpp.github.io{site_base or base}"
        css = css or f"{base}katex.min.css"
        (root / "index.html").write_text(
            f'<link rel="canonical" href="{url}"><link rel="stylesheet" href="{css}">'
            f'<link href="{base}assets/css/styles.css"><script src="{base}assets/js/main.js"></script>'
            f'<a class="navbar__link">{label}</a><a>v{self.VERSION} (Stable)</a>', encoding="utf-8")
        (root / "sitemap.xml").write_text(f"<urlset><url><loc>{url}download/</loc></url></urlset>", encoding="utf-8")
        return root

    def test_each_channel_needs_its_base_url_and_label(self) -> None:
        """AC-T4.3-3: stable is served from / as vX.Y.Z, dev from /dev/ as 'dev (latest)'."""
        self.assertEqual([], build_check.check(self.build("/", f"v{self.VERSION}"), "stable", self.VERSION))
        self.assertEqual([], build_check.check(self.build("/dev/", "dev (latest)"), "dev", self.VERSION))
        wrong = build_check.check(self.build("/", f"v{self.VERSION}"), "dev", self.VERSION)
        self.assertTrue(any("expected base URL '/dev/'" in problem for problem in wrong))
        self.assertTrue(any("labelled as release" in problem for problem in wrong))
        self.assertTrue(build_check.check(self.build("/dev/", "dev (latest)"), "stable", self.VERSION))
        # Search engines and the sitemap must point at the channel's own pages
        stale = build_check.check(self.build("/dev/", "dev (latest)", site_base="/"), "dev", self.VERSION)
        self.assertIn("canonical URL https://mne-cpp.github.io/ is outside https://mne-cpp.github.io/dev/", stale)
        self.assertIn("sitemap lists 1 URLs outside https://mne-cpp.github.io/dev/", stale)
        redirected = self.build("/dev/", "dev (latest)")
        (redirected / "sitemap.xml").write_text("<urlset><url><loc>https://mne-cpp.github.io/dev/download</loc></url></urlset>")
        self.assertIn("sitemap lists 1 URLs without a trailing slash, which GitHub Pages redirects",
                      build_check.check(redirected, "dev", self.VERSION))
        # A config stylesheet is not prefixed with the base URL by Docusaurus
        root_css = build_check.check(self.build("/dev/", "dev (latest)", css="/katex.min.css"), "dev", self.VERSION)
        self.assertIn("local links outside base URL '/dev/': /katex.min.css", root_css)

    def test_unrendered_markdown_and_math_fail(self) -> None:
        root = self.build("/dev/", "dev (latest)")
        page = root / "docs" / "api" / "x" / "index.html"
        page.parent.mkdir(parents=True)
        page.write_text('<article><p>see <a href="/dev/docs/api/network"><code>[Network](/docs/api/network)</code></a></p>'
                        '<pre><code>a[0](/b)</code></pre><span class="katex-error" title="x">\\frac</span></article>',
                        encoding="utf-8")
        problems = build_check.check(root, "dev", self.VERSION)
        self.assertIn("docs/api/x/index.html: 1 x markdown link shown as text", problems)
        self.assertIn("docs/api/x/index.html: 1 x KaTeX error", problems)

    def test_download_page_has_no_hard_coded_release(self) -> None:
        page = (_WEBSITE / "src" / "pages" / "download.tsx").read_text(encoding="utf-8")
        self.assertNotRegex(page, r"releases/tag/v\d|tag_name: 'v\d")

    def test_download_page_links_each_release_asset_by_its_exact_name(self) -> None:
        sys.path.insert(0, str(_REPO_ROOT / "scripts" / "release"))
        import verify_release  # noqa: PLC0415

        page = (_WEBSITE / "src" / "pages" / "download.tsx").read_text(encoding="utf-8")
        rows = re.findall(r"asset\('([^']+)', '([^']+)'\)", page)
        suffixes = [suffix for _, suffix in rows]
        required = [s for platform in verify_release.REQUIRED.values() for s in platform]
        # Each release asset is offered exactly once, so an installer row cannot pick up another .dmg
        self.assertEqual(sorted(suffixes), sorted(required))
        for label, suffix in rows:
            self.assertIn("ARM64" if "arm64" in suffix else "x86_64", label, suffix)

    def test_stated_macos_minimum_is_the_build_target(self) -> None:
        sys.path.insert(0, str(_REPO_ROOT / "scripts" / "release"))
        import verify_release  # noqa: PLC0415

        cmake = (_REPO_ROOT / "src" / "CMakeLists.txt").read_text(encoding="utf-8")
        target = re.search(r'set\(CMAKE_OSX_DEPLOYMENT_TARGET "([\d.]+)"', cmake)
        self.assertIsNotNone(target, "src/CMakeLists.txt sets no macOS deployment target")
        self.assertLess(cmake.index("CMAKE_OSX_DEPLOYMENT_TARGET"), cmake.index("project(mne_cpp"))
        self.assertEqual(verify_release.MACOS_MINIMUM, target.group(1))
        page = (_WEBSITE / "src" / "pages" / "download.tsx").read_text(encoding="utf-8")
        self.assertIn(f"<td>macOS {target.group(1)} or later</td>", page)

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
                self.assertRegex(text, rf"uses: ./.github/workflows/_reusable-docs.yml\n    with:\n      channel: {channel}\n")
        docs = (_WORKFLOWS / "_reusable-docs.yml").read_text(encoding="utf-8")
        self.assertIn('doc/check-docs.sh --channel "${{ inputs.channel }}"', docs)
        script = (_REPO_ROOT / "doc" / "check-docs.sh").read_text(encoding="utf-8")
        self.assertIn('check_website_build.py doc/website/build --channel "$CHANNEL"', script)


if __name__ == "__main__":
    unittest.main()
