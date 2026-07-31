#!/usr/bin/env python3
# =============================================================================================================
#
# @file     test_validate_test_inventory.py
# @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
# @since    2.4.0
# @date     July, 2026
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#
# @brief    Fixtures proving the test-inventory validator rejects what it claims to reject.
#
# =============================================================================================================
"""Fixtures for ``tools/quality/validate_test_inventory.py``.

A validator nobody has tried to fool is a validator nobody should trust, so each
check here is fed a synthetic tree that violates exactly one invariant and is
asserted to produce exactly that finding code.  The trees are built in a
temporary directory; the real ``src/testframes`` is never touched.

Run with::

    python3 -m unittest discover -s tools/quality/tests -v
"""

from __future__ import annotations

import datetime
import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path

_THIS_DIR = Path(__file__).resolve().parent
_VALIDATOR_PATH = _THIS_DIR.parent / "validate_test_inventory.py"

_spec = importlib.util.spec_from_file_location("validate_test_inventory", _VALIDATOR_PATH)
assert _spec is not None and _spec.loader is not None
vti = importlib.util.module_from_spec(_spec)
sys.modules["validate_test_inventory"] = vti
_spec.loader.exec_module(vti)


# --------------------------------------------------------------------------------------------------------
# Fixture construction
# --------------------------------------------------------------------------------------------------------

LEAF_TEMPLATE = """\
cmake_minimum_required(VERSION 3.14)
project({name} LANGUAGES CXX)

add_executable({name} {name}.cpp)
{extra}
"""

DEFAULT_POLICY = {
    "allowed_labels": ["unit", "integration", "parity", "example", "gui", "slow", "requires-data"],
    "timeout_seconds": {"min": 5, "max": 3600},
    "unregistered": {},
    "ratchet": {"max_tests_without_labels": 100, "max_tests_without_timeout": 100},
    "quarantine": [],
}

ADD_TEST = "add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})"


class InventoryFixture:
    """A throwaway ``src/testframes`` built to break one rule at a time."""

    def __init__(self, root: Path) -> None:
        self.root = root
        self.testframes = root / "src" / "testframes"
        self.testframes.mkdir(parents=True)
        self._registered: list[str] = []

    def add_test_dir(self, name: str, *, extra: str = ADD_TEST, register: bool = True) -> None:
        directory = self.testframes / name
        directory.mkdir()
        (directory / "CMakeLists.txt").write_text(
            LEAF_TEMPLATE.format(name=name, extra=extra), encoding="utf-8"
        )
        if register:
            self._registered.append(name)

    def register_only(self, name: str) -> None:
        """Name a directory in the root file without creating it."""
        self._registered.append(name)

    def write_root(self) -> None:
        lines = ["set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/tests)", ""]
        lines += [f"add_subdirectory({name})" for name in self._registered]
        (self.testframes / "CMakeLists.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")

    def collect(self):
        self.write_root()
        return vti.collect_inventory(self.testframes, self.testframes / "CMakeLists.txt")


class ValidatorTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.root = Path(self._tmp.name)
        self.fixture = InventoryFixture(self.root)
        self.policy = json.loads(json.dumps(DEFAULT_POLICY))

    @staticmethod
    def codes(findings) -> list[str]:
        return [f.code for f in findings]

    def write_registry(self, payload: object) -> Path:
        path = self.root / "api_registry.json"
        path.write_text(json.dumps(payload), encoding="utf-8")
        return path


# --------------------------------------------------------------------------------------------------------
# AC-T1.4-1: deleting add_test() or using an unknown label fails validation
# --------------------------------------------------------------------------------------------------------


class TestAddTestRegistration(ValidatorTestCase):
    def test_missing_add_test_is_an_error(self) -> None:
        self.fixture.add_test_dir("test_alpha", extra="")
        entries, _ = self.fixture.collect()
        self.assertIn("TI010", self.codes(vti.check_add_test(entries)))

    def test_present_add_test_passes(self) -> None:
        self.fixture.add_test_dir("test_alpha")
        entries, _ = self.fixture.collect()
        self.assertEqual([], vti.check_add_test(entries))

    def test_multiline_add_test_is_recognised(self) -> None:
        self.fixture.add_test_dir(
            "test_alpha",
            extra="add_test(\n    NAME ${PROJECT_NAME}\n    COMMAND ${PROJECT_NAME}\n)",
        )
        entries, _ = self.fixture.collect()
        self.assertTrue(entries["test_alpha"].has_add_test)
        self.assertEqual([], vti.check_add_test(entries))

    def test_commented_out_add_test_does_not_count(self) -> None:
        self.fixture.add_test_dir("test_alpha", extra="# add_test(NAME x COMMAND x)")
        entries, _ = self.fixture.collect()
        self.assertFalse(entries["test_alpha"].has_add_test)
        self.assertIn("TI010", self.codes(vti.check_add_test(entries)))

    def test_hash_inside_a_quoted_string_is_not_a_comment(self) -> None:
        self.fixture.add_test_dir(
            "test_alpha",
            extra='set_tests_properties(x PROPERTIES ENVIRONMENT "TAG=a#b")\n' + ADD_TEST,
        )
        entries, _ = self.fixture.collect()
        self.assertTrue(entries["test_alpha"].has_add_test)


