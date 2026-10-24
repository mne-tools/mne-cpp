#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Developer ID signing and notarization of the macOS release artifacts.

Every step runs in one of two modes:

``production``
    Real Apple credentials (the ``APPLE_*`` repository secrets, see
    ``doc/wiki/MacOS-Code-Signing.md``), a trusted *Developer ID Application*
    identity of ``APPLE_TEAM_ID``, ``xcrun notarytool``, ``stapler`` and
    ``spctl``. Any failure exits non-zero, so nothing unsigned is published.

``mock``
    The dummy values of ``scripts/release/fixtures/macos-signing-dummy.env`` and
    an ephemeral self-signed identity. Code is really signed with that identity,
    so ordering and verification are exercised, but Apple is never contacted and
    every result is labelled ``MOCK — NOT SIGNED OR NOTARIZED``. Mock mode
    refuses to run for a release tag and refuses real-looking credentials.

Commands::

    macos_signing.py check-credentials --mode production
    macos_signing.py select-identity --mode production --keychain KC
    macos_signing.py sign --mode production --identity ID [--keychain KC] PATH...
    macos_signing.py verify --mode production PATH...
    macos_signing.py notarize --mode production FILE [--evidence OUT.json]
    macos_signing.py staple --mode production PATH...
    macos_signing.py require-production-evidence EVIDENCE.json...
    macos_signing.py make-test-identity OUT_DIR
