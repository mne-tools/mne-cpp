#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     August, 2026
#
#
# @brief    Tests for the LCOV coverage model and its policy (T0.2, T2.2).
"""Tests for ``tools/quality/summarize_coverage.py`` and the coverage configuration."""

from __future__ import annotations

import importlib.util
import io
import json
import re
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

_QUALITY_DIR = Path(__file__).resolve().parents[1]
_REPO_ROOT = _QUALITY_DIR.parents[1]
_spec = importlib.util.spec_from_file_location("summarize_coverage", _QUALITY_DIR / "summarize_coverage.py")
assert _spec is not None and _spec.loader is not None
coverage = importlib.util.module_from_spec(_spec)
sys.modules["summarize_coverage"] = coverage
_spec.loader.exec_module(coverage)


def _record(path: str, lines: dict[int, int], branches: tuple[str, ...] = ()) -> str:
    body = [f"SF:{path}", *(f"BRDA:{entry}" for entry in branches)]
    body += [f"DA:{line},{count}" for line, count in lines.items()]
    return "\n".join(body + ["end_of_record"]) + "\n"


class CoverageTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.root = Path(self._tmp.name)

    def write(self, name: str, text: str) -> Path:
        path = self.root / name
        path.write_text(text, encoding="utf-8")
        return path

    def parse(self, contents: str):
        return coverage.parse_lcov(self.write("coverage.info", contents))

    def policy(self, exclusions: list) -> Path:
        return self.write("policy.json", json.dumps({"exclusions": exclusions}))


class TestLcovParsing(CoverageTestCase):
    def test_line_and_branch_totals_reconcile(self) -> None:
        files = self.parse(
            _record("/checkout/src/libraries/fiff/stream.cpp", {1: 3, 2: 0, 3: 1}, ("2,0,0,1", "2,0,1,0", "3,0,0,-"))
            + _record("/checkout/src/tools/info/mne_show/main.cpp", {1: 0, 2: 5})
        )
        summary = coverage.build_summary(files, commit="abc", source_url=None, top=25)
        totals = summary["totals"]
        self.assertEqual((5, 3, 60.0), (totals["lines_found"], totals["lines_hit"], totals["line_percent"]))
        self.assertEqual((3, 1, 33.33), (totals["branches_found"], totals["branches_hit"], totals["branch_percent"]))
        self.assertEqual({"libraries", "tools"}, {name for name, values in summary["scopes"].items() if values["files"]})

    def test_paths_are_repository_relative(self) -> None:
        files = self.parse(
            _record("C:\\work\\src\\applications\\mne_scan\\main.cpp", {1: 0})
            + _record("/home/runner/work/mne-cpp/mne-cpp/build/src/tools/info/x_autogen/moc_a.cpp", {1: 1})
        )
        self.assertEqual(["src/applications/mne_scan/main.cpp", "src/tools/info/x_autogen/moc_a.cpp"],
                         [item.path for item in files])
        self.assertEqual("src/applications/mne_scan", files[0].component)

    def test_duplicate_records_are_summed(self) -> None:
        files = self.parse(
            _record("/a/src/libraries/mne/example.cpp", {1: 0, 2: 1}, ("1,0,0,-",))
            + _record("/b/src/libraries/mne/example.cpp", {1: 2, 2: 0}, ("1,0,0,4",))
        )
        self.assertEqual(1, len(files))
        self.assertEqual({1: 2, 2: 1}, files[0].lines)
        self.assertEqual(1, files[0].branches_hit)

    def test_unterminated_record_fails(self) -> None:
        with self.assertRaisesRegex(ValueError, "unterminated LCOV record"):
            self.parse("SF:/checkout/src/libraries/fiff/stream.cpp\nDA:1,1\n")

    def test_out_of_scope_record_fails(self) -> None:
        with self.assertRaisesRegex(ValueError, "outside the measured production scope"):
            self.parse(_record("/checkout/src/testframes/test_fiff/test.cpp", {1: 1}))

    def test_written_report_round_trips(self) -> None:
        files = self.parse(_record("/x/src/libraries/mne/a.cpp", {3: 1, 1: 0}, ("3,0,1,0", "3,0,0,2", "4,1,0,-")))
        out = self.root / "out.info"
        coverage.write_lcov(files, out)
        text = out.read_text(encoding="utf-8")
        self.assertIn("SF:src/libraries/mne/a.cpp\n", text)
        self.assertIn("BRDA:4,1,0,-\n", text)
        self.assertNotIn("FN:", text)
        again = coverage.parse_lcov(out)
        self.assertEqual((files[0].lines, files[0].branches), (again[0].lines, again[0].branches))


