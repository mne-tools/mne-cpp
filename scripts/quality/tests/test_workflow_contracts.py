# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""CI source-of-truth contracts.

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


_KEY_VALUE = re.compile(r"^(\s*)(- )?[\w.-]+:\s+(\S.*)$")


def _plain_scalars_with_colon(text: str) -> list[int]:
    """Lines whose unquoted value contains ': ', which YAML rejects ("mapping values are not allowed").

    A workflow that does not parse fails without running a single job, so this keeps the check that actionlint
    does in CI available to the bare interpreter of the lint jobs (no PyYAML).
    """
    bad: list[int] = []
    block_indent: int | None = None
    for number, line in enumerate(text.splitlines(), start=1):
        indent = len(line) - len(line.lstrip())
        if block_indent is not None:
            if not line.strip() or indent > block_indent:
                continue
            block_indent = None
        match = _KEY_VALUE.match(line)
        if match is None:
            continue
        value = match.group(3).split(" #", 1)[0].rstrip()
        if value[0] in "|>":
            block_indent = len(match.group(1)) + (2 if match.group(2) else 0)
        elif value[0] not in "'\"[{" and ": " in value:
            bad.append(number)
    return bad


class WorkflowContracts(unittest.TestCase):
    def test_workflows_have_no_unquoted_colon_values(self) -> None:
        for name, text in _workflow_texts().items():
            with self.subTest(workflow=name):
                self.assertEqual(_plain_scalars_with_colon(text), [])

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

    def test_cross_os_caches_hold_workspace_paths_only(self) -> None:
        # actions/cache stores a path outside the workspace by where it was on the saving runner
        # (/home/runner/...), so another OS restores it to the wrong place; a cross-OS cache must
        # name paths inside the workspace.
        offending = []
        for name, text in _workflow_texts().items():
            for step in re.split(r"\n\s*- (?=name:|uses:)", text):
                if "actions/cache@" not in step or not re.search(r"enableCrossOsArchive:\s*true", step):
                    continue
                block = re.search(r"\bpath:\s*(\|[^\n]*\n(?:\s+\S[^\n]*\n)*|[^\n]+)", step + "\n")
                paths = [p.strip() for p in (block.group(1) if block else "").lstrip("|").splitlines() if p.strip()]
                offending += [f"{name}: {p}" for p in paths
                              if p.startswith(("~", "/", "$HOME", "${{ runner")) or re.match(r"[A-Za-z]:", p) or ".." in p]
        self.assertEqual([], offending)

    def test_windows_links_are_not_made_with_mklink_from_bash(self) -> None:
        # Git Bash rewrites cmd's /J and /D switches into paths ("Invalid switch - C:\\Users\\...")
        offending = [f"{name}:{number}" for name, text in _workflow_texts().items()
                     for number, line in enumerate(text.splitlines(), 1)
                     if "mklink" in line and "cmd //c" in line]
        self.assertEqual([], offending)

    def test_coverage_floor_is_the_release_minimum(self) -> None:
        policy = json.loads((REPO_ROOT / "scripts" / "quality" / "coverage_policy.json").read_text(encoding="utf-8"))
        self.assertGreaterEqual(policy["line_floor_percent"],
                                generate_quality_report.THRESHOLDS["coverage_line_combined"])

    def test_every_gate_runs_on_pull_requests_staging_and_main(self) -> None:
        for name in ("pull-request.yml", "staging.yml", "main.yml"):
            text = (WORKFLOWS / name).read_text(encoding="utf-8")
            with self.subTest(workflow=name):
                self.assertEqual([c for c in GATE_COMMANDS if c not in text], [])

    def test_sanitizers_run_on_pull_requests_staging_and_main(self) -> None:
        sanitizers = (WORKFLOWS / "_reusable-sanitizers.yml").read_text(encoding="utf-8")
        for flag in ("thread", "address,undefined"):
            self.assertIn(flag, sanitizers)
        self.assertIn("scripts/ci/sanitizers", sanitizers)
        self.assertNotIn("continue-on-error", sanitizers)
        for name in ("pull-request.yml", "staging.yml", "main.yml"):
            with self.subTest(workflow=name):
                self.assertIn("uses: ./.github/workflows/_reusable-sanitizers.yml",
                              (WORKFLOWS / name).read_text(encoding="utf-8"))

    def test_qt_free_sdk_runs_on_pull_requests_staging_and_main(self) -> None:
        job = (WORKFLOWS / "_reusable-qt-free.yml").read_text(encoding="utf-8")
        self.assertIn("scripts/ci/qt_free_sdk.sh", job)
        self.assertNotIn("--kind qt", job)
        self.assertNotIn("continue-on-error", job)
        for name in ("pull-request.yml", "staging.yml", "main.yml"):
            with self.subTest(workflow=name):
                self.assertIn("uses: ./.github/workflows/_reusable-qt-free.yml",
                              (WORKFLOWS / name).read_text(encoding="utf-8"))

    def test_feature_branches_run_the_pull_request_checks_on_push(self) -> None:
        text = (WORKFLOWS / "pull-request.yml").read_text(encoding="utf-8")
        self.assertRegex(text, r"\n  push:\n    branches:\n      - 'feature/\*\*'\n")
        self.assertIn("\n  workflow_dispatch:\n", text)
        # Pushes run only in forks: upstream branches get their checks from the pull request.
        jobs = re.findall(r"\n  ([A-Za-z]+):\n((?:    .*\n|\n)*)", text.split("\njobs:\n", 1)[1])
        self.assertTrue(jobs)
        for name, body in jobs:
            with self.subTest(job=name):
                self.assertIn("if: ${{ github.event_name != 'push' || github.repository != 'mne-tools/mne-cpp' }}", body)
        # A push and the pull request of the same branch share one group, so they never both run.
        self.assertIn("group: pr-${{ github.event.pull_request.head.repo.full_name || github.repository }}"
                      "-${{ github.head_ref || github.ref_name }}", text)

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

    def test_linux_verification_installs_the_documented_system_libraries(self) -> None:
        # Qt links libEGL/libOpenGL from the system; the clean runner must have exactly what users are told to install
        verify = (WORKFLOWS / "_reusable-release-verify.yml").read_text(encoding="utf-8")
        download = (REPO_ROOT / "doc" / "website" / "src" / "pages" / "download.tsx").read_text(encoding="utf-8")
        packages = re.search(r"apt-get install -y --no-install-recommends ([a-z0-9. -]+)", verify)
        self.assertIsNotNone(packages)
        self.assertIn("libegl1", packages.group(1))
        self.assertIn(f"sudo apt install {packages.group(1).strip()}", download)
        self.assertIn("workflow_dispatch:", verify)

    def test_unsigned_platform_is_one_explicit_switch(self) -> None:
        # G7: a platform ships unsigned only when the owner sets <OS>_SIGNING=unavailable, never on a missing secret
        for os_name, step, files in (
                ("MACOS", "Import Apple Developer ID certificate", ("release-macos.yml", "release-installer.yml")),
                ("WINDOWS", "Authenticode-sign the [a-z]+", ("release-windows.yml", "release-installer.yml"))):
            switch = f"vars.{os_name}_SIGNING"
            for name in files:
                text = (WORKFLOWS / name).read_text(encoding="utf-8")
                steps = re.findall(rf"- name: {step}\n\s+if: ([^\n]+)", text)
                self.assertTrue(steps, name)
                for condition in steps:
                    self.assertIn(f"{switch} != 'unavailable'", condition, name)
            self.assertIn(f"{switch} == 'unavailable'",
                          (WORKFLOWS / "release-candidate.yml").read_text(encoding="utf-8"))
            self.assertIn(f"unsigned_{os_name.lower()}: ${{{{ {switch} == 'unavailable' }}}}",
                          (WORKFLOWS / "main.yml").read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