class TestLabels(ValidatorTestCase):
    def _with_labels(self, labels: str) -> dict:
        self.fixture.add_test_dir(
            "test_alpha",
            extra=f'{ADD_TEST}\nset_tests_properties(${{PROJECT_NAME}} PROPERTIES LABELS "{labels}")',
        )
        entries, _ = self.fixture.collect()
        return entries

    def test_unknown_label_is_an_error(self) -> None:
        entries = self._with_labels("unit;banana")
        self.assertIn("TI030", self.codes(vti.check_labels(entries, self.policy)))

    def test_allowed_labels_pass(self) -> None:
        entries = self._with_labels("unit;slow;requires-data")
        self.assertEqual(["requires-data", "slow", "unit"], entries["test_alpha"].labels)
        self.assertEqual([], vti.check_labels(entries, self.policy))


# --------------------------------------------------------------------------------------------------------
# AC-T1.4-2: registry references resolve to a registered test
# --------------------------------------------------------------------------------------------------------


class TestRegistryReferences(ValidatorTestCase):
    def test_reference_to_running_test_passes(self) -> None:
        self.fixture.add_test_dir("test_alpha")
        entries, _ = self.fixture.collect()
        registry = self.write_registry({"classes": [{"name": "A", "test": "test_alpha"}]})
        self.assertEqual([], vti.check_registry_references(entries, registry))

    def test_reference_to_unknown_test_is_an_error(self) -> None:
        self.fixture.add_test_dir("test_alpha")
        entries, _ = self.fixture.collect()
        registry = self.write_registry({"classes": [{"name": "A", "test": "test_ghost"}]})
        findings = vti.check_registry_references(entries, registry)
        self.assertIn("TI062", self.codes(findings))
        self.assertIn("no such directory", findings[0].message)

    def test_reference_to_unregistered_test_is_an_error(self) -> None:
        self.fixture.add_test_dir("test_alpha", register=False)
        entries, _ = self.fixture.collect()
        registry = self.write_registry({"classes": [{"name": "A", "test": "test_alpha"}]})
        findings = vti.check_registry_references(entries, registry)
        self.assertIn("TI062", self.codes(findings))
        self.assertIn("not registered", findings[0].message)

    def test_reference_to_test_without_add_test_is_an_error(self) -> None:
        self.fixture.add_test_dir("test_alpha", extra="")
        entries, _ = self.fixture.collect()
        registry = self.write_registry({"classes": [{"name": "A", "test": "test_alpha"}]})
        findings = vti.check_registry_references(entries, registry)
        self.assertIn("TI062", self.codes(findings))
        self.assertIn("never calls add_test", findings[0].message)

    def test_nested_and_null_references_are_handled(self) -> None:
        self.fixture.add_test_dir("test_alpha")
        entries, _ = self.fixture.collect()
        registry = self.write_registry(
            {
                "modules": {"fiff": {"classes": [{"test": "test_alpha"}, {"test": None}]}},
                "classes": [{"methods": [{"test": "test_alpha"}]}],
            }
        )
        self.assertEqual([], vti.check_registry_references(entries, registry))


# --------------------------------------------------------------------------------------------------------
# AC-T1.4-3: quarantine requires issue/owner/expiry
# --------------------------------------------------------------------------------------------------------


