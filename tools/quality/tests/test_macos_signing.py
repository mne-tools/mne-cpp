# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Tests for tools/release/macos_signing.py and the macOS signing workflow wiring."""

import json
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
WORKFLOWS = REPO_ROOT / ".github" / "workflows"
DUMMY_ENV = REPO_ROOT / "tools" / "release" / "fixtures" / "macos-signing-dummy.env"
sys.path.insert(0, str(REPO_ROOT / "tools" / "release"))

import macos_signing as ms  # noqa: E402

MACH_O = b"\xcf\xfa\xed\xfe" + b"\0" * 28
NAMES = ("APPLE_APPLICATION_CERT_BASE64", "APPLE_APPLICATION_CERT_PASSWORD", "APPLE_ID", "APPLE_ID_PASSWORD",
         "APPLE_INSTALLER_CERT_BASE64", "APPLE_INSTALLER_CERT_PASSWORD", "APPLE_KEYCHAIN_PASSWORD", "APPLE_TEAM_ID")


def dummy() -> dict[str, str]:
    lines = [line for line in DUMMY_ENV.read_text().splitlines() if line and not line.startswith("#")]
    return dict(line.split("=", 1) for line in lines)


def real() -> dict[str, str]:
    p12 = "MIIK" + "A" * 60
    return {"APPLE_APPLICATION_CERT_BASE64": p12, "APPLE_APPLICATION_CERT_PASSWORD": "secret",
            "APPLE_ID": "signer@example.org", "APPLE_ID_PASSWORD": "abcd-efgh-ijkl-mnop", "APPLE_TEAM_ID": "AB12CD34EF"}


class Done:
    def __init__(self, returncode=0, stdout="", stderr=""):
        self.returncode, self.stdout, self.stderr = returncode, stdout, stderr


class Recorder:
    """Stand-in for subprocess: records commands and answers from a table of (prefix, reply)."""

    def __init__(self, replies=()):
        self.commands, self.replies = [], list(replies)

    def __call__(self, command):
        self.commands.append(list(command))
        for prefix, reply in self.replies:
            if command[:len(prefix)] == prefix:
                return reply
        return Done()


class Credentials(unittest.TestCase):
    def test_fixture_has_exactly_the_eight_secret_names(self):
        self.assertEqual(set(dummy()), set(NAMES))
        self.assertEqual(set(ms.REQUIRED + ms.OPTIONAL), set(NAMES))

    def test_dummy_values_pass_mock_and_fail_production(self):
        self.assertEqual(ms.check_credentials("mock", dummy()), [])
        problems = ms.check_credentials("production", dummy())
        self.assertEqual(len(problems), 8)
        self.assertTrue(all("TEST_ONLY placeholder" in p for p in problems))

    def test_five_values_suffice_for_production(self):
        self.assertEqual(ms.check_credentials("production", real()), [])

    def test_missing_and_malformed_values_are_named(self):
        env = real()
        del env["APPLE_ID_PASSWORD"]
        env["APPLE_TEAM_ID"] = "short"
        env["APPLE_APPLICATION_CERT_BASE64"] = "not base64!"
        env["APPLE_INSTALLER_CERT_BASE64"] = "MIIK" + "A" * 60
        problems = ms.check_credentials("production", env)
        self.assertIn("APPLE_ID_PASSWORD is not set", problems)
        self.assertTrue(any("APPLE_TEAM_ID must be" in p for p in problems))
        self.assertIn("APPLE_APPLICATION_CERT_BASE64 is not valid Base64", problems)
        self.assertTrue(any("APPLE_INSTALLER_CERT_BASE64 is set without" in p for p in problems))

    def test_mock_mode_refuses_real_credentials_and_release_tags(self):
        self.assertTrue(any("does not look like a dummy" in p for p in ms.check_credentials("mock", real())))
        with self.assertRaises(ms.SigningError):
            ms.guard_mode("mock", {"GITHUB_REF": "refs/tags/v2.4.0"})
        ms.guard_mode("production", {"GITHUB_REF": "refs/tags/v2.4.0"})


