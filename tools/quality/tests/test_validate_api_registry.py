#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     September, 2026
#
#
# @brief    Fixtures for the example-evidence checks in tools/validate_api_registry.py (T3.2).
"""Fixtures for ``validate_examples`` in ``tools/validate_api_registry.py``."""

from __future__ import annotations

import copy
import importlib.util
import json
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parents[3]
_spec = importlib.util.spec_from_file_location("validate_api_registry", _REPO_ROOT / "tools" / "validate_api_registry.py")
assert _spec is not None and _spec.loader is not None
validator = importlib.util.module_from_spec(_spec)
sys.modules["validate_api_registry"] = validator
_spec.loader.exec_module(validator)

EXAMPLE = """int main()
{
    int unrelated = 0;
    //! [widget_use]
    int used = 1;
    //! [widget_use]
    return unrelated + used;
}
"""
HEADER = "/**\n * @brief Widget.\n *\n * @snippet ex_widget/main.cpp widget_use\n */\nclass Widget {};\n"
GOOD = {
    "name": "Widget", "module": "demo", "header": "demo/widget.h",
    "example": "ex_widget", "example_snippet": ["ex_widget/main.cpp#widget_use"],
    "example_mode": "run", "required_data": ["mne-cpp-test-data"], "screenshots": ["demo/overview"],
}


class ExampleEvidenceTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self.root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.root)
        (self.root / "src" / "libraries" / "demo").mkdir(parents=True)
        (self.root / "src" / "libraries" / "demo" / "widget.h").write_text(HEADER, encoding="utf-8")
        example = self.root / "src" / "examples" / "ex_widget"
        example.mkdir(parents=True)
        (example / "main.cpp").write_text(EXAMPLE, encoding="utf-8")
        (example / "CMakeLists.txt").write_text("project(ex_widget LANGUAGES CXX)\n", encoding="utf-8")
        (self.root / "src" / "examples" / "CMakeLists.txt").write_text("add_subdirectory(ex_widget)\n", encoding="utf-8")
        self.policy: dict = {"example_debt": {"ceiling": 0, "names": []}}
        self.manifest = {"shots": [{"id": "demo/overview"}]}

    def check(self, *records: dict) -> list[str]:
        registry = {"classes": [copy.deepcopy(record) for record in records]}
        return validator.validate_examples(self.root, registry, self.policy, self.manifest)[1]

    def with_(self, **changes) -> dict:
        record = copy.deepcopy(GOOD)
        for key, value in changes.items():
            if value is None:
                record.pop(key, None)
            else:
                record[key] = value
        return record

    def assert_issue(self, fragment: str, issues: list[str]) -> None:
        self.assertTrue(any(fragment in issue for issue in issues), f"{fragment!r} not in {issues}")


class TestSnippets(ExampleEvidenceTestCase):
    def test_complete_record_passes(self) -> None:
        self.assertEqual([], self.check(GOOD))

    def test_missing_region_fails(self) -> None:
        """AC-T3.2-1: a snippet naming a region the example does not contain."""
        self.assert_issue("no balanced region", self.check(self.with_(example_snippet=["ex_widget/main.cpp#nothing"])))

    def test_unbalanced_marker_fails(self) -> None:
        """AC-T3.2-1: a marker must appear exactly twice."""
        source = self.root / "src" / "examples" / "ex_widget" / "main.cpp"
        source.write_text(EXAMPLE.replace("    //! [widget_use]\n", "", 1), encoding="utf-8")
        issues = self.check(GOOD)
        self.assert_issue("marker [widget_use] appears 1 times", issues)
        self.assert_issue("no balanced region", issues)

    def test_wrong_snippet_path_fails(self) -> None:
        """AC-T3.2-1: the path must belong to the record's own example."""
        for ref in ("main.cpp#widget_use", "ex_other/main.cpp#widget_use", "ex_widget/main.cpp"):
            with self.subTest(ref=ref):
                self.assert_issue("must be 'ex_widget/<file>#<region>'", self.check(self.with_(example_snippet=[ref])))

    def test_snippet_must_be_referenced_by_the_header(self) -> None:
        (self.root / "src" / "libraries" / "demo" / "widget.h").write_text("class Widget {};\n", encoding="utf-8")
        self.assert_issue("is not referenced by @snippet", self.check(GOOD))

    def test_header_snippet_must_be_registered(self) -> None:
        self.assert_issue("is not listed in any registry", self.check(self.with_(example_snippet=[])))


