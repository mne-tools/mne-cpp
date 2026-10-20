# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Tests for tools/release/evidence_bundle.py (v2.4.0 T10.2, T10.8d)."""

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "release"))

import evidence_bundle  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parents[3]


class EvidenceBundleTest(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.root = Path(self._tmp.name)
        self.onboarding = self.root / "onboarding-artifacts" / "onboarding-linux"
        self.onboarding.mkdir(parents=True)
        (self.onboarding / "onboarding-linux.json").write_text(json.dumps(
            {"os": "linux", "ok": False, "failed_step": "./init.sh", "total_seconds": 12.5,
             "checkout_bytes": 10, "platform": "Linux", "dependencies": {"QT_VERSION": "6.12.0"}, "steps": []}))
        # An old report: the session ended after init.bat, the build never ran, yet "ok" was written
        (self.onboarding / "onboarding-windows.json").write_text(json.dumps(
            {"os": "windows", "ok": True, "failed_step": "cmake --build", "total_seconds": 61.3,
             "checkout_bytes": 5, "platform": "Windows",
             "steps": [{"command": ".\\init.bat", "ok": True}, {"command": "cmake --build", "ok": False}]}))
        self.signing = self.root / "release-evidence.json"
        self.signing.write_text("[]\n")
        self.bundle = self.root / "bundle"

    def tearDown(self):
        self._tmp.cleanup()

    def build(self, **kwargs):
        args = dict(out=self.bundle, reports=REPO_ROOT / "doc" / "release" / "v2.4.0", onboarding=None,
                    signing=None, ci_run="https://ci/run/1", commit="abc", version="2.4.0")
        args.update(kwargs)
        return evidence_bundle.build(**args)

    def test_bundle_maps_every_gate_and_verifies_offline(self):
        index = self.build(onboarding=self.onboarding.parent, signing=self.signing)
        self.assertEqual(evidence_bundle.verify(self.bundle), [])
        self.assertEqual(sorted(index["gates"]), sorted([*evidence_bundle.REPORT_GATES, "G7_signing"]))
        self.assertEqual(index["gates"]["G7_signing"]["files"], ["signing/release-evidence.json"])
        self.assertEqual(index["gates"]["G8_onboarding"]["files"],
                         ["reports/onboarding-results.json", "onboarding/onboarding-linux.json",
                          "onboarding/onboarding-windows.json"])
        for gate in evidence_bundle.REPORT_GATES:
            for name in index["gates"][gate]["files"]:
                self.assertTrue((self.bundle / name).is_file(), name)

    def test_pinned_onboarding_keeps_one_history_entry_per_run(self):
        target = self.root / "onboarding-results.json"
        reports = sorted(self.onboarding.parent.rglob("onboarding-*.json"))
        evidence_bundle.pin_onboarding(target, reports, "run/1", "2026-10-01")
        pinned = evidence_bundle.pin_onboarding(target, reports, "run/2", "2026-10-08")
        evidence_bundle.pin_onboarding(target, reports, "run/2", "2026-10-08")
        pinned = json.loads(target.read_text())
        self.assertEqual(pinned["run"], "run/2")
        self.assertEqual(pinned["results"]["linux"],
                         {"ok": False, "failed_step": "./init.sh", "total_seconds": 12.5, "checkout_bytes": 10,
                          "platform": "Linux", "dependencies": {"QT_VERSION": "6.12.0"}})
        self.assertEqual(pinned["results"]["windows"]["ok"], False)
        self.assertEqual(pinned["results"]["windows"]["failed_step"], "cmake --build")
        self.assertEqual(pinned["history"], [{"run": "run/1", "date": "2026-10-01", "linux": 12.5, "windows": 61.3},
                                             {"run": "run/2", "date": "2026-10-08", "linux": 12.5, "windows": 61.3}])

    def test_tampering_is_detected(self):
        self.build()
        self.assertIn("pending", json.loads((self.bundle / "index.json").read_text())["gates"]["G7_signing"]["status"])
        (self.bundle / "reports" / "coverage-baseline.json").write_text("{}")
        (self.bundle / "reports" / "extra.txt").write_text("x")
        (self.bundle / "reports" / "parity-baseline.json").unlink()
        self.assertEqual(evidence_bundle.verify(self.bundle),
                         ["reports/coverage-baseline.json does not match its checksum",
                          "reports/parity-baseline.json is missing",
                          "reports/extra.txt is not listed in index.json"])
        (self.bundle / "index.json").unlink()
        self.assertEqual(evidence_bundle.verify(self.bundle), ["index.json is missing"])


if __name__ == "__main__":
    unittest.main()