class Identity(unittest.TestCase):
    LISTING = """
  1) 1111111111111111111111111111111111111111 "Developer ID Application: Someone Else (ZZZZZZZZZZ)"
  2) 2222222222222222222222222222222222222222 "Developer ID Application: Signer (AB12CD34EF)"
  3) 3333333333333333333333333333333333333333 "Developer ID Installer: Signer (AB12CD34EF)"
  4) 4444444444444444444444444444444444444444 "Developer ID Application: MNE-CPP TEST ONLY (TESTONLY00)" (CSSMERR_TP_NOT_TRUSTED)
"""

    def test_selects_the_application_identity_of_the_team(self):
        self.assertEqual(ms.select_identity("production", "AB12CD34EF", self.LISTING),
                         ("2" * 40, "Developer ID Application: Signer (AB12CD34EF)"))

    def test_untrusted_test_identity_only_in_mock_mode(self):
        with self.assertRaisesRegex(ms.SigningError, "valid"):
            ms.select_identity("production", "TESTONLY00", self.LISTING)
        self.assertEqual(ms.select_identity("mock", "TESTONLY00", self.LISTING)[0], "4" * 40)

    def test_two_identities_of_one_team_are_ambiguous(self):
        listing = self.LISTING + '  5) 5555555555555555555555555555555555555555 "Developer ID Application: Signer (AB12CD34EF)"\n'
        with self.assertRaisesRegex(ms.SigningError, "exactly one"):
            ms.select_identity("production", "AB12CD34EF", listing)