class TestQuarantine(ValidatorTestCase):
    TODAY = datetime.date(2026, 7, 1)

    def _entries(self) -> dict:
        self.fixture.add_test_dir("test_alpha")
        entries, _ = self.fixture.collect()
        return entries

    def _check(self, entry: dict) -> list[str]:
        self.policy["quarantine"] = [entry]
        return self.codes(vti.check_quarantine(self._entries(), self.policy, self.TODAY))

    def test_complete_entry_passes(self) -> None:
        self.assertEqual(
            [],
            self._check(
                {
                    "test": "test_alpha",
                    "owner": "A Maintainer <a@example.org>",
                    "issue": "https://example.org/issues/1",
                    "expires": "2026-12-31",
                    "reason": "upstream driver regression",
                }
            ),
        )

    def test_missing_owner_is_an_error(self) -> None:
        codes = self._check(
            {
                "test": "test_alpha",
                "issue": "https://example.org/issues/1",
                "expires": "2026-12-31",
                "reason": "r",
            }
        )
        self.assertIn("TI071", codes)

    def test_missing_issue_is_an_error(self) -> None:
        codes = self._check(
            {"test": "test_alpha", "owner": "o", "expires": "2026-12-31", "reason": "r"}
        )
        self.assertIn("TI071", codes)

    def test_missing_expiry_is_an_error(self) -> None:
        codes = self._check(
            {"test": "test_alpha", "owner": "o", "issue": "i", "reason": "r"}
        )
        self.assertIn("TI071", codes)

    def test_expired_quarantine_is_an_error(self) -> None:
        codes = self._check(
            {
                "test": "test_alpha",
                "owner": "o",
                "issue": "i",
                "expires": "2026-06-30",
                "reason": "r",
            }
        )
        self.assertIn("TI075", codes)

    def test_imminent_expiry_warns(self) -> None:
        findings = vti.check_quarantine(
            self._entries(),
            {
                "quarantine": [
                    {
                        "test": "test_alpha",
                        "owner": "o",
                        "issue": "i",
                        "expires": "2026-07-07",
                        "reason": "r",
                    }
                ]
            },
            self.TODAY,
        )
        self.assertEqual(["TI076"], self.codes(findings))
        self.assertEqual("warning", findings[0].severity)

    def test_malformed_date_is_an_error(self) -> None:
        codes = self._check(
            {
                "test": "test_alpha",
                "owner": "o",
                "issue": "i",
                "expires": "31.12.2026",
                "reason": "r",
            }
        )
        self.assertIn("TI074", codes)

    def test_quarantining_an_unknown_test_is_an_error(self) -> None:
        codes = self._check(
            {
                "test": "test_ghost",
                "owner": "o",
                "issue": "i",
                "expires": "2026-12-31",
                "reason": "r",
            }
        )
        self.assertIn("TI073", codes)


# --------------------------------------------------------------------------------------------------------
# Registration, naming, output directories and bounds
# --------------------------------------------------------------------------------------------------------


class TestRegistration(ValidatorTestCase):
    def test_unregistered_directory_is_an_error(self) -> None:
        self.fixture.add_test_dir("test_alpha", register=False)
        entries, missing = self.fixture.collect()
        self.assertIn("TI003", self.codes(vti.check_registration(entries, missing, self.policy)))

    def test_unregistered_directory_with_recorded_reason_passes(self) -> None:
        self.fixture.add_test_dir("test_alpha", register=False)
        entries, missing = self.fixture.collect()
        self.policy["unregistered"] = {"test_alpha": "standalone demo, not a CTest case"}
        self.assertEqual([], vti.check_registration(entries, missing, self.policy))

    def test_stale_excuse_for_a_registered_test_is_an_error(self) -> None:
        self.fixture.add_test_dir("test_alpha")
        entries, missing = self.fixture.collect()
        self.policy["unregistered"] = {"test_alpha": "no longer true"}
        self.assertIn("TI002", self.codes(vti.check_registration(entries, missing, self.policy)))

    def test_excuse_for_a_deleted_directory_is_an_error(self) -> None:
        entries, missing = self.fixture.collect()
        self.policy["unregistered"] = {"test_gone": "deleted last release"}
        self.assertIn("TI004", self.codes(vti.check_registration(entries, missing, self.policy)))

    def test_registering_a_missing_directory_is_an_error(self) -> None:
        self.fixture.register_only("test_absent")
        entries, missing = self.fixture.collect()
        self.assertEqual(["test_absent"], missing)
        self.assertIn("TI001", self.codes(vti.check_registration(entries, missing, self.policy)))