class TestExclusions(CoverageTestCase):
    def test_missing_reason_is_rejected(self) -> None:
        """AC-T2.2-3: an exclusion without a reason is a policy error."""
        for entry in ({"pattern": "src/tools/*"}, {"pattern": "src/tools/*", "reason": "  "},
                      {"pattern": "", "reason": "x"}, {"pattern": "a", "reason": "b", "owner": "c"}):
            with self.subTest(entry=entry):
                with self.assertRaises(coverage.PolicyError):
                    coverage.load_exclusions(self.policy([entry]))

    def test_main_rejects_policy_without_reason(self) -> None:
        lcov = self.write("c.info", _record("/x/src/libraries/mne/a.cpp", {1: 1}))
        err = io.StringIO()
        with redirect_stdout(io.StringIO()), redirect_stderr(err):
            code = coverage.main([str(lcov), "--policy", str(self.policy([{"pattern": "*"}]))])
        self.assertEqual(1, code)
        self.assertIn("missing reason", err.getvalue())

    def test_excluded_files_leave_totals_and_are_listed(self) -> None:
        lcov = self.write("c.info", _record("/b/src/tools/t/x_autogen/moc_a.cpp", {1: 0, 2: 0})
                          + _record("/b/src/tools/t/main.cpp", {1: 1}))
        policy = self.policy([{"pattern": "*_autogen/*", "reason": "generated"}])
        out, summary = self.root / "f.info", self.root / "s.json"
        with redirect_stdout(io.StringIO()):
            code = coverage.main([str(lcov), "--policy", str(policy), "--lcov-out", str(out), "--json", str(summary)])
        self.assertEqual(0, code)
        self.assertNotIn("autogen", out.read_text(encoding="utf-8"))
        data = json.loads(summary.read_text(encoding="utf-8"))
        self.assertEqual((1, 1), (data["totals"]["lines_found"], data["totals"]["lines_hit"]))
        self.assertEqual([{"pattern": "*_autogen/*", "reason": "generated", "reported_files": 1,
                           "reported_lines": 2, "tracked_files": 0}], data["exclusions"])

    def test_stale_exclusion_is_reported(self) -> None:
        entries = [{"pattern": "src/tools/gone/*", "reason": "x"}, {"pattern": "src/tools/t/*", "reason": "y"}]
        self.assertEqual(["src/tools/gone/*"], coverage.stale_exclusions(entries, {"src/tools/t/a.cpp"}))


class TestCompleteness(unittest.TestCase):
    SOURCES = {
        "src/libraries/a/reported.cpp": "int f() { return 1; }\n",
        "src/libraries/a/unbuilt.cpp": "int g() { return 2; }\n",
        "src/libraries/a/empty.cpp": "/* header */\n#include \"empty.h\"\n\nusing namespace A;\n// done\n",
        "src/libraries/a/header.h": "int f();\n",
        "src/tools/t/template/plugin.cpp": "void h() {}\n",
    }

    def check(self, reported: set[str], exclusions: list | None = None) -> dict:
        return coverage.check_completeness(reported, sorted(self.SOURCES), exclusions or [], self.SOURCES.__getitem__)

    def test_every_translation_unit_is_accounted_for(self) -> None:
        """AC-T2.2-1: a compiled-nowhere unit with code is reported as missing."""
        result = self.check({"src/libraries/a/reported.cpp"},
                            [{"pattern": "src/tools/t/template/*", "reason": "template"}])
        self.assertEqual(["src/libraries/a/unbuilt.cpp"], result["missing"])
        self.assertEqual(["src/libraries/a/empty.cpp"], result["no_executable_code"])
        self.assertEqual((4, 1, 1), (result["translation_units"], result["reported"], result["excluded"]))

    def test_untracked_report_entries_are_flagged(self) -> None:
        result = self.check({"src/libraries/a/reported.cpp", "src/libraries/a/generated.cpp"})
        self.assertEqual(["src/libraries/a/generated.cpp"], result["untracked"])


