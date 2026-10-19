# Release process

Branch model: **feature → `staging` → `main`**. Releases are cut from `main`
via an annotated `vX.Y.Z` tag, which triggers
[`main.yml`](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/main.yml)
to build platform binaries, installers, the SDK, the WASM bundle, and a
GitHub Release.

## Pre-release checklist

1. All tasks in `doc/dev-notes/vX.Y.Z-requirements.md` are DONE or
   explicitly deferred.
2. `staging.yml` is green on the latest `staging` commit (all 22 jobs).
3. Versioned files are bumped to `X.Y.Z` in one commit:
   - [`src/CMakeLists.txt`](https://github.com/mne-tools/mne-cpp/blob/main/src/CMakeLists.txt) — `MNE_CPP_VERSION_MAJOR/MINOR/PATCH`
   - [`doc/Doxyfile`](https://github.com/mne-tools/mne-cpp/blob/main/doc/Doxyfile) — `PROJECT_NUMBER`
   - [`CITATION.cff`](https://github.com/mne-tools/mne-cpp/blob/main/CITATION.cff) — `version:` and `date-released:`
   - [`README.md`](https://github.com/mne-tools/mne-cpp/blob/main/README.md) — version badge
   - [`CHANGELOG.md`](https://github.com/mne-tools/mne-cpp/blob/main/CHANGELOG.md) — new section with highlights
   - [`doc/website/docusaurus.config.ts`](https://github.com/mne-tools/mne-cpp/blob/main/doc/website/docusaurus.config.ts) — site version
4. Doxygen passes with **zero warnings** for new/changed code.
5. Website builds without broken links.

## Cut the release

```bash
# 1. Fast-forward main to staging
git checkout main
git pull --ff-only origin main
git merge --ff-only staging
git push origin main

# 2. Tag (annotated)
git tag -a vX.Y.Z -m "Release vX.Y.Z — <codename>"
git push origin vX.Y.Z
```

## What CI does on a `v*` tag (automatic)

| Workflow file | Output |
|---------------|--------|
| [`main.yml`](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/main.yml) | Orchestrator — calls the workflows below and creates the GitHub Release |
| [`release-linux.yml`](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/release-linux.yml) | Linux dynamic + static `.tar.gz` |
| [`release-macos.yml`](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/release-macos.yml) | macOS dynamic + static `.tar.gz` |
| [`release-windows.yml`](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/release-windows.yml) | Windows dynamic + static `.zip` |
| [`release-installer.yml`](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/release-installer.yml) | QtIFW cross-platform installers |
| [`release-sdk.yml`](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/release-sdk.yml) | `mnecpp-X.Y.Z-sdk-<plat>.tar.gz` (CMake `find_package`) |

## Code signing

Tagged releases are signed or they fail; `staging` and `main` builds stay
unsigned. `release-evidence.json` and the release notes record the status of
every artifact, and `SHA256SUMS` covers all of them.

**Windows (Authenticode via [SignPath Foundation](https://signpath.org)).**
The [`signpath-sign`](https://github.com/mne-tools/mne-cpp/blob/main/.github/actions/signpath-sign/action.yml)
action submits the deployment `.zip` files and the installer `.exe` to the
SignPath project `mne-cpp` (signing policy `release-signing`) and verifies
every signed file with `signtool verify /pa /all /tw`. Repository settings:

| Kind | Name | Value |
|------|------|-------|
| Secret | `SIGNPATH_API_TOKEN` | API token of a submitter in the SignPath project |
| Variable | `SIGNPATH_ORGANIZATION_ID` | SignPath organization ID |

Artifact configurations in SignPath (files are uploaded unwrapped):

```xml
<!-- windows-archive -->
<artifact-configuration xmlns="http://signpath.io/artifact-configuration/v1">
  <zip-file>
    <pe-file-set>
      <include path="**/mne_*.exe" max-matches="unbounded" />
      <include path="**/mne_*.dll" min-matches="0" max-matches="unbounded" />
      <include path="**/mne_*_plugins/*.dll" min-matches="0" max-matches="unbounded" />
      <for-each><authenticode-sign /></for-each>
    </pe-file-set>
  </zip-file>
</artifact-configuration>

<!-- windows-installer -->
<artifact-configuration xmlns="http://signpath.io/artifact-configuration/v1">
  <pe-file><authenticode-sign /></pe-file>
</artifact-configuration>
```

**macOS (Developer ID, paid Apple Developer Program).**
[`macos-codesign-setup`](https://github.com/mne-tools/mne-cpp/blob/main/.github/actions/macos-codesign-setup/action.yml)
imports the certificate into a temporary keychain; the `.app` bundles are
signed with the hardened runtime, and
[`macos_notarize.sh`](https://github.com/mne-tools/mne-cpp/blob/main/scripts/deploy/macos_notarize.sh)
signs, notarizes and staples each `.dmg`, then checks it with
`codesign --verify`, `stapler validate` and `spctl --assess`. Secrets:
`APPLE_DEVELOPER_ID_P12` (base64 `.p12`), `APPLE_DEVELOPER_ID_PASSWORD`,
`APPLE_ID`, `APPLE_APP_PASSWORD` (app-specific password) and `APPLE_TEAM_ID`.
The `.tar.gz` archives are covered by `SHA256SUMS` only.

## Post-release

1. Verify all artifacts are attached to the release page.
2. Verify the website's "Latest version" banner updated
   (auto via Docusaurus `version` field).
3. Check Zenodo minted a new versioned DOI (concept DOI is `10.5281/zenodo.593102`).
4. Open the next release's task sheet (`doc/dev-notes/vX.Y.(Z+1)-requirements.md`).

## Patch releases

For `vX.Y.(Z+1)`:
1. Cherry-pick fixes from `staging` onto `main`.
2. Bump `CHANGELOG`, `CITATION.cff`, `README` badge.
3. Tag `vX.Y.(Z+1)` on `main`.