class TestOutputDirectoryOverride(ValidatorTestCase):
    def test_output_directory_override_is_an_error(self) -> None:
        self.fixture.add_test_dir(
            "test_alpha",
            extra=(
                "set_target_properties(${PROJECT_NAME} PROPERTIES\n"
                '    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../../out/tests")\n' + ADD_TEST
            ),
        )
        entries, _ = self.fixture.collect()
        self.assertIn("TI011", self.codes(vti.check_output_directory_overrides(entries)))

    def test_inherited_output_directory_passes(self) -> None:
        self.fixture.add_test_dir(
            "test_alpha",
            extra='set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "tests")\n' + ADD_TEST,
        )
        entries, _ = self.fixture.collect()
        self.assertEqual([], vti.check_output_directory_overrides(entries))


class TestNames(ValidatorTestCase):
    def test_project_name_must_match_directory(self) -> None:
        directory = self.fixture.testframes / "test_alpha"
        directory.mkdir()
        (directory / "CMakeLists.txt").write_text(
            LEAF_TEMPLATE.format(name="test_beta", extra=ADD_TEST), encoding="utf-8"
        )
        self.fixture._registered.append("test_alpha")
        entries, _ = self.fixture.collect()
        self.assertIn("TI020", self.codes(vti.check_names(entries)))

    def test_matching_names_pass(self) -> None:
        self.fixture.add_test_dir("test_alpha")
        self.fixture.add_test_dir("test_beta")
        entries, _ = self.fixture.collect()
        self.assertEqual([], vti.check_names(entries))


class TestTimeouts(ValidatorTestCase):
    def _with_timeout(self, seconds: int) -> dict:
        self.fixture.add_test_dir(
            "test_alpha",
            extra=f"{ADD_TEST}\nset_tests_properties(${{PROJECT_NAME}} PROPERTIES TIMEOUT {seconds})",
        )
        entries, _ = self.fixture.collect()
        return entries

    def test_timeout_within_bounds_passes(self) -> None:
        entries = self._with_timeout(600)
        self.assertEqual(600, entries["test_alpha"].timeout)
        self.assertEqual([], vti.check_timeouts(entries, self.policy))

    def test_timeout_below_the_floor_is_an_error(self) -> None:
        self.assertIn("TI040", self.codes(vti.check_timeouts(self._with_timeout(1), self.policy)))

    def test_timeout_above_the_ceiling_is_an_error(self) -> None:
        self.assertIn("TI040", self.codes(vti.check_timeouts(self._with_timeout(99999), self.policy)))


class TestRatchet(ValidatorTestCase):
    def test_exceeding_the_ratchet_is_an_error(self) -> None:
        self.fixture.add_test_dir("test_alpha")
        entries, _ = self.fixture.collect()
        self.policy["ratchet"] = {"max_tests_without_labels": 0, "max_tests_without_timeout": 0}
        findings, actual = vti.check_ratchets(entries, self.policy)
        self.assertEqual({"max_tests_without_labels": 1, "max_tests_without_timeout": 1}, actual)
        self.assertEqual(["TI051", "TI051"], self.codes(findings))

    def test_slack_in_the_ratchet_only_warns(self) -> None:
        self.fixture.add_test_dir("test_alpha")
        entries, _ = self.fixture.collect()
        self.policy["ratchet"] = {"max_tests_without_labels": 5, "max_tests_without_timeout": 5}
        findings, _ = vti.check_ratchets(entries, self.policy)
        self.assertEqual(["TI052", "TI052"], self.codes(findings))
        self.assertTrue(all(f.severity == "warning" for f in findings))


class TestCTestCrossCheck(ValidatorTestCase):
    def test_test_missing_from_ctest_is_an_error(self) -> None:
        self.fixture.add_test_dir("test_alpha")
        entries, _ = self.fixture.collect()
        ctest = self.root / "ctest.json"
        ctest.write_text(json.dumps({"tests": []}), encoding="utf-8")
        self.assertIn("TI081", self.codes(vti.check_ctest_json(entries, ctest)))

    def test_matching_inventories_pass(self) -> None:
        self.fixture.add_test_dir("test_alpha")
        entries, _ = self.fixture.collect()
        ctest = self.root / "ctest.json"
        ctest.write_text(json.dumps({"tests": [{"name": "test_alpha"}]}), encoding="utf-8")
        self.assertEqual([], vti.check_ctest_json(entries, ctest))

