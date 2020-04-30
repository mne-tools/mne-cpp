---
title: Continuous Integration
parent: Develop
nav_order: 2
---
# Continuous Integration

## Github Actions

MNE-CPP facilitates Github Actions to do continous integration (CI). Github Actions operates on Github events which trigger so called workflows. You can read more on the terminology [here](https://help.github.com/en/actions/reference){:target="_blank" rel="noopener"}. Currently, the following Github events trigger MNE-CPP's CI pipeline:

| Event type | Workflow Name | Workflow Script | Effect |
| ---------- | ------------- | --------------- | ------ |
| Pull Requests | `PullRequest` | [pullrequest.yml](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/pullrequest.yml){:target="_blank" rel="noopener"} | Triggers checks to run on the PR code.| 
| Pushes/Merges to `master` | `Linux|MacOS|Win|WASM` | [release.yml](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/release.yml){:target="_blank" rel="noopener"} | Triggers the Development Release binaries to be updated with the most recently pushed changes. This workflow basically follows the idea of nightly builds. |
| Publishing a new release with tag syntax `v0.x.y` | `Linux|MacOS|Win|WASM` | [release.yml](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/release.yml){:target="_blank" rel="noopener"} | Triggers stable release processing described in more detail below. |
| Pushes to the `docu` branch | `DocuTest` | [docutest.yml](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/docutest.yml){:target="_blank" rel="noopener"} | Creates a new version of the documentation website and makes them accessible via the repository's `gh-pages` branch. |
| Pushes to the `wasm` branch | `WasmTest` | [wasmtest.yml](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/wasmtest.yml){:target="_blank" rel="noopener"} | Creates new versions of the WebAssembly capable MNE-CPP applications and makes them accessible via the repository's `gh-pages` branch. |
| Timer runs out | `Coverity` | [coverity.yml](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/coverity.yml){:target="_blank" rel="noopener"} | Triggers every two days to run [Coverity](https://scan.coverity.com/projects/mne-tools-mne-cpp){:target="_blank" rel="noopener"} static code analysis tools. |
| Pushes to the `generateqt` branch | `BuildQtBinaries` | [buildqtbinaries.yml](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/buildqtbinaries.yml){:target="_blank" rel="noopener"} | Triggers builds of all needed Qt versions and makes them accesible as [artifacts via the Github Actions interface](https://help.github.com/en/actions/configuring-and-managing-workflows/persisting-workflow-data-using-artifacts){:target="_blank" rel="noopener"}. |  

## Releases

New development takes place on the `master` branch. Once the developers have rough consensus we create a new stable release on GitHub, following the `v0.x.y` tag syntax. This will trigger the following steps, which are fully automated by the [release.yml](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/release.yml){:target="_blank" rel="noopener"} workflow script:

1. Increment version numbers in [mne-cpp.pri](https://github.com/mne-tools/mne-cpp/blob/master/mne-cpp.pri){:target="_blank" rel="noopener"} and [mne-cpp_doxyfile](https://github.com/mne-tools/mne-cpp/blob/master/doc/doxygen/mne-cpp_doxyfile){:target="_blank" rel="noopener"} by `0.x.y`.
2. Create new branch named `v0.x.y` based on current `master` branch.
3. Build dynamically as well as statically linked binaries and upload them to the corresponding release on Github.

## Solving for dependencies

### Internal resources and dependencies

|Platform                     |MNE-CPP Libraries                       |MNE-CPP Applications                   |MNE-CPP Examples                  |
| --------------------------- | -------------------------------------- | ------------------------------------- | -------------------------------- |
|Windows |Build into `mne-cpp/lib` and copied afterwards to `mne-cpp/bin`. Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. | Build to `mne-cpp/bin`. Either Qt's lib folder needs to be added to PATH in order to start applications from explorer or application needs to be started from inside QtCreator. Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. | Build to ``mne-cpp/bin``. Either Qt's lib folder needs to be added to PATH in order to start examples from explorer or examples needs to be started from inside QtCreator. Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. |
|Linux |Build into `mne-cpp/lib`. Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. | Build to `mne-cpp/bin/resources`. Qt's and MNE-CPP's lib folder need to be added to `LD_LIBRARY_PATH` to start applications from navigator. Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. |Build to `mne-cpp/bin`. Qt's and MNE-CPP's lib folder need to be added to `LD_LIBRARY_PATH` to start examples from navigator. Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. |
|MacOS |Build into `mne-cpp/lib`. Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin`. | Build to `mne-cpp/bin`. Build as .app or .dmg (MNE Scan, MNE Analyze). Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. | Build to `mne-cpp/bin`. Qt's and MNE-CPP's lib folder need to be added to `DYLD_LIBRARY_PATH`. Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. |

### External dependencies

|Platform                     |MNE-CPP Libraries                       |MNE-CPP Applications                   |MNE-CPP Examples                  |
| --------------------------- | -------------------------------------- | ------------------------------------- | -------------------------------- |
|Windows |Copied to `mne-cpp/bin`. windeployqt on MNE-CPP libraries which were already copied to `mne-cpp/bin`. Afterwards, Qt libs reside in `mne-cpp/bin` as well. | windeployqt on MNE-CPP all applications. | windeployqt on MNE-CPP all examples. |
|Linux |Reside in `mne-cpp/lib`. |[linuxdeployqt](https://github.com/probonopd/linuxdeployqt){:target="_blank" rel="noopener"} on MNE Scan. The Qt and MNE-CPP libs are looked up via `LD_LIBRARY_PATH`. `RPATH` is set to point to `mne-cpp/lib` folder. | The Qt and MNE-CPP libs are looked up via `LD_LIBRARY_PATH`. `RPATH` is set to point to `mne-cpp/lib` folder. |
|MacOS |Reside in `mne-cpp/lib`. |Some applications are created as .app or .dmg (MNE Scan, MNE Analyze) via macdeployqt. macdeployqt also copies in Qt and MNE-CPP libraries to the .app and .dmg bundles. Needed resources are copied from `mne-cpp/resources` to .app folders. | Examples are not build as .app or .dmg bundles. The Qt and MNE-CPP libs are looked up via `DYLD_LIBRARY_PATH`. |

### Packaging

|Platform                     |MNE-CPP Libraries                       |MNE-CPP Applications                   |MNE-CPP Examples                  |
| --------------------------- | -------------------------------------- | ------------------------------------- | -------------------------------- |
|Windows |Libraries are packaged as well since they reside in the `mne-cpp/bin` folder. |The `mne-cpp/bin` folder including the MNE-CPP libraries is is zipped as a whole and uploaded as a release asset to GitHub. | Examples are packaged for dynamically linked versions only. |
|Linux |Libraries are compressed to a tar.gz file and uploaded as a release asset to GitHub.. |The whole `mne-cpp/bin` and `mne-cpp/lib` folder are compressed into a single .tar.gz file and uploaded as a release asset to GitHub. |Examples are packaged for dynamically linked versions only. |
|MacOS |Libraries are not packaged. They can only be found as part of the .dmg bundles. |MNE Scan and MNE Analyze created as .dmg files and uploaded as a release asset to GitHub. |Examples are packaged for dynamically linked versions only. |
