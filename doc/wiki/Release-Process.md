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
3. Versioned files are bumped to `X.Y.Z`:
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
