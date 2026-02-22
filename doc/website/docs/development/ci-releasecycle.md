---
title: Release Guide
sidebar_label: Release Guide
---

# Release Guide

New development takes place on the `main` branch. Once the developers have rough consensus we create a new stable release on GitHub, following the `v0.x.y` tag syntax. The MNE-CPP stable release steps are:

1. (Manually) Increment version number in [CMakeLists.txt](https://github.com/mne-tools/mne-cpp/blob/main/CMakeLists.txt) and [mne-cpp_doxyfile](https://github.com/mne-tools/mne-cpp/blob/main/doc/doxygen/mne-cpp_doxyfile) by `0.x.y`.
2. (Manually) Change version numbers of applications, e.g., [MNE Scan](https://github.com/mne-tools/mne-cpp/blob/main/applications/mne_scan/mne_scan/info.h) and [MNE Analyze](https://github.com/mne-tools/mne-cpp/blob/main/applications/mne_analyze/mne_analyze/info.h).
3. (Manually) Prepare and update the release table on the documentation website.
4. (Manually) Prepare and update the changelog. The changelog of the to-be-released version can be found on the [GitHub Wiki](https://github.com/mne-tools/mne-cpp/wiki/Changelog-WIP). Note: The number of commits per contributor can be listed via: `git shortlog -s -n <branchName> --no-merges --since="<dateOfLastRelease"`.
5. (Manually) Create release with tag `v0.x.y` via Github. 
6. (Automated by [release.yml](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/release.yml)) Create a new branch named `v0.x.y` based on current `main` branch. A new branch is only created for minor version releases, e.g., going from `v0.1.0` to `v0.2.0`.
7. (Automated by [release.yml](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/release.yml)) Build dynamically as well as statically linked binaries and upload them to the `v0.x.y` release on Github.
8. (Manually) In case of a minor or major version bump, create branch protection rules for the newly created branch via the Github web interface.