class TestEligibility(ExampleEvidenceTestCase):
    def test_eligible_without_example_fails(self) -> None:
        """AC-T3.2-2: an eligible class needs a built example with a snippet."""
        bare = {"name": "Widget", "module": "demo", "header": "demo/widget.h", "example": None}
        issues = self.check(bare)
        self.assert_issue("example-eligible but has no built example", issues)
        self.assert_issue("example-eligible but has no built example", self.check(self.with_(example_snippet=[])))

    def test_debt_admits_listed_classes_and_only_shrinks(self) -> None:
        bare = {"name": "Widget", "module": "demo", "header": "demo/widget.h"}
        (self.root / "src" / "libraries" / "demo" / "widget.h").write_text("class Widget {};\n", encoding="utf-8")
        self.policy = {"example_debt": {"ceiling": 1, "names": ["Widget"]}}
        self.assertEqual([], self.check(bare))
        self.policy = {"example_debt": {"ceiling": 0, "names": ["Widget"]}}
        self.assert_issue("above its ceiling", self.check(bare))
        self.policy = {"example_debt": {"ceiling": 2, "names": ["Widget", "Gone"]}}
        self.assert_issue("'Gone' is not a registry class", self.check(bare))

    def test_backed_class_must_leave_the_debt(self) -> None:
        self.policy = {"example_debt": {"ceiling": 1, "names": ["Widget"]}}
        self.assert_issue("remove it from example_debt", self.check(GOOD))

    def test_policy_exemption_counts_as_exempt(self) -> None:
        self.policy = {"exempt_name_suffixes": {"Widget": "demo"}, "example_debt": {"ceiling": 0, "names": []}}
        bare = {"name": "Widget", "module": "demo", "header": "demo/widget.h"}
        (self.root / "src" / "libraries" / "demo" / "widget.h").write_text("class Widget {};\n", encoding="utf-8")
        self.assertEqual([], self.check(bare))


class TestExemptions(ExampleEvidenceTestCase):
    def setUp(self) -> None:
        super().setUp()
        (self.root / "src" / "libraries" / "demo" / "widget.h").write_text("class Widget {};\n", encoding="utf-8")

    def test_exemption_needs_a_reason(self) -> None:
        """AC-T3.2-3: example_exempt without a non-empty reason fails."""
        for reason in (None, "", "  "):
            record = {"name": "Widget", "module": "demo", "header": "demo/widget.h", "example_exempt": True}
            if reason is not None:
                record["example_exempt_reason"] = reason
            with self.subTest(reason=reason):
                self.assert_issue("requires a non-empty 'example_exempt_reason'", self.check(record))

    def test_reasoned_exemption_passes(self) -> None:
        record = {"name": "Widget", "module": "demo", "header": "demo/widget.h",
                  "example_exempt": True, "example_exempt_reason": "Needs a running server."}
        self.assertEqual([], self.check(record))

    def test_inconsistent_exemption_fields_fail(self) -> None:
        self.assert_issue("without 'example_exempt: true'", self.check(
            {"name": "Widget", "module": "demo", "header": "demo/widget.h", "example_exempt_reason": "x"}))
        self.assert_issue("must be boolean", self.check(
            {"name": "Widget", "module": "demo", "header": "demo/widget.h", "example_exempt": "yes",
             "example_exempt_reason": "x"}))


class TestTargetsAndMetadata(ExampleEvidenceTestCase):
    def test_example_must_be_a_cmake_target(self) -> None:
        """AC-T3.2-4: the referenced example must be added by src/examples/CMakeLists.txt."""
        (self.root / "src" / "examples" / "CMakeLists.txt").write_text("", encoding="utf-8")
        self.assert_issue("is not a CMake target", self.check(GOOD))

    def test_unknown_mode_data_and_screenshot_fail(self) -> None:
        self.assert_issue("'example_mode' must be one of", self.check(self.with_(example_mode="maybe")))
        self.assert_issue("'required_data' must list datasets", self.check(self.with_(required_data=["ftp"])))
        self.assert_issue("screenshot 'nope' is not in", self.check(self.with_(screenshots=["nope"])))

    def test_example_fields_require_an_example(self) -> None:
        record = {"name": "Widget", "module": "demo", "header": "demo/widget.h", "example_mode": "run",
                  "example_exempt": True, "example_exempt_reason": "x"}
        self.assert_issue("'example_mode' requires 'example'", self.check(record))


class ParityTestFieldTestCase(unittest.TestCase):
    def issues(self, **record) -> list[str]:
        record = {"python": "mne.f", "status": "implemented", "mne_cpp": "F", **record}
        return validator.validate_parity_block({"parity": {"mne_python_pinned": "1.11", "records": [record]}},
                                               _REPO_ROOT)[1]

    def test_existing_test_passes(self) -> None:
        self.assertEqual(self.issues(test="test_fiff_core_python"), [])

    def test_unknown_test_or_missing_record_fails(self) -> None:
        self.assertIn("'test' must name an existing", self.issues(test="test_nope")[0])
        self.assertIn("'test' must name an existing",
                      self.issues(status="missing", mne_cpp="", test="test_fiff_core_python")[0])


class TestRepositoryRegistry(unittest.TestCase):
    def test_committed_registry_passes(self) -> None:
        registry = json.loads((_REPO_ROOT / "doc" / "api_registry.json").read_text(encoding="utf-8"))
        policy = json.loads((_REPO_ROOT / validator.EVIDENCE_POLICY).read_text(encoding="utf-8"))
        manifest = json.loads((_REPO_ROOT / validator.SCREENSHOT_MANIFEST).read_text(encoding="utf-8"))
        ok, issues, stats = validator.validate_examples(_REPO_ROOT, registry, policy, manifest)
        self.assertEqual([], issues)
        self.assertGreaterEqual(stats["backed"], 1)
        self.assertEqual(policy["example_debt"]["ceiling"], len(policy["example_debt"]["names"]))


if __name__ == "__main__":
    unittest.main()
