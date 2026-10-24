# macOS Code Signing

## How it works

| Mode | When | Credentials | Apple contacted | Result |
|---|---|---|---|---|
| **A — Development** | pull requests, `staging`, `main` | none | no | apps ad-hoc signed (required on Apple silicon), labelled development builds |
| **B — Pipeline test** | [`macos-signing-test.yml`](../../.github/workflows/macos-signing-test.yml) (manual, or on changes to the signing code) | [`macos-signing-dummy.env`](../../scripts/release/fixtures/macos-signing-dummy.env) + ephemeral self-signed identity | no | `MOCK — NOT SIGNED OR NOTARIZED`; cannot publish |
| **C — Production** | release tags `v*` | the `APPLE_*` secrets | yes | Developer ID signed, notarized, stapled, verified, then published |

Production (`release-macos.yml`, `release-installer.yml`):

1. `macos_signing.py check-credentials --mode production` rejects missing, malformed or
   `TEST_ONLY` values before anything is built into a release.
2. [`macos-codesign-setup`](../../.github/actions/macos-codesign-setup/action.yml) imports
   the `.p12` into a temporary keychain and selects the one **trusted**
   *Developer ID Application* identity whose Team ID equals `APPLE_TEAM_ID`.
3. [`deploy.sh`](../../scripts/deploy/deploy.sh) deploys Qt and the MNE-CPP libraries into
   the bundles, then
   [`macos_release_signing.sh tree`](../../scripts/deploy/macos_release_signing.sh) signs every
   Mach-O file, framework and app in `bin/` and `lib/` inside-out (no `--deep`) with the
   hardened runtime and a secure timestamp, verifies the Apple chain, Team ID and
   timestamp of each, notarizes them with `xcrun notarytool submit --wait`, and staples
   the apps. The tarball, the `.dmg` and the installer payload therefore contain the
   same notarized code.
4. The `.dmg` is signed, notarized, stapled and assessed with `spctl --type open`.
5. The installer job verifies that its payload carries the same team's signatures and
   stapled tickets, signs the QtIFW installer app after `binarycreator`, notarizes and
   staples it, then does the same for its `.dmg`.
6. `require-production-evidence` lets the upload run only if Apple returned `Accepted`
   for every submission. A rejection prints the notarization log and fails the job.

Entitlements: none. With one Developer ID, all frameworks and plugins share the Team ID,
so the hardened runtime's library validation accepts them; a run with library
validation relaxed showed MNE Scan loading every plugin. App data lives in
`Contents/Resources`, so its signatures are stored in the bundle seal and survive
archiving.

The static macOS archives are not Developer ID signed; the release notes say so.
`MACOS_SIGNING=unavailable` is the only way to release without Apple signing, and the
release verification then requires the release notes to record it.

## Repository settings checklist

- [ ] Repository secrets `APPLE_APPLICATION_CERT_BASE64`, `APPLE_APPLICATION_CERT_PASSWORD`, `APPLE_ID`,
      `APPLE_ID_PASSWORD` (app-specific password) and `APPLE_TEAM_ID` set; `APPLE_INSTALLER_CERT_BASE64`,
      `APPLE_INSTALLER_CERT_PASSWORD` (only for a flat `.pkg`) and `APPLE_KEYCHAIN_PASSWORD` unset.
- [ ] Repository variable `MACOS_SIGNING` deleted.
- [ ] Workflow permissions stay read-only by default; secrets are never passed to
      pull-request workflows, and `macos-signing-test.yml` reads no secret.
- [ ] Environment `release` keeps its required reviewer; it gates the Release Candidate approval.
