#!/usr/bin/env python3
# =============================================================================================================
#
# @file     test_audit_dependencies.py
# @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
# @since    2.4.0
# @date     July, 2026
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#
# @brief    Fixtures proving the dependency audit rejects the graphs it claims to reject.
#
# =============================================================================================================
"""Fixtures for ``tools/quality/audit_dependencies.py``.

Each case builds a synthetic ``src/libraries`` in a temporary directory that
breaks exactly one architectural rule, and asserts the matching finding code.
The last two cases run the real tool against the real tree, so the shipped
policy cannot drift away from the repository it describes.

Run with::

    python3 -m unittest discover -s tools/quality/tests -v
"""

from __future__ import annotations

import contextlib
import datetime
import importlib.util
import io
import json
import sys
import tempfile
import unittest
from pathlib import Path

_THIS_DIR = Path(__file__).resolve().parent
_AUDIT_PATH = _THIS_DIR.parent / "audit_dependencies.py"

_spec = importlib.util.spec_from_file_location("audit_dependencies", _AUDIT_PATH)
assert _spec is not None and _spec.loader is not None
audit = importlib.util.module_from_spec(_spec)
sys.modules["audit_dependencies"] = audit
_spec.loader.exec_module(audit)


LIBRARY_TEMPLATE = """\
cmake_minimum_required(VERSION 3.14)
project({name} LANGUAGES CXX)

set(MNE_LIBS_REQUIRED
{deps}
)

add_library({name} SHARED {name}.cpp)

target_link_libraries(${{PROJECT_NAME}} PRIVATE
    ${{MNE_LIBS_REQUIRED}}
    {external}
)
"""


class AuditTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.libraries = Path(self._tmp.name) / "src" / "libraries"
        self.libraries.mkdir(parents=True)

    def add_library(self, directory: str, deps: list[str] | None = None, external: str = "") -> None:
        path = self.libraries / directory
        path.mkdir()
        body = "\n".join(f"    {d}" for d in (deps or []))
        (path / "CMakeLists.txt").write_text(
            LIBRARY_TEMPLATE.format(name=f"mne_{directory}", deps=body, external=external),
            encoding="utf-8",
        )

    def graph(self, prefixes: dict[str, str] | None = None) -> dict:
        return audit.build_graph(self.libraries, prefixes)

    @staticmethod
    def policy(layers: list[tuple[str, list[str]]], **extra) -> dict:
        policy = {
            "layers": [{"name": name, "libraries": libs} for name, libs in layers],
            "forbidden_edges": [],
            "allowed_external": [],
            "ignored_variables": [],
            "exceptions": [],
        }
        policy.update(extra)
        return policy

    @staticmethod
    def codes(findings) -> list[str]:
        return [f.code for f in findings]

    def evaluate(self, policy: dict, today: datetime.date | None = None) -> list[str]:
        """Run the whole rule set the way the command line does."""
        graph = self.graph(policy.get("variable_prefixes", {}))
        index, names, findings = audit.layer_index(policy)
        findings += audit.check_layer_assignment(graph, index, policy)
        exception_findings, exempt = audit.check_exceptions(
            graph, policy, today or datetime.date(2026, 7, 1)
        )
        findings += exception_findings
        findings += audit.check_edges(graph, index, names, policy, exempt)
        findings += audit.check_external(graph, policy)
        return self.codes(findings)


# --------------------------------------------------------------------------------------------------------
# AC-T6.7-1: the graph is reproducible
# --------------------------------------------------------------------------------------------------------


