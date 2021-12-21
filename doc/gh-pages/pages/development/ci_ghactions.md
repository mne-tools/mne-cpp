---
title: Github Actions
parent: Continuous Integration
grand_parent: Development
nav_order: 1
---

# Github Actions

MNE-CPP facilitates Github Actions to do continous integration (CI). Github Actions operates on Github events which trigger so called workflows. You can read more on the terminology [here](https://help.github.com/en/actions/reference){:target="_blank" rel="noopener"}. Currently, the following Github events trigger MNE-CPP's CI pipeline:

| Event type | Workflow Name | Workflow Script | Effect |
| ---------- | ------------- | --------------- | ------ |
| Pull Requests | `PullRequest` | [pullrequest.yml](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/pullrequest.yml){:target="_blank" rel="noopener"} | Triggers checks to run on the PR code.| 
| Pushes/Merges to `main` | `Linux|MacOS|Win|WASM` | [release.yml](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/release.yml){:target="_blank" rel="noopener"} | Triggers the Development Release binaries to be updated with the most recently pushed changes. This workflow basically follows the idea of nightly builds. |
| Publishing a new release with tag syntax `v0.x.y` | `Linux|MacOS|Win|WASM` | [release.yml](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/release.yml){:target="_blank" rel="noopener"} | Triggers stable release processing steps described in more detail [here](ci_releasecycle.md). |
| Pushes to the `docu` branch | `DocuTest` | [docutest.yml](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/docutest.yml){:target="_blank" rel="noopener"} | Creates a new version of the documentation website and makes them accessible via the repository's `gh-pages` branch. |
| Pushes to the `wasm` branch | `WasmTest` | [wasmtest.yml](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/wasmtest.yml){:target="_blank" rel="noopener"} | Creates new versions of the WebAssembly capable MNE-CPP applications and makes them accessible via the repository's `gh-pages` branch. |
| Pushes to the `testci` branch | `TestCI` | [testci.yml](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/testci.yml){:target="_blank" rel="noopener"} | Triggers checks to run cross-platform build setups and tests without the need to create a Github PR. |
| Timer runs out | `Coverity` | [coverity.yml](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/coverity.yml){:target="_blank" rel="noopener"} | Triggers every two days to run [Coverity](https://scan.coverity.com/projects/mne-tools-mne-cpp){:target="_blank" rel="noopener"} static code analysis tools. |
| Pushes to the `generateqt` branch | `BuildQtBinaries` | [buildqtbinaries.yml](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/buildqtbinaries.yml){:target="_blank" rel="noopener"} | Triggers builds of all needed Qt versions and makes them accesible as [artifacts via the Github Actions interface](https://help.github.com/en/actions/configuring-and-managing-workflows/persisting-workflow-data-using-artifacts){:target="_blank" rel="noopener"}. |  