class Signing(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        root = Path(self._tmp.name)
        app = root / "bin" / "mne_scan.app" / "Contents"
        for rel in ("MacOS/mne_scan", "MacOS/mne_scan_plugins/scan_fiffsimulator.dylib",
                    "Frameworks/libmne_fiff.dylib", "Frameworks/QtCore.framework/Versions/A/QtCore",
                    "Frameworks/QtCore.framework/Versions/A/Helpers/helper", "PlugIns/platforms/libqcocoa.dylib"):
            (app / rel).parent.mkdir(parents=True, exist_ok=True)
            (app / rel).write_bytes(MACH_O)
        (app / "Frameworks/QtCore.framework/Versions/Current").symlink_to("A")
        (app / "Resources").mkdir()
        (app / "Resources/layout.xml").write_text("<data/>")
        (root / "bin" / "mne_show_fiff").write_bytes(MACH_O)
        self.root = root

    def tearDown(self):
        self._tmp.cleanup()

    def rel(self, paths):
        return [p.relative_to(self.root).as_posix() for p in paths]

    def test_nested_code_is_signed_before_its_container(self):
        order = self.rel(ms.signing_order(self.root / "bin" / "mne_scan.app"))
        app = "bin/mne_scan.app"
        self.assertEqual(order[-1], app)
        for inner, outer in ((f"{app}/Contents/Frameworks/QtCore.framework/Versions/A/Helpers/helper",
                              f"{app}/Contents/Frameworks/QtCore.framework"),
                             (f"{app}/Contents/MacOS/mne_scan_plugins/scan_fiffsimulator.dylib", app)):
            self.assertLess(order.index(inner), order.index(outer))
        self.assertNotIn(f"{app}/Contents/MacOS/mne_scan", order)
        self.assertNotIn(f"{app}/Contents/Frameworks/QtCore.framework/Versions/A/QtCore", order)
        self.assertFalse(any("Resources" in p or "Current" in p for p in order))

    def test_production_uses_hardened_runtime_and_secure_timestamps(self):
        runner = Recorder()
        count = ms.sign("production", "ID", [self.root / "bin"], "kc", runner)
        self.assertEqual(count, len(runner.commands))
        for command in runner.commands:
            self.assertIn("--timestamp", command)
            self.assertIn("runtime", command)
            self.assertNotIn("--deep", command)
        self.assertIn(str(self.root / "bin" / "mne_show_fiff"), [c[-1] for c in runner.commands])

    def test_disk_image_is_signed_without_runtime_and_mock_without_timestamp(self):
        runner = Recorder()
        ms.sign("mock", "ID", [self.root / "x.dmg"], None, runner)
        self.assertEqual(runner.commands, [["codesign", "--force", "--sign", "ID", "--timestamp=none",
                                            str(self.root / "x.dmg")]])

    def test_codesign_failure_stops_signing(self):
        runner = Recorder([(["codesign"], Done(1, stderr="errSecInternalComponent"))])
        with self.assertRaisesRegex(ms.SigningError, "errSecInternalComponent"):
            ms.sign("production", "ID", [self.root / "bin"], None, runner)
        self.assertEqual(len(runner.commands), 1)

    def test_production_verification_requires_apple_chain_team_and_timestamp(self):
        good = ("flags=0x10000(runtime)\nAuthority=Developer ID Application: Signer (AB12CD34EF)\n"
                "Authority=Developer ID Certification Authority\nAuthority=Apple Root CA\n"
                "Timestamp=Oct 23, 2026\nTeamIdentifier=AB12CD34EF\n")
        ms.verify("production", [self.root / "bin"], "AB12CD34EF",
                  Recorder([(["codesign", "--display"], Done(stderr=good))]))
        for broken, message in ((good.replace("Apple Root CA", "Self"), "issued by Apple"),
                                (good.replace("TeamIdentifier=AB12CD34EF", "TeamIdentifier=not set"), "Team ID"),
                                (good.replace("Timestamp=Oct 23, 2026", ""), "timestamp"),
                                (good.replace("(runtime)", ""), "hardened runtime")):
            with self.subTest(message=message), self.assertRaisesRegex(ms.SigningError, message):
                ms.verify("production", [self.root / "bin"], "AB12CD34EF",
                          Recorder([(["codesign", "--display"], Done(stderr=broken))]))


class Notarization(unittest.TestCase):
    def test_accepted_submission_yields_production_evidence(self):
        reply = Done(stdout=json.dumps({"id": "abc-123", "status": "Accepted"}))
        runner = Recorder([(["xcrun", "notarytool", "submit"], reply)])
        evidence = ms.notarize("production", Path("x.dmg"), real(), runner)
        self.assertEqual(evidence, {"artifact": "x.dmg", "mode": "production", "submission_id": "abc-123",
                                    "status": "Accepted", "team_id": "AB12CD34EF"})
        command = runner.commands[0]
        self.assertEqual(command[:3], ["xcrun", "notarytool", "submit"])
        self.assertIn("--wait", command)
        self.assertNotIn("altool", " ".join(command))

    def test_rejected_submission_fetches_the_log_and_fails(self):
        runner = Recorder([(["xcrun", "notarytool", "submit"], Done(stdout=json.dumps({"id": "abc", "status": "Invalid"}))),
                           (["xcrun", "notarytool", "log"], Done(stdout='{"issues": ["not signed"]}'))])
        with self.assertRaisesRegex(ms.SigningError, "Invalid"):
            ms.notarize("production", Path("x.dmg"), real(), runner)
        self.assertEqual(runner.commands[1][:4], ["xcrun", "notarytool", "log", "abc"])

    def test_mock_never_calls_apple_and_never_claims_acceptance(self):
        runner = Recorder()
        evidence = ms.notarize("mock", Path("x.dmg"), dummy(), runner)
        self.assertEqual(runner.commands, [])
        self.assertEqual(evidence["status"], ms.MOCK_LABEL)
        with self.assertRaisesRegex(ms.SigningError, "simulated"):
            ms.notarize("mock", Path("x.dmg"), {"MACOS_MOCK_NOTARY_RESULT": "Rejected"}, runner)
        ms.staple("mock", [Path("x.dmg")], runner)
        self.assertEqual(runner.commands, [])

    def test_staple_uses_the_assessment_type_of_each_artifact(self):
        runner = Recorder([(["spctl"], Done(stderr="x: accepted\nsource=Notarized Developer ID"))])
        ms.staple("production", [Path("a.dmg"), Path("b.app"), Path("c.pkg")], runner)
        assessments = [c for c in runner.commands if c[0] == "spctl"]
        self.assertEqual([c[3] for c in assessments], ["open", "execute", "install"])
        with self.assertRaisesRegex(ms.SigningError, "Gatekeeper"):
            ms.staple("production", [Path("a.app")], Recorder([(["spctl"], Done(3, stderr="rejected"))]))

    def test_publish_gate_accepts_only_accepted_production_evidence(self):
        with tempfile.TemporaryDirectory() as tmp:
            ok, mock = Path(tmp) / "ok.json", Path(tmp) / "mock.json"
            ok.write_text(json.dumps([{"artifact": "a", "mode": "production", "status": "Accepted"}]))
            mock.write_text(json.dumps([{"artifact": "a", "mode": "mock", "status": ms.MOCK_LABEL}]))
            self.assertEqual(ms.require_production_evidence([ok]), [])
            self.assertEqual(len(ms.require_production_evidence([mock, Path(tmp) / "missing.json"])), 2)


@unittest.skipUnless(sys.platform == "darwin", "needs macOS codesign")
class RealCodesign(unittest.TestCase):
    """Sign a copy of a system binary with an ephemeral self-signed identity: the real ordering and checks."""

    def test_mock_identity_signs_and_production_verification_rejects_it(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            p12 = ms.make_test_identity(tmp / "id")
            keychain = tmp / "t.keychain-db"
            for command in (["security", "create-keychain", "-p", "kp", str(keychain)],
                            ["security", "unlock-keychain", "-p", "kp", str(keychain)],
                            ["security", "import", str(p12), "-k", str(keychain), "-P", "TEST_ONLY_PASSWORD",
                             "-T", "/usr/bin/codesign"],
                            ["security", "set-key-partition-list", "-S", "apple-tool:,apple:,codesign:", "-s",
                             "-k", "kp", str(keychain)]):
                subprocess.run(command, check=True, capture_output=True)
            try:
                listing = subprocess.run(["security", "find-identity", "-p", "codesigning", str(keychain)],
                                         capture_output=True, text=True, check=True).stdout
                sha, _name = ms.select_identity("mock", "TESTONLY00", listing)
                app = tmp / "Tool.app" / "Contents" / "MacOS"
                app.mkdir(parents=True)
                (app.parent / "Info.plist").write_text(
                    '<?xml version="1.0"?><plist version="1.0"><dict><key>CFBundleExecutable</key>'
                    "<string>Tool</string><key>CFBundleIdentifier</key><string>org.mnecpp.test</string></dict></plist>")
                (app / "Tool").write_bytes(Path("/usr/bin/true").read_bytes())
                (app / "Tool").chmod(0o755)
                ms.sign("mock", sha, [tmp / "Tool.app"], str(keychain))
                ms.verify("mock", [tmp / "Tool.app"], "TESTONLY00")
                with self.assertRaisesRegex(ms.SigningError, "issued by Apple"):
                    ms.verify("production", [tmp / "Tool.app"], "TESTONLY00")
            finally:
                subprocess.run(["security", "delete-keychain", str(keychain)], capture_output=True)


class Workflows(unittest.TestCase):
    def text(self, name):
        return (WORKFLOWS / name).read_text(encoding="utf-8")

    def test_only_the_eight_secret_names_are_used(self):
        used = set()
        for path in list(WORKFLOWS.glob("*.yml")) + list((REPO_ROOT / ".github" / "actions").glob("*/action.yml")):
            used |= set(re.findall(r"secrets\.(APPLE_\w+)", path.read_text(encoding="utf-8")))
        self.assertTrue(used)
        self.assertLessEqual(used, set(NAMES))

    def test_release_jobs_check_credentials_and_gate_the_upload(self):
        macos = self.text("release-macos.yml")
        dynamic = macos[:macos.index("MacOSStatic:")]
        order = ["check-credentials --mode production", "macos-codesign-setup", "deploy.sh dynamic pack",
                 "require-production-evidence", "gh release upload"]
        self.assertEqual(order, sorted(order, key=dynamic.index))
        self.assertIn("mode: production", dynamic)
        installer = self.text("release-installer.yml")
        mac = installer[installer.index("MacOSInstaller:"):installer.index("WindowsInstaller:")]
        order = ["check-credentials --mode production", "Installer payload is signed", "binarycreator \\",
                 "Codesign installer app", "Notarize and staple installer app", "Create DMG",
                 "require-production-evidence", "gh release upload"]
        self.assertEqual(order, sorted(order, key=mac.index))
        self.assertNotIn("--deep --options runtime", mac)
        self.assertNotIn("APPLE_INSTALLER_CERT_BASE64 }}\n        certificate", mac)

    def test_pipeline_test_is_mock_only_and_cannot_publish(self):
        text = self.text("macos-signing-test.yml")
        self.assertNotRegex(text, r"secrets\.APPLE_")
        self.assertNotIn("gh release upload", text)
        self.assertIn("contents: read", text)
        self.assertIn("mode: mock", text)
        self.assertNotIn("mode: production", text)
        self.assertNotIn("tags:", text)


if __name__ == "__main__":
    unittest.main()