class TestGraphExtraction(AuditTestCase):
    def test_dependencies_are_read_through_the_variable(self) -> None:
        self.add_library("math")
        self.add_library("utils", ["mne_math"])
        self.add_library("fiff", ["mne_utils", "mne_math"])
        graph = self.graph()
        self.assertEqual(["mne_math", "mne_utils"], graph["mne_fiff"].depends_on)
        self.assertEqual([], graph["mne_math"].depends_on)

    def test_commented_out_dependency_is_ignored(self) -> None:
        self.add_library("math")
        path = self.libraries / "math" / "CMakeLists.txt"
        path.write_text(
            path.read_text(encoding="utf-8") + "\n# target_link_libraries(mne_math PRIVATE mne_ghost)\n",
            encoding="utf-8",
        )
        self.assertEqual([], self.graph()["mne_math"].depends_on)

    def test_external_dependencies_are_separated_from_internal_ones(self) -> None:
        self.add_library("math", external="Eigen3::Eigen")
        graph = self.graph()
        self.assertEqual([], graph["mne_math"].depends_on)
        self.assertEqual(["Eigen3::Eigen"], graph["mne_math"].external)

    def test_qt_targets_are_normalised_across_spellings(self) -> None:
        self.add_library("math", external="Qt6::Core Qt${QT_VERSION_MAJOR}::Gui")
        self.assertEqual(["Qt::Core", "Qt::Gui"], self.graph()["mne_math"].external)
        self.assertEqual([], self.graph()["mne_math"].unresolved)

    def test_variable_prefixes_recover_bare_qt_module_names(self) -> None:
        path = self.libraries / "math"
        path.mkdir()
        (path / "CMakeLists.txt").write_text(
            "project(mne_math LANGUAGES CXX)\n"
            "set(QT_REQUIRED_COMPONENTS Core Network)\n"
            "target_link_libraries(mne_math PRIVATE ${QT_REQUIRED_COMPONENTS})\n",
            encoding="utf-8",
        )
        graph = self.graph({"QT_REQUIRED_COMPONENTS": "Qt::"})
        self.assertEqual(["Qt::Core", "Qt::Network"], graph["mne_math"].external)

    def test_unresolvable_variable_is_recorded(self) -> None:
        self.add_library("math", external="${MYSTERY_LIBS}")
        self.assertEqual(["MYSTERY_LIBS"], self.graph()["mne_math"].unresolved)

    def test_graph_output_is_deterministic(self) -> None:
        self.add_library("math")
        self.add_library("utils", ["mne_math"])
        policy = self.policy([("core", ["mne_math"]), ("support", ["mne_utils"])])
        index, names, _ = audit.layer_index(policy)
        first = json.dumps(audit.graph_as_dict(self.graph(), index, names), sort_keys=True)
        second = json.dumps(audit.graph_as_dict(self.graph(), index, names), sort_keys=True)
        self.assertEqual(first, second)


# --------------------------------------------------------------------------------------------------------
# AC-T6.7-2: a synthetic forbidden edge or cycle fails
# --------------------------------------------------------------------------------------------------------


class TestLayerViolations(AuditTestCase):
    def test_clean_stack_passes(self) -> None:
        self.add_library("math")
        self.add_library("utils", ["mne_math"])
        self.add_library("fiff", ["mne_utils"])
        policy = self.policy(
            [("core", ["mne_math"]), ("support", ["mne_utils"]), ("io", ["mne_fiff"])]
        )
        self.assertEqual([], self.evaluate(policy))

    def test_edge_into_a_higher_layer_fails(self) -> None:
        self.add_library("math", ["mne_fiff"])
        self.add_library("fiff")
        policy = self.policy([("core", ["mne_math"]), ("io", ["mne_fiff"])])
        self.assertIn("DEP010", self.evaluate(policy))

    def test_edge_within_the_same_layer_fails(self) -> None:
        self.add_library("math")
        self.add_library("utils", ["mne_math"])
        policy = self.policy([("core", ["mne_math", "mne_utils"])])
        self.assertIn("DEP010", self.evaluate(policy))

    def test_unassigned_library_fails(self) -> None:
        self.add_library("math")
        self.assertIn("DEP001", self.evaluate(self.policy([("core", [])])))

    def test_policy_naming_a_missing_library_fails(self) -> None:
        self.add_library("math")
        policy = self.policy([("core", ["mne_math", "mne_ghost"])])
        self.assertIn("DEP002", self.evaluate(policy))

    def test_library_in_two_layers_fails(self) -> None:
        self.add_library("math")
        policy = self.policy([("core", ["mne_math"]), ("support", ["mne_math"])])
        self.assertIn("DEP003", self.evaluate(policy))

    def test_dependency_on_an_unknown_library_fails(self) -> None:
        self.add_library("math", ["mne_ghost"])
        self.assertIn("DEP004", self.evaluate(self.policy([("core", ["mne_math"])])))