class TestConditionalRegistration(ValidatorTestCase):
    GUARDED = "\n".join(
        [
            "if(NOT TARGET some_tool)",
            "    message(STATUS \"skipping\")",
            "    return()",
            "endif()",
            "",
            ADD_TEST,
        ]
    )

    def test_add_test_behind_an_early_return_is_reported(self) -> None:
        self.fixture.add_test_dir("test_guarded", extra=self.GUARDED)
        entries, _ = self.fixture.collect()
        self.assertTrue(entries["test_guarded"].guarded_registration)
        findings = vti.check_conditional_registration(entries, self.policy)
        self.assertIn("TI012", self.codes(findings))

    def test_declared_conditional_test_is_accepted(self) -> None:
        self.fixture.add_test_dir("test_guarded", extra=self.GUARDED)
        entries, _ = self.fixture.collect()
        self.policy["conditional"] = {"test_guarded": "Skipped when some_tool is not built."}
        self.assertEqual(vti.check_conditional_registration(entries, self.policy), [])

    def test_declaration_without_a_reason_is_refused(self) -> None:
        self.fixture.add_test_dir("test_guarded", extra=self.GUARDED)
        entries, _ = self.fixture.collect()
        self.policy["conditional"] = {"test_guarded": "   "}
        findings = vti.check_conditional_registration(entries, self.policy)
        self.assertIn("TI012", self.codes(findings))

    def test_unconditional_registration_is_not_reported(self) -> None:
        self.fixture.add_test_dir("test_plain")
        entries, _ = self.fixture.collect()
        self.assertFalse(entries["test_plain"].guarded_registration)
        self.assertEqual(vti.check_conditional_registration(entries, self.policy), [])

    def test_return_after_add_test_is_not_a_guard(self) -> None:
        self.fixture.add_test_dir("test_late_return", extra=ADD_TEST + "\n\nif(APPLE)\n    return()\nendif()")
        entries, _ = self.fixture.collect()
        self.assertFalse(entries["test_late_return"].guarded_registration)
        self.assertEqual(vti.check_conditional_registration(entries, self.policy), [])

    def test_commented_out_return_is_not_a_guard(self) -> None:
        self.fixture.add_test_dir("test_commented", extra="# return()\n" + ADD_TEST)
        entries, _ = self.fixture.collect()
        self.assertFalse(entries["test_commented"].guarded_registration)

    def test_declaration_for_an_unknown_test_is_refused(self) -> None:
        self.fixture.add_test_dir("test_plain")
        entries, _ = self.fixture.collect()
        self.policy["conditional"] = {"test_gone": "stale"}
        findings = vti.check_conditional_registration(entries, self.policy)
        self.assertIn("TI013", self.codes(findings))

    def test_stale_declaration_is_reported_as_a_warning(self) -> None:
        self.fixture.add_test_dir("test_plain")
        entries, _ = self.fixture.collect()
        self.policy["conditional"] = {"test_plain": "no longer guarded"}
        findings = vti.check_conditional_registration(entries, self.policy)
        self.assertEqual(self.codes(findings), ["TI013"])
        self.assertEqual(findings[0].severity, "warning")


# --------------------------------------------------------------------------------------------------------
# The shipped policy must itself be valid
# --------------------------------------------------------------------------------------------------------


class TestShippedPolicy(unittest.TestCase):
    def test_policy_parses_and_declares_the_required_keys(self) -> None:
        policy = json.loads(vti.POLICY_FILE.read_text(encoding="utf-8"))
        self.assertIn("allowed_labels", policy)
        self.assertIn("timeout_seconds", policy)
        self.assertIn("unregistered", policy)
        self.assertIn("max_tests_without_labels", policy["ratchet"])
        self.assertIn("max_tests_without_timeout", policy["ratchet"])
        self.assertIsInstance(policy["quarantine"], list)

    def test_every_unregistered_entry_states_a_reason(self) -> None:
        policy = json.loads(vti.POLICY_FILE.read_text(encoding="utf-8"))
        for name, reason in policy["unregistered"].items():
            self.assertTrue(str(reason).strip(), f"{name} is excused without a reason")

    def test_every_conditional_entry_states_a_reason(self) -> None:
        policy = json.loads(vti.POLICY_FILE.read_text(encoding="utf-8"))
        for name, reason in policy["conditional"].items():
            self.assertTrue(str(reason).strip(), f"{name} is declared conditional without a reason")

    def test_conditional_list_matches_the_real_tree(self) -> None:
        """The declared list is a defect inventory; it must not drift from what is actually guarded."""
        entries, _ = vti.collect_inventory(vti.TESTFRAMES_DIR, vti.ROOT_TEST_CMAKE)
        policy = json.loads(vti.POLICY_FILE.read_text(encoding="utf-8"))
        guarded = {name for name, entry in entries.items() if entry.registered and entry.guarded_registration}
        self.assertEqual(guarded, set(policy["conditional"]))


if __name__ == "__main__":
    unittest.main()