class TestCodecovModel(CoverageTestCase):
    def test_branch_counters_decide_the_line_and_partials_are_hits(self) -> None:
        files = self.parse(_record("/x/src/libraries/a/b.cpp", {1: 1, 2: 1, 3: 0, 4: 5},
                                   ("2,0,0,1", "2,0,1,0", "4,0,0,0", "4,0,1,-")))
        self.assertEqual((4, 2), coverage.codecov_counts(files[0]))

    def test_reconcile_names_every_difference(self) -> None:
        files = self.parse(_record("/x/src/libraries/a/b.cpp", {1: 1, 2: 0})
                           + _record("/x/src/libraries/a/c.cpp", {1: 1}))
        codecov = {"files": [{"name": "src/libraries/a/b.cpp", "totals": {"lines": 2, "hits": 1}},
                             {"name": "src/libraries/a/d.cpp", "totals": {"lines": 1, "hits": 0}}]}
        self.assertEqual(["src/libraries/a/c.cpp: missing from Codecov",
                          "src/libraries/a/d.cpp: in Codecov but not in the report"],
                         coverage.reconcile(files, codecov))
        codecov["files"][0]["totals"]["hits"] = 2
        self.assertIn("src/libraries/a/b.cpp: Codecov 2/2, expected 1/2", coverage.reconcile(files, codecov))


class TestSummary(CoverageTestCase):
    def test_priority_order_is_deterministic(self) -> None:
        files = self.parse(_record("/c/src/libraries/mne/z.cpp", {1: 1, 2: 0})
                           + _record("/c/src/libraries/mne/a.cpp", {1: 1, 2: 0})
                           + _record("/c/src/libraries/mne/b.cpp", {1: 0, 2: 0, 3: 0}))
        summary = coverage.build_summary(files, commit=None, source_url=None, top=2)
        self.assertEqual(["src/libraries/mne/b.cpp", "src/libraries/mne/a.cpp"],
                         [item["path"] for item in summary["priority_files"]])


class TestRepositoryConfiguration(unittest.TestCase):
    """The committed policy, codecov.yml and workflow describe one coverage model."""

    def test_committed_exclusions_have_reasons(self) -> None:
        entries = coverage.load_exclusions(coverage.POLICY_FILE)
        self.assertTrue(entries)
        self.assertEqual(len(entries), len({entry["pattern"] for entry in entries}))

    def test_every_module_has_a_codecov_component(self) -> None:
        text = (_REPO_ROOT / "codecov.yml").read_text(encoding="utf-8")
        declared = dict(re.findall(r"component_id: (\S+)\s+name: \S+\s+paths:\s+- (\S+)", text))
        expected = {scope: f"src/{scope}/**" for scope in coverage.SCOPES}
        for scope in coverage.SCOPES:
            for module in sorted(p for p in (_REPO_ROOT / "src" / scope).iterdir() if p.is_dir()):
                expected[f"{scope}_{module.name}"] = f"src/{scope}/{module.name}/**"
        self.assertEqual(expected, declared)

    def test_codecov_counts_lines_like_the_gate(self) -> None:
        text = (_REPO_ROOT / "codecov.yml").read_text(encoding="utf-8")
        self.assertRegex(text, r"lcov:\s+partials_as_hits: true")
        self.assertRegex(text, r"project:\s+default:\s+target: auto\s+threshold: 0\.1%")
        self.assertRegex(text, r"patch:\s+default:\s+target: 90%")

    def test_workflow_uses_the_model(self) -> None:
        text = (_REPO_ROOT / ".github" / "workflows" / "_reusable-tests.yml").read_text(encoding="utf-8")
        self.assertIn("fastcov --process-gcno --branch-coverage", text)
        self.assertRegex(text, r"summarize_coverage\.py coverage_raw\.info\s+\\\s+--repo-root \. --lcov-out coverage_filtered\.info")
        self.assertIn("disable_file_fixes: true", text)
        self.assertIn("check_coverage_gate.py coverage_filtered.info", text)


if __name__ == "__main__":
    unittest.main()