class TestCycles(AuditTestCase):
    def test_two_library_cycle_is_detected(self) -> None:
        self.add_library("alpha", ["mne_beta"])
        self.add_library("beta", ["mne_alpha"])
        cycles = audit.find_cycles(self.graph())
        self.assertEqual(1, len(cycles))
        self.assertEqual(cycles[0][0], cycles[0][-1])
        self.assertEqual({"mne_alpha", "mne_beta"}, set(cycles[0]))

    def test_three_library_cycle_is_detected(self) -> None:
        self.add_library("alpha", ["mne_beta"])
        self.add_library("beta", ["mne_gamma"])
        self.add_library("gamma", ["mne_alpha"])
        self.assertEqual(1, len(audit.find_cycles(self.graph())))

    def test_cycle_is_reported_as_a_violation(self) -> None:
        self.add_library("alpha", ["mne_beta"])
        self.add_library("beta", ["mne_alpha"])
        policy = self.policy([("core", ["mne_alpha"]), ("support", ["mne_beta"])])
        self.assertIn("DEP011", self.evaluate(policy))

    def test_a_directed_acyclic_graph_has_no_cycles(self) -> None:
        self.add_library("math")
        self.add_library("utils", ["mne_math"])
        self.add_library("fiff", ["mne_utils", "mne_math"])
        self.assertEqual([], audit.find_cycles(self.graph()))

    def test_a_diamond_is_not_a_cycle(self) -> None:
        self.add_library("base")
        self.add_library("left", ["mne_base"])
        self.add_library("right", ["mne_base"])
        self.add_library("top", ["mne_left", "mne_right"])
        self.assertEqual([], audit.find_cycles(self.graph()))


class TestForbiddenEdges(AuditTestCase):
    def test_forbidden_edge_fails(self) -> None:
        self.add_library("math")
        self.add_library("utils", ["mne_math"])
        policy = self.policy(
            [("core", ["mne_math"]), ("support", ["mne_utils"])],
            forbidden_edges=[{"from": "mne_utils", "to": "mne_math", "reason": "kept deliberately apart"}],
        )
        codes = self.evaluate(policy)
        self.assertIn("DEP012", codes)

    def test_forbidden_edge_that_is_absent_passes(self) -> None:
        self.add_library("math")
        self.add_library("utils", ["mne_math"])
        policy = self.policy(
            [("core", ["mne_math"]), ("support", ["mne_utils"])],
            forbidden_edges=[{"from": "mne_math", "to": "mne_utils", "reason": "would invert the stack"}],
        )
        self.assertEqual([], self.evaluate(policy))


class TestExternalDependencies(AuditTestCase):
    def test_unlisted_third_party_target_fails(self) -> None:
        self.add_library("math", external="boost::asio")
        policy = self.policy([("core", ["mne_math"])])
        self.assertIn("DEP020", self.evaluate(policy))

    def test_listed_third_party_target_passes(self) -> None:
        self.add_library("math", external="Eigen3::Eigen")
        policy = self.policy([("core", ["mne_math"])], allowed_external=["Eigen3::Eigen"])
        self.assertEqual([], self.evaluate(policy))

    def test_unknown_variable_warns_but_does_not_fail(self) -> None:
        self.add_library("math", external="${MYSTERY_LIBS}")
        policy = self.policy([("core", ["mne_math"])])
        findings = audit.check_external(self.graph(), policy)
        self.assertEqual(["DEP021"], self.codes(findings))
        self.assertEqual("warning", findings[0].severity)

    def test_reviewed_variable_is_silent(self) -> None:
        self.add_library("math", external="${MYSTERY_LIBS}")
        policy = self.policy([("core", ["mne_math"])], ignored_variables=["MYSTERY_LIBS"])
        self.assertEqual([], audit.check_external(self.graph(), policy))


# --------------------------------------------------------------------------------------------------------
# AC-T6.7-3: exceptions carry owner, rationale and expiry
# --------------------------------------------------------------------------------------------------------


