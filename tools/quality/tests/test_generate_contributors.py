# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Tests for tools/generate_contributors.py."""

import importlib.util
import subprocess
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
_spec = importlib.util.spec_from_file_location("generate_contributors", REPO_ROOT / "tools" / "generate_contributors.py")
generate = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(generate)


class ContributorsTest(unittest.TestCase):
    def test_identities_are_merged_ranked_and_bots_dropped(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)

            def commit(name: str, email: str) -> None:
                subprocess.run(["git", "-c", f"user.name={name}", "-c", f"user.email={email}", "commit", "-q",
                                "--allow-empty", "-m", "x"], cwd=repo, check=True)

            subprocess.run(["git", "init", "-q"], cwd=repo, check=True)
            (repo / ".mailmap").write_text("Lorenz Esch <lorenzesch@hotmail.com> <lorenz.esch@tu-ilmenau.de>\n"
                                           "Lorenz Esch <lorenzesch@hotmail.com>\n")
            for _ in range(2):
                commit("Christoph Dinh", "christoph.dinh@mne-cpp.org")
            commit("LorenzE", "lorenzesch@hotmail.com")
            commit("lorenz_e", "lorenz.esch@tu-ilmenau.de")
            commit("Lorenz Esch", "lorenz.esch@tu-ilmenau.de")
            commit("dependabot[bot]", "bot@users.noreply.github.com")
            commit("Someone New", "new@example.org")
            self.assertEqual(generate.contributors(repo), [
                {"name": "Lorenz Esch", "login": "LorenzE", "contributions": 3},
                {"name": "Christoph Dinh", "login": "chdinh", "contributions": 2},
                {"name": "Someone New", "login": None, "contributions": 1},
            ])

    def test_every_github_login_belongs_to_a_mailmap_name(self) -> None:
        mailmap = (REPO_ROOT / ".mailmap").read_text(encoding="utf-8")
        canonical = {line.split(" <", 1)[0] for line in mailmap.splitlines() if line and not line.startswith("#")}
        # Names that need no .mailmap entry because they never committed under another identity
        single_identity = {"Jana Kiesel", "Andrey Parfenov", "Tim Kunze", "Johannes Behrens", "buildqa",
                           "Blerta Hamzallari", "Eric Larson", "Benjamin Kay", "Mainak Jas", "Martin Luessi"}
        self.assertEqual(set(generate.GITHUB_LOGINS) - canonical - single_identity, set())


if __name__ == "__main__":
    unittest.main()