"""

from __future__ import annotations

import argparse
import base64
import binascii
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Callable, Sequence

MOCK_LABEL = "MOCK — NOT SIGNED OR NOTARIZED"
REQUIRED = ("APPLE_APPLICATION_CERT_BASE64", "APPLE_APPLICATION_CERT_PASSWORD", "APPLE_ID",
            "APPLE_ID_PASSWORD", "APPLE_TEAM_ID")
INSTALLER = ("APPLE_INSTALLER_CERT_BASE64", "APPLE_INSTALLER_CERT_PASSWORD")
OPTIONAL = INSTALLER + ("APPLE_KEYCHAIN_PASSWORD",)
DUMMY_MARKERS = ("TEST_ONLY", "TESTONLY", "example.invalid")
TEAM_ID = re.compile(r"^[A-Z0-9]{10}$")
MACH_O_MAGIC = {b"\xfe\xed\xfa\xce", b"\xce\xfa\xed\xfe", b"\xfe\xed\xfa\xcf", b"\xcf\xfa\xed\xfe",
                b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca"}
BUNDLES = (".app", ".framework", ".bundle", ".plugin", ".appex", ".xpc")
# Signed as a whole, without the hardened runtime, which applies to code only.
CONTAINERS = (".dmg", ".pkg")

Runner = Callable[[Sequence[str]], subprocess.CompletedProcess]


def run(command: Sequence[str]) -> subprocess.CompletedProcess:
    return subprocess.run(list(command), capture_output=True, text=True, check=False)


class SigningError(RuntimeError):
    pass


def _banner(mode: str, message: str) -> None:
    prefix = f"[{MOCK_LABEL}] " if mode == "mock" else ""
    print(prefix + message, flush=True)


def guard_mode(mode: str, environ: dict[str, str]) -> None:
    """Mock mode is a test of the pipeline, never a way to produce release artifacts."""
    if mode == "mock" and environ.get("GITHUB_REF", "").startswith("refs/tags/"):
        raise SigningError("mock signing is not allowed for a release tag")


def is_dummy(value: str) -> bool:
    return any(marker in value for marker in DUMMY_MARKERS)


def check_credentials(mode: str, environ: dict[str, str]) -> list[str]:
    """Every problem with the APPLE_* values for *mode*; empty when they are usable."""
    problems = [f"{name} is not set" for name in REQUIRED if not environ.get(name)]
    present = [name for name in INSTALLER if environ.get(name)]
    if len(present) == 1:
        problems.append(f"{present[0]} is set without its counterpart; set both or neither")
    supplied = {name: environ[name] for name in REQUIRED + OPTIONAL if environ.get(name)}
    if mode == "mock":
        real = [name for name, value in supplied.items() if not is_dummy(value)]
        problems += [f"{name} does not look like a dummy value; mock mode never handles real credentials"
                     for name in real]
        return problems
    problems += [f"{name} holds a TEST_ONLY placeholder" for name, value in supplied.items() if is_dummy(value)]
    team = environ.get("APPLE_TEAM_ID", "")
    if team and not is_dummy(team) and not TEAM_ID.match(team):
        problems.append("APPLE_TEAM_ID must be the 10-character Apple Team ID (A-Z, 0-9)")
    apple_id = environ.get("APPLE_ID", "")
    if apple_id and not is_dummy(apple_id) and not re.match(r"^[^@\s]+@[^@\s]+\.[^@\s]+$", apple_id):
        problems.append("APPLE_ID must be the e-mail address of the Apple account")
    for name in ("APPLE_APPLICATION_CERT_BASE64", "APPLE_INSTALLER_CERT_BASE64"):
        value = environ.get(name, "")
        if not value or is_dummy(value):
            continue
        try:
            der = base64.b64decode("".join(value.split()), validate=True)
        except (binascii.Error, ValueError):
            problems.append(f"{name} is not valid Base64")
            continue
        if not der.startswith(b"\x30"):
            problems.append(f"{name} does not decode to a PKCS#12 (.p12) file")
    return problems


def select_identity(mode: str, team: str, find_identity_output: str) -> tuple[str, str]:
    """(SHA-1, name) of the one Developer ID Application identity of *team* in ``security find-identity`` output."""
    pattern = re.compile(r'^\s*\d+\)\s+([0-9A-F]{40})\s+"(Developer ID Application: .*\(([A-Z0-9]+)\))"(.*)$')
    matches = []
    for line in find_identity_output.splitlines():
        hit = pattern.match(line)
        if hit and hit.group(3) == team:
            matches.append((hit.group(1), hit.group(2), hit.group(4).strip()))
    if mode == "production":
        matches = [m for m in matches if not m[2]]
    unique = {(sha, name) for sha, name, _status in matches}
    if not unique:
        kind = "valid (trusted)" if mode == "production" else ""
        raise SigningError(f"no {kind} 'Developer ID Application' identity for team {team} in the keychain".replace("  ", " "))
    if len(unique) > 1:
        raise SigningError(f"{len(unique)} Developer ID Application identities for team {team}; import exactly one")
    return unique.pop()


def is_mach_o(path: Path) -> bool:
    if not path.is_file() or path.is_symlink():
        return False
    with path.open("rb") as stream:
        return stream.read(4) in MACH_O_MAGIC


def signing_order(root: Path) -> list[Path]:
    """Nested code of *root*, deepest first, then *root* itself.

    Bundles (.app, .framework, ...) are signed as a whole after their content;
    loose Mach-O files are signed individually; files inside an already listed
    framework version are covered by signing the framework.
    """
    items: list[Path] = []
    if root.is_file():
        return [root] if is_mach_o(root) else []
    # os.walk does not follow the Versions/Current symlinks of frameworks.
    for directory, dirs, files in os.walk(root):
        here = Path(directory)
        items += [here / d for d in dirs if d.endswith(BUNDLES) and not (here / d).is_symlink()]
        for name in files:
            path = here / name
            # codesign's own temporary copy, left behind when a signing run is interrupted
            if name.endswith(".cstemp") or not is_mach_o(path):
                continue
            # The main binary of a bundle is signed with the bundle itself.
            bundle = next((p for p in path.parents if p.suffix in BUNDLES), None)
            if bundle is not None and name == bundle.stem and (
                    path.parent.name == "MacOS" or path.parent.parent.name == "Versions"):
                continue
            items.append(path)
    items.sort(key=lambda p: (-len(p.relative_to(root).parts), str(p)))
    if root.suffix in BUNDLES:
        items.append(root)
    return items


def sign(mode: str, identity: str, paths: Sequence[Path], keychain: str | None, runner: Runner = run,
         entitlements: Path | None = None) -> int:
    """Sign every path inside-out with the hardened runtime; return the number of signed items."""
    base = ["codesign", "--force", "--sign", identity]
    base += ["--timestamp"] if mode == "production" else ["--timestamp=none"]
    if keychain:
        base += ["--keychain", keychain]
    count = 0
    for root in paths:
        for item in [root] if root.suffix in CONTAINERS else signing_order(root):
            command = list(base) if item.suffix in CONTAINERS else base + ["--options", "runtime"]
            if entitlements and item.suffix == ".app":
                command += ["--entitlements", str(entitlements)]
            result = runner(command + [str(item)])
            if result.returncode != 0:
                raise SigningError(f"codesign failed for {item}: {result.stderr.strip()}")
            count += 1
    _banner(mode, f"signed {count} item(s) with the hardened runtime as '{identity}'")
    return count


def top_level_code(root: Path) -> list[Path]:
    """*root* if it is a bundle, disk image or Mach-O file, else the bundles and Mach-O files below it."""
    if root.suffix in BUNDLES + CONTAINERS or root.is_file():
        return [root] if root.suffix in BUNDLES + CONTAINERS or is_mach_o(root) else []
    found: list[Path] = []
    for child in sorted(root.iterdir()):
        if not child.is_symlink():
            found += top_level_code(child)
    return found


def verify(mode: str, paths: Sequence[Path], team: str, runner: Runner = run) -> None:
    """Strict signature check; in production also the Developer ID chain, Team ID and secure timestamp."""
    targets = [item for root in paths for item in top_level_code(root)]
    if not targets:
        raise SigningError(f"no code to verify in {', '.join(map(str, paths))}")
    for path in targets:
        result = runner(["codesign", "--verify", "--strict", "--deep", "--verbose=2", str(path)])
        if result.returncode != 0:
            raise SigningError(f"{path}: signature does not verify: {result.stderr.strip()}")
        details = runner(["codesign", "--display", "--verbose=4", str(path)])
        text = details.stdout + details.stderr
        if path.suffix not in CONTAINERS and "(runtime)" not in text:
            raise SigningError(f"{path}: not signed with the hardened runtime")
        if mode == "production":
            chain = ("Authority=Developer ID Application:", "Authority=Developer ID Certification Authority",
                     "Authority=Apple Root CA")
            if not all(link in text for link in chain):
                raise SigningError(f"{path}: not signed with a Developer ID Application certificate issued by Apple")
            if f"TeamIdentifier={team}" not in text:
                raise SigningError(f"{path}: Team ID differs from APPLE_TEAM_ID")
            if "Timestamp=" not in text:
                raise SigningError(f"{path}: signature has no secure timestamp")
    _banner(mode, f"{len(targets)} signature(s) verified")


def notarize(mode: str, artifact: Path, environ: dict[str, str], runner: Runner = run) -> dict:
    """Submit *artifact* and wait; raise unless Apple returns Accepted. Returns non-sensitive evidence."""
    if mode == "mock":
        status = environ.get("MACOS_MOCK_NOTARY_RESULT", "Accepted")
        _banner(mode, f"notarytool not called; simulated status for {artifact.name}: {status}")
        if status != "Accepted":
            _banner(mode, "simulated notarization log: the binary is not signed with a valid Developer ID certificate")
            raise SigningError(f"{artifact.name}: notarization {status} (simulated)")
        return {"artifact": artifact.name, "mode": "mock", "status": MOCK_LABEL}
    auth = ["--apple-id", environ["APPLE_ID"], "--password", environ["APPLE_ID_PASSWORD"],
            "--team-id", environ["APPLE_TEAM_ID"]]
    result = runner(["xcrun", "notarytool", "submit", str(artifact), *auth, "--wait", "--timeout", "2h",
                     "--output-format", "json"])
    try:
        reply = json.loads(result.stdout or "{}")
    except json.JSONDecodeError:
        reply = {}
    submission, status = reply.get("id", ""), reply.get("status", "")
    print(f"{artifact.name}: notarization submission {submission or '(none)'}: {status or 'no status'}")
    if status != "Accepted":
        if submission:
            log = runner(["xcrun", "notarytool", "log", submission, *auth])
            print(log.stdout or log.stderr)
        raise SigningError(f"{artifact.name}: notarization {status or 'failed'}: {result.stderr.strip()}")
    return {"artifact": artifact.name, "mode": "production", "submission_id": submission, "status": "Accepted",
            "team_id": environ["APPLE_TEAM_ID"]}


def staple(mode: str, paths: Sequence[Path], runner: Runner = run) -> None:
    """Staple and validate the ticket, then ask Gatekeeper with the assessment type of each artifact."""
    for path in paths:
        if mode == "mock":
            _banner(mode, f"{path.name}: stapler and spctl not called")
            continue
        for command in (["xcrun", "stapler", "staple", str(path)], ["xcrun", "stapler", "validate", str(path)]):
            result = runner(command)
            if result.returncode != 0:
                raise SigningError(f"{' '.join(command[:3])} failed for {path}: {result.stdout}{result.stderr}")
        assess = {".dmg": ["--type", "open", "--context", "context:primary-signature"],
                  ".pkg": ["--type", "install"]}.get(path.suffix, ["--type", "execute"])
        result = runner(["spctl", "--assess", *assess, "--verbose=2", str(path)])
        if result.returncode != 0 or "accepted" not in (result.stdout + result.stderr):
            raise SigningError(f"Gatekeeper rejects {path}: {result.stdout}{result.stderr}")
        print(f"{path.name}: stapled, validated and accepted by Gatekeeper")


def require_production_evidence(files: Sequence[Path]) -> list[str]:
    """Problems that must stop a release upload: mock, missing or unaccepted notarization evidence."""
    problems = []
    for file in files:
        if not file.is_file():
            problems.append(f"{file}: no notarization evidence")
            continue
        for record in json.loads(file.read_text(encoding="utf-8")):
            if record.get("mode") != "production" or record.get("status") != "Accepted":
                problems.append(f"{file}: {record.get('artifact')} is {record.get('status', 'unknown')}")
    return problems


def make_test_identity(out: Path, team: str = "TESTONLY00") -> Path:
    """Ephemeral self-signed code-signing identity as .p12 (password TEST_ONLY_PASSWORD) for mock runs."""
    out.mkdir(parents=True, exist_ok=True)
    config = out / "cert.cnf"
    config.write_text(
        "[req]\ndistinguished_name = dn\nprompt = no\nx509_extensions = ext\n"
        f"[dn]\nCN = Developer ID Application: MNE-CPP TEST ONLY ({team})\nOU = {team}\nO = MNE-CPP TEST ONLY\n"
        "[ext]\nbasicConstraints = critical,CA:false\nkeyUsage = critical,digitalSignature\n"
        "extendedKeyUsage = critical,codeSigning\n", encoding="utf-8")
    key, cert, p12 = out / "key.pem", out / "cert.pem", out / "test-identity.p12"
    subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-keyout", str(key), "-out",
                    str(cert), "-days", "1", "-config", str(config)], check=True, capture_output=True)
    export = ["openssl", "pkcs12", "-export", "-inkey", str(key), "-in", str(cert), "-out", str(p12),
              "-passout", "pass:TEST_ONLY_PASSWORD"]
    # macOS `security import` reads only the legacy PKCS#12 ciphers; LibreSSL has no -legacy flag.
    if subprocess.run([*export, "-legacy"], capture_output=True).returncode != 0:
        subprocess.run(export, check=True, capture_output=True)
    key.unlink()
    return p12


def _write_evidence(path: Path | None, record: dict) -> None:
    if path is None:
        return
    records = json.loads(path.read_text(encoding="utf-8")) if path.is_file() else []
    records.append(record)
    path.write_text(json.dumps(records, indent=2) + "\n", encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    for name in ("check-credentials", "select-identity", "sign", "verify", "notarize", "staple"):
        sub = commands.add_parser(name)
        sub.add_argument("--mode", choices=("production", "mock"), required=True)
        if name == "select-identity":
            sub.add_argument("--keychain", required=True)
        if name == "sign":
            sub.add_argument("--identity", required=True)
            sub.add_argument("--keychain")
            sub.add_argument("--entitlements", type=Path)
        if name == "notarize":
            sub.add_argument("--evidence", type=Path)
        if name in {"sign", "verify", "staple", "notarize"}:
            sub.add_argument("paths", nargs="+", type=Path)
    gate = commands.add_parser("require-production-evidence")
    gate.add_argument("paths", nargs="+", type=Path)
    test_identity = commands.add_parser("make-test-identity")
    test_identity.add_argument("out", type=Path)
    args = parser.parse_args(argv)
    environ = dict(os.environ)
    try:
        if args.command == "make-test-identity":
            print(make_test_identity(args.out))
            return 0
        if args.command == "require-production-evidence":
            problems = require_production_evidence(args.paths)
            for problem in problems:
                print(f"::error::{problem}")
            return 1 if problems else 0
        guard_mode(args.mode, environ)
        if args.command == "check-credentials":
            problems = check_credentials(args.mode, environ)
            for problem in problems:
                print(f"::error::{problem}")
            if not problems:
                _banner(args.mode, "Apple signing credentials are complete for " + args.mode)
            return 1 if problems else 0
        if args.command == "select-identity":
            listing = run(["security", "find-identity", "-p", "codesigning"]
                          + (["-v"] if args.mode == "production" else []) + [args.keychain])
            sha, name = select_identity(args.mode, environ.get("APPLE_TEAM_ID", ""), listing.stdout)
            print(f"{sha}\t{name}")
            return 0
        if args.command == "sign":
            sign(args.mode, args.identity, args.paths, args.keychain, entitlements=args.entitlements)
            return 0
        if args.command == "verify":
            verify(args.mode, args.paths, environ.get("APPLE_TEAM_ID", ""))
            return 0
        if args.command == "notarize":
            for path in args.paths:
                _write_evidence(args.evidence, notarize(args.mode, path, environ))
            return 0
        if args.command == "staple":
            staple(args.mode, args.paths)
            return 0
    except (SigningError, KeyError) as error:
        print(f"::error::{error}")
        return 1
    return 2


if __name__ == "__main__":
    sys.exit(main())
