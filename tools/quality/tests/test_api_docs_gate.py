#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     October, 2026
#
#
# @brief    Fixtures for the registry/XML audit and the generated-pages check (T4.2).
"""Fixtures for ``tools/doxy2mdx/audit_registry.py`` and ``tools/doxy2mdx/check_generated.py``."""

from __future__ import annotations

import importlib.util
import json
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[3]
_DOXY2MDX = _REPO_ROOT / "tools" / "doxy2mdx"


def _load(name: str):
    spec = importlib.util.spec_from_file_location(name, _DOXY2MDX / f"{name}.py")
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


audit = _load("audit_registry")
generated = _load("check_generated")

REGISTRY = {
    "modules": {"fiff": {"namespace": "FIFFLIB", "dir_slug": "fiff"}},
    "classes": [{"name": "FiffInfo", "module": "fiff", "header": "fiff/fiff_info.h", "documented": True}],
}
EXCLUSIONS = {
    "namespaces": {"std": "Standard library."},
    "unnamespaced": {"reason": "Renderer internals.", "header_prefixes": ["disp3D/"]},
    "classes": {"FIFFLIB::FiffTool": "Command-line helper."},
}


class AuditTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self.xml = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.xml)
        self.compounds: dict[str, tuple[str, str]] = {}
        self.add("FIFFLIB::FiffInfo", "fiff/fiff_info.h")
        self.add("FIFFLIB::FiffTool", "fiff/fiff_tool.h")
        self.add("std::vector", "vector")
        self.add("BrainView", "disp3D/view/brainview.h")

    def add(self, name: str, header: str, kind: str = "class") -> None:
        self.compounds[name] = (header, kind)

    def problems(self, registry: dict | None = None, exclusions: dict | None = None) -> list[str]:
        index = ["<doxygenindex>",
                 '<compound kind="namespace" refid="ns_f"><name>FIFFLIB</name></compound>',
                 '<compound kind="namespace" refid="ns_s"><name>std</name></compound>']
        for i, (name, (header, kind)) in enumerate(self.compounds.items()):
            index.append(f'<compound kind="{kind}" refid="c{i}"><name>{name}</name></compound>')
            (self.xml / f"c{i}.xml").write_text(
                f'<doxygen><compounddef id="c{i}"><compoundname>{name}</compoundname>'
                f'<location file="{header}"/></compounddef></doxygen>', encoding="utf-8")
        (self.xml / "index.xml").write_text("".join(index) + "</doxygenindex>", encoding="utf-8")
        return audit.audit(registry or REGISTRY, audit.xml_classes(self.xml), audit.xml_namespaces(self.xml),
                           exclusions if exclusions is not None else EXCLUSIONS)


class TestRegistryAudit(AuditTestCase):
    def test_consistent_inputs_pass(self) -> None:
        self.assertEqual([], self.problems())

    def test_unregistered_xml_class_fails(self) -> None:
        """AC-T4.2-1: a public class in the XML that is neither registered nor excluded."""
        self.add("FIFFLIB::FiffNew", "fiff/fiff_new.h")
        self.assertEqual(["class 'FIFFLIB::FiffNew' (fiff/fiff_new.h) is in the Doxygen XML but not in the registry"],
                         self.problems())

    def test_documented_class_missing_from_xml_fails(self) -> None:
        """AC-T4.2-1: a documented registry class that Doxygen did not extract."""
        del self.compounds["FIFFLIB::FiffInfo"]
        self.assertIn("registry class 'FiffInfo' is documented but not in the Doxygen XML", self.problems())

    def test_module_namespace_must_exist(self) -> None:
        registry = json.loads(json.dumps(REGISTRY))
        registry["modules"]["fiff"]["namespace"] = "FIFFLIBRARY"
        self.assertIn("module 'fiff': namespace 'FIFFLIBRARY' does not occur in the Doxygen XML",
                      self.problems(registry))

    def test_nested_classes_are_not_public_units(self) -> None:
        self.add("FIFFLIB::FiffInfo::Entry", "fiff/fiff_info.h")
        self.add("FIFFLIB::FiffInfo::Item", "fiff/fiff_info.h", "struct")
        self.assertEqual([], self.problems())

    def test_unregistered_struct_fails(self) -> None:
        """Exported structs (parameter and result records) are public API like classes."""
        self.add("FIFFLIB::FiffParams", "fiff/fiff_params.h", "struct")
        self.assertEqual(["class 'FIFFLIB::FiffParams' (fiff/fiff_params.h) is in the Doxygen XML but not in the registry"],
                         self.problems())

    def test_documented_struct_passes(self) -> None:
        self.add("FIFFLIB::FiffParams", "fiff/fiff_params.h", "struct")
        registry = json.loads(json.dumps(REGISTRY))
        registry["classes"].append({"name": "FiffParams", "module": "fiff", "header": "fiff/fiff_params.h",
                                    "documented": True})
        self.assertEqual([], self.problems(registry))


