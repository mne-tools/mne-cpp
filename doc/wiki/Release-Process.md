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
   - [`doc/Doxyfile`](https://github.com/mne-tools/mne-cpp/blob/main/doc/Doxyfile) and `doc/Doxyfile.internal` — `PROJECT_NUMBER`
   - [`CITATION.cff`](https://github.com/mne-tools/mne-cpp/blob/main/CITATION.cff) — `version:` and `date-released:`
   - [`README.md`](https://github.com/mne-tools/mne-cpp/blob/main/README.md) — version badge
   - [`CHANGELOG.md`](https://github.com/mne-tools/mne-cpp/blob/main/CHANGELOG.md) — new section with highlights
   - [`doc/website/docusaurus.config.ts`](https://github.com/mne-tools/mne-cpp/blob/main/doc/website/docusaurus.config.ts) — site version
4. Doxygen passes with **zero warnings** for new/changed code.
5. Website builds without broken links.

## Cut the release

1. Run **Release Candidate** (`release-candidate.yml`, *Run workflow* on
   `staging`). It is GO only when the Staging run of that commit is green,
   `scripts/release/preflight.py` passes, every dashboard criterion is met
   (`generate_quality_report.py --gates`), the signing credentials are set and
   the evidence bundle verifies. Approve the `release` environment; the bundle
   stays attached to the run for 90 days.
2. Release exactly the approved commit `<SHA>`; `main.yml` refuses a tag whose
   commit has no successful candidate run:

```bash
python3 scripts/release/preflight.py --commit <SHA> --remote origin
git checkout main
git pull --ff-only origin main
git merge --ff-only <SHA>
git push origin main
git tag -a vX.Y.Z -m "Release vX.Y.Z — <codename>" <SHA>
git push origin vX.Y.Z
```

## Rerun and recovery

Every release step can be rerun: assets are uploaded with `--clobber`, the
evidence job rewrites `SHA256SUMS`, `release-evidence.json` and its section of
the release notes in place, and the website deploy replaces the site.

| Situation | Action |
|-----------|--------|
| A platform/installer job failed | *Re-run failed jobs* on the tag's `main.yml` run, then re-run `ReleaseEvidence` and `VerifyRelease`. |
| An asset is missing or corrupt on the release page | Re-run the job that uploads it; `VerifyRelease` fails until `SHA256SUMS` and the asset agree. |
| Website deploy failed | Re-run the `Website` job; nothing else depends on it. |
| `dev_build` is broken or deleted | Run `staging.yml` manually; it recreates the pre-release from the last commit whose builds all compiled. |
| A wrong commit was tagged | Do not move the tag. Fix forward with `vX.Y.(Z+1)`; deleting a published tag needs the maintainers' explicit approval because downstream mirrors and Zenodo have already archived it. |

Recovery never needs credentials on a local machine: signing, notarization
and uploads run only in CI from repository secrets.

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

Tagged releases are signed or they fail, unless a platform is opted out below.
The macOS `staging` and `main` builds are signed and notarized too; their
Windows builds stay unsigned. `release-evidence.json` and the release notes record the status of
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

The two artifact configurations (files are uploaded unwrapped) are kept in the
repository and must be pasted unchanged into SignPath:

| SignPath slug | Repository file | Signs |
|---|---|---|
| `windows-archive` | [`.signpath/artifact-configurations/windows-archive.xml`](https://github.com/mne-tools/mne-cpp/blob/main/.signpath/artifact-configurations/windows-archive.xml) | `mne_*.exe`, `mne_*.dll` and `mne_*_plugins/*.dll` inside the deployment `.zip` files; Qt and other third-party DLLs are not signed |
| `windows-installer` | [`.signpath/artifact-configurations/windows-installer.xml`](https://github.com/mne-tools/mne-cpp/blob/main/.signpath/artifact-configurations/windows-installer.xml) | the Qt Installer Framework `.exe` itself |

Both restrict every signed file to `product-name="MNE-CPP"` and
`product-version="${version}"`, as the SignPath Foundation requires. The action
passes `version` (from `src/CMakeLists.txt`) with each request and first runs
`scripts/release/pe_metadata.py check` against the same XML, so a binary without
the version resource of
[`cmake/mne_version_info.rc.in`](https://github.com/mne-tools/mne-cpp/blob/main/cmake/mne_version_info.rc.in)
fails the build with its name. The installer template `installerbase.exe` gets
that resource from `pe_metadata.py stamp` before `binarycreator` runs.

**macOS (Developer ID, paid Apple Developer Program).** The apps, tools and
libraries of the dynamic build, the `.dmg` images and the QtIFW installer are
signed with the hardened runtime, notarized with `notarytool`, stapled and
verified before upload; see [MacOS-Code-Signing.md](MacOS-Code-Signing.md) for the
pipeline, the three operating modes and the secrets (`APPLE_APPLICATION_CERT_BASE64`,
`APPLE_APPLICATION_CERT_PASSWORD`, `APPLE_ID`, `APPLE_ID_PASSWORD`, `APPLE_TEAM_ID`).
The `.tar.gz` archives contain the notarized code and are covered by `SHA256SUMS`.

To release without an Apple Developer ID, set the repository variable
`MACOS_SIGNING` to `unavailable`. The macOS builds are then only ad-hoc signed,
the release candidate no longer asks for the Apple secrets, the release notes
mark both `.dmg` images as unsigned and explain how to open them, and the
release verification requires exactly that. Delete the variable once the Apple
secrets are configured.

Likewise, to release before SignPath's release certificate is active, set
`WINDOWS_SIGNING` to `unavailable`. The Windows `.zip` and `.exe` assets are
then not submitted to SignPath, the release notes mark them as unsigned and
explain the SmartScreen warning, and the release verification requires exactly
that. Delete the variable once the `release-signing` policy is valid.

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
