# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""CI source-of-truth contracts (v2.4.0 T10.1).

Tool versions, thresholds and gate commands are declared once and every
workflow must agree with that declaration.
"""

import json
import re
import subprocess
import sys
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
WORKFLOWS = REPO_ROOT / ".github" / "workflows"
sys.path.insert(0, str(REPO_ROOT / "scripts" / "quality"))

import check_format  # noqa: E402
import check_tidy  # noqa: E402
import generate_quality_report  # noqa: E402

# Gate commands every PR, staging and main run must execute (G1-G6 static checks)
GATE_COMMANDS = [
    "validate_api_registry.py", "check_format.py --all", "unittest discover -s scripts/quality/tests",
    "validate_test_inventory.py", "extract_setup_commands.py --check", "gap_analysis.py --check",
    "audit_dependencies.py --check", "audit_maintainability.py --ceilings", "scripts.license_headers.validate",
    "check_fixtures.py --check", "generate_quality_report.py --check", "actionlint",
]


def _workflow_texts() -> dict[str, str]:
    files = sorted(WORKFLOWS.glob("*.yml")) + sorted((REPO_ROOT / ".github" / "actions").glob("*/action.yml"))
    return {p.relative_to(REPO_ROOT).as_posix(): p.read_text(encoding="utf-8") for p in files}


class WorkflowContracts(unittest.TestCase):
    def test_tool_pins_match_the_scripts(self) -> None:
        texts = "\n".join(_workflow_texts().values())
        self.assertEqual(set(re.findall(r"clang-format==([\d.]+)", texts)), {check_format.CLANG_FORMAT_VERSION})
        self.assertEqual(set(re.findall(r"clang-tidy==([\d.]+)", texts)), {check_tidy.CLANG_TIDY_VERSION})
        self.assertEqual(len(set(re.findall(r"actionlint_([\d.]+)_linux", texts))), 1)

    def test_each_action_is_used_at_one_major_version(self) -> None:
        majors: dict[str, set[str]] = {}
        for text in _workflow_texts().values():
            for action, version in re.findall(r"uses: ([\w./-]+?)@v(\d+)", text):
                majors.setdefault(action, set()).add(version)
        self.assertEqual({a: v for a, v in majors.items() if len(v) > 1}, {})

    def test_coverage_floor_is_the_release_minimum(self) -> None:
        policy = json.loads((REPO_ROOT / "scripts" / "quality" / "coverage_policy.json").read_text(encoding="utf-8"))
        self.assertGreaterEqual(policy["line_floor_percent"],
                                generate_quality_report.THRESHOLDS["coverage_line_combined"])

    def test_every_gate_runs_on_pull_requests_staging_and_main(self) -> None:
        for name in ("pull-request.yml", "staging.yml", "main.yml"):
            text = (WORKFLOWS / name).read_text(encoding="utf-8")
            with self.subTest(workflow=name):
                self.assertEqual([c for c in GATE_COMMANDS if c not in text], [])

    def test_release_requires_the_release_candidate(self) -> None:
        candidate = (WORKFLOWS / "release-candidate.yml").read_text(encoding="utf-8")
        for step in ("staging.yml --commit", "preflight.py", "--gates", "evidence_bundle.py verify"):
            self.assertIn(step, candidate)
        self.assertNotRegex(candidate, r"continue-on-error|inputs:\s*\n\s+skip")
        self.assertIn("release-candidate.yml --commit", (WORKFLOWS / "main.yml").read_text(encoding="utf-8"))

    def test_no_branch_shares_the_release_tag_name(self) -> None:
        # SignPath (and plain git) cannot resolve a ref that is both branch and tag; v2.4.0 failed on it
        text = (WORKFLOWS / "main.yml").read_text(encoding="utf-8")
        self.assertNotRegex(text, r'refs/heads/"?\$\{?TAG\}?"?')
        self.assertNotRegex(text, r'git checkout -b "?\$\{?TAG\}?"?')

    def test_release_notes_are_the_changelog_section(self) -> None:
        text = (WORKFLOWS / "main.yml").read_text(encoding="utf-8")
        self.assertNotIn("--generate-notes", text)
        extract = re.search(r"awk (-v v=\"## \[\$\{TAG#v\}\]\" '[^']+') CHANGELOG\.md", text)
        self.assertIsNotNone(extract)
        self.assertIn("--notes-file", text)
        changelog = "# Changelog\n\n## [2.4.0] - 2026-10-22\n\nNew.\n\n## [2.3.0] - 2026-04-28\n\nOld.\n"
        script = extract.group(1).replace("${TAG#v}", "2.4.0")
        notes = subprocess.run(["sh", "-c", f"awk {script}"], input=changelog, capture_output=True,
                               text=True, check=True).stdout
        self.assertEqual(notes.strip(), "New.")

    def test_unsigned_macos_is_one_explicit_switch(self) -> None:
        # G7: macOS ships unsigned only when the owner sets MACOS_SIGNING=unavailable, never on a missing secret
        switch = "vars.MACOS_SIGNING"
        for name in ("release-macos.yml", "release-installer.yml"):
            text = (WORKFLOWS / name).read_text(encoding="utf-8")
            imports = re.findall(r"- name: Import Apple Developer ID certificate\n\s+if: ([^\n]+)", text)
            self.assertTrue(imports, name)
            for condition in imports:
                self.assertIn(f"{switch} != 'unavailable'", condition, name)
        self.assertIn(f"{switch} == 'unavailable'", (WORKFLOWS / "release-candidate.yml").read_text(encoding="utf-8"))
        self.assertIn(f"unsigned_macos: ${{{{ {switch} == 'unavailable' }}}}",
                      (WORKFLOWS / "main.yml").read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