class TestExclusions(AuditTestCase):
    def test_each_exclusion_kind_applies(self) -> None:
        """AC-T4.2-3: namespace, class and unnamespaced-header exclusions each hide one class."""
        for key, fragment in (("namespaces", "std::vector"), ("classes", "FIFFLIB::FiffTool"),
                              ("unnamespaced", "BrainView")):
            with self.subTest(kind=key):
                reduced = json.loads(json.dumps(EXCLUSIONS))
                reduced[key] = {}
                self.assertTrue(any(fragment in problem for problem in self.problems(exclusions=reduced)))

    def test_unused_exclusion_fails(self) -> None:
        self.assertIn("exclusion: class 'FIFFLIB::FiffGone' matches nothing", self.problems(exclusions={
            **EXCLUSIONS, "classes": {**EXCLUSIONS["classes"], "FIFFLIB::FiffGone": "Removed."}}))

    def test_exclusion_needs_a_reason(self) -> None:
        path = self.xml / "exclusions.json"
        for broken in ({"classes": {"FIFFLIB::FiffTool": " "}},
                       {"unnamespaced": {"header_prefixes": ["disp3D/"]}}):
            with self.subTest(broken=broken):
                path.write_text(json.dumps(broken), encoding="utf-8")
                with self.assertRaises(ValueError):
                    audit.load_exclusions(path)

    def test_committed_exclusions_have_reasons(self) -> None:
        audit.load_exclusions(audit.EXCLUSIONS)


class TestGeneratedDiff(unittest.TestCase):
    def setUp(self) -> None:
        self.root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.root)
        self.gen, self.com = self.root / "gen", self.root / "com"
        for base in (self.gen, self.com):
            (base / "fiff").mkdir(parents=True)
            (base / "fiff" / "fiff-info.mdx").write_text("page\n", encoding="utf-8")
        (self.com / "index.mdx").write_text("hand-written\n", encoding="utf-8")

    def test_identical_output_passes(self) -> None:
        self.assertEqual({"stale": [], "missing": [], "orphaned": []}, generated.compare(self.gen, self.com))

    def test_stale_missing_and_orphaned_pages_fail(self) -> None:
        """AC-T4.2-2: a changed page, an uncommitted page and a page nobody generates are reported."""
        (self.com / "fiff" / "fiff-info.mdx").write_text("edited\n", encoding="utf-8")
        (self.gen / "fiff" / "fiff-new.mdx").write_text("new\n", encoding="utf-8")
        (self.com / "fiff" / "fiff-gone.mdx").write_text("old\n", encoding="utf-8")
        self.assertEqual({"stale": ["fiff/fiff-info.mdx"], "missing": ["fiff/fiff-new.mdx"],
                          "orphaned": ["fiff/fiff-gone.mdx"]}, generated.compare(self.gen, self.com))


class TestEntryPoint(unittest.TestCase):
    def test_check_mode_audits_and_diffs_without_rewriting(self) -> None:
        script = (_REPO_ROOT / "doc" / "build-api-docs.sh").read_text(encoding="utf-8")
        commands = [line.strip() for line in script.splitlines() if line.strip() and not line.lstrip().startswith("#")]
        check = next(i for i, line in enumerate(commands) if "check_generated.py" in line)
        audit_step = next(i for i, line in enumerate(commands) if "audit_registry.py --check" in line)
        rewrite = next(i for i, line in enumerate(commands) if line.startswith("python3 tools/doxy2mdx/doxy2mdx.py"))
        self.assertLess(audit_step, check)
        self.assertEqual("exit 0", commands[check + 1])
        self.assertLess(check, rewrite)
        gate = (_REPO_ROOT / "doc" / "check-docs.sh").read_text(encoding="utf-8")
        self.assertIn("doc/build-api-docs.sh --check", gate)
        for workflow in ("pull-request.yml", "staging.yml", "main.yml"):
            text = (_REPO_ROOT / ".github" / "workflows" / workflow).read_text(encoding="utf-8")
            self.assertIn("uses: ./.github/workflows/_reusable-docs.yml", text, workflow)


class TestCommittedPages(unittest.TestCase):
    def test_every_api_link_resolves(self) -> None:
        """A link to a module whose classes are all excluded must not survive in the index pages."""
        import re

        api = _REPO_ROOT / "doc" / "website" / "docs" / "api"
        unresolved = []
        for page in api.rglob("*.mdx"):
            for link in re.findall(r"\]\((/docs/api/[^)#\s]+)", page.read_text(encoding="utf-8")):
                target = api / link[len("/docs/api/"):].rstrip("/")
                if not (target.with_suffix(".mdx").exists() or (target / "index.mdx").exists()):
                    unresolved.append(f"{page.relative_to(api)} -> {link}")
        self.assertEqual([], unresolved)


if __name__ == "__main__":
    unittest.main()