class TestExceptions(AuditTestCase):
    TODAY = datetime.date(2026, 7, 1)

    def setUp(self) -> None:
        super().setUp()
        # An inverted stack: core depends on io.
        self.add_library("math", ["mne_fiff"])
        self.add_library("fiff")
        self.layers = [("core", ["mne_math"]), ("io", ["mne_fiff"])]

    def exception(self, **overrides) -> dict:
        entry = {
            "from": "mne_math",
            "to": "mne_fiff",
            "owner": "A Maintainer <a@example.org>",
            "rationale": "the shared reader has not been extracted yet",
            "expires": "2026-12-31",
        }
        entry.update(overrides)
        return {k: v for k, v in entry.items() if v is not None}

    def test_violation_without_an_exception_fails(self) -> None:
        self.assertIn("DEP010", self.evaluate(self.policy(self.layers), self.TODAY))

    def test_complete_exception_suppresses_the_violation(self) -> None:
        policy = self.policy(self.layers, exceptions=[self.exception()])
        self.assertEqual([], self.evaluate(policy, self.TODAY))

    def test_missing_owner_fails(self) -> None:
        policy = self.policy(self.layers, exceptions=[self.exception(owner=None)])
        codes = self.evaluate(policy, self.TODAY)
        self.assertIn("DEP030", codes)
        self.assertIn("DEP010", codes)

    def test_missing_rationale_fails(self) -> None:
        policy = self.policy(self.layers, exceptions=[self.exception(rationale=None)])
        self.assertIn("DEP030", self.evaluate(policy, self.TODAY))

    def test_missing_expiry_fails(self) -> None:
        policy = self.policy(self.layers, exceptions=[self.exception(expires=None)])
        self.assertIn("DEP030", self.evaluate(policy, self.TODAY))

    def test_expired_exception_fails(self) -> None:
        policy = self.policy(self.layers, exceptions=[self.exception(expires="2026-06-30")])
        self.assertIn("DEP031", self.evaluate(policy, self.TODAY))

    def test_imminent_expiry_warns(self) -> None:
        policy = self.policy(self.layers, exceptions=[self.exception(expires="2026-07-20")])
        findings, _ = audit.check_exceptions(self.graph(), policy, self.TODAY)
        self.assertEqual(["DEP035"], self.codes(findings))
        self.assertEqual("warning", findings[0].severity)

    def test_malformed_expiry_fails(self) -> None:
        policy = self.policy(self.layers, exceptions=[self.exception(expires="31.12.2026")])
        self.assertIn("DEP034", self.evaluate(policy, self.TODAY))

    def test_exception_for_an_absent_edge_is_reported_as_stale(self) -> None:
        policy = self.policy(self.layers, exceptions=[self.exception(**{"from": "mne_fiff", "to": "mne_math"})])
        findings, exempt = audit.check_exceptions(self.graph(), policy, self.TODAY)
        self.assertEqual(["DEP032"], self.codes(findings))
        self.assertEqual(set(), exempt)

    def test_exception_naming_an_unknown_library_fails(self) -> None:
        policy = self.policy(self.layers, exceptions=[self.exception(**{"to": "mne_ghost"})])
        self.assertIn("DEP033", self.evaluate(policy, self.TODAY))


# --------------------------------------------------------------------------------------------------------
# The shipped policy must describe the tree it ships with
# --------------------------------------------------------------------------------------------------------


class TestShippedArchitecture(unittest.TestCase):
    def test_the_real_graph_satisfies_the_real_policy(self) -> None:
        report = io.StringIO()
        with contextlib.redirect_stdout(report):
            status = audit.main(["--check"])
        self.assertEqual(0, status, report.getvalue())

    def test_the_real_graph_is_reproducible(self) -> None:
        policy = json.loads(audit.POLICY_FILE.read_text(encoding="utf-8"))
        prefixes = policy.get("variable_prefixes", {})
        index, names, _ = audit.layer_index(policy)
        first = audit.graph_as_dict(audit.build_graph(audit.LIBRARIES_DIR, prefixes), index, names)
        second = audit.graph_as_dict(audit.build_graph(audit.LIBRARIES_DIR, prefixes), index, names)
        self.assertEqual(json.dumps(first, sort_keys=True), json.dumps(second, sort_keys=True))

    def test_every_library_is_assigned_to_exactly_one_layer(self) -> None:
        policy = json.loads(audit.POLICY_FILE.read_text(encoding="utf-8"))
        assigned: list[str] = []
        for layer in policy["layers"]:
            assigned.extend(layer["libraries"])
        self.assertEqual(sorted(assigned), sorted(set(assigned)), "a library is listed in two layers")
        graph = audit.build_graph(audit.LIBRARIES_DIR, policy.get("variable_prefixes", {}))
        self.assertEqual(sorted(graph), sorted(assigned))

    def test_every_layer_states_what_it_is_for(self) -> None:
        policy = json.loads(audit.POLICY_FILE.read_text(encoding="utf-8"))
        for layer in policy["layers"]:
            self.assertTrue(str(layer.get("description", "")).strip(), f"{layer['name']} has no description")


if __name__ == "__main__":
    unittest.main()
