---
title: CI Pipeline
parent: Develop
nav_order: 2
---
# CI (Continuous Integration) Pipeline

Everytime a merge to mne-tools/mne-cpp:master occurs the [devbuild workflow in Github Actions](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/devbuilds.yml){:target="_blank" rel="noopener"} is triggered. This workflow builds, solves for dependencies, packages and distributes the last commited version to our [development release](https://github.com/mne-tools/mne-cpp/releases){:target="_blank" rel="noopener"}. 

New development takes place on master until the developers decide it is time for a new stable release. At that point we merge the current master branch into the stable branch, ask developers and users to test the stable branch, and once we have rough consensus we create a new release on GitHub.

## Build/Run

Compiling MNE-CPP and running applications.

|Platform                     |MNE-CPP Libraries                       |MNE-CPP Applications                   |MNE-CPP Examples                  |
| --------------------------- | -------------------------------------- | ------------------------------------- | -------------------------------- |
|Windows |Build into `mne-cpp/lib` and copied afterwards to `mne-cpp/bin`. Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. | Build to `mne-cpp/bin`. Either Qt's lib folder needs to be added to PATH in order to start applications from explorer or application needs to be started from inside QtCreator. Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. | Build to ``mne-cpp/bin``. Either Qt's lib folder needs to be added to PATH in order to start examples from explorer or examples needs to be started from inside QtCreator. Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. |
|Linux |Build into `mne-cpp/lib`. Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. | Build to `mne-cpp/bin/resources`. Qt's and MNE-CPP's lib folder need to be added to `LD_LIBRARY_PATH` to start applications from navigator. Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. |Build to `mne-cpp/bin`. Qt's and MNE-CPP's lib folder need to be added to `LD_LIBRARY_PATH` to start examples from navigator. Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. |
|MacOS |Build into `mne-cpp/lib`. Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin`. | Build to `mne-cpp/bin`. Build as .app or .dmg (MNE Scan, MNE Analyze). Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. | Build to `mne-cpp/bin`. Qt's and MNE-CPP's lib folder need to be added to `DYLD_LIBRARY_PATH`. Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. |

## Solving for Dependencies

Handling dependencies.

|Platform                     |MNE-CPP Libraries                       |MNE-CPP Applications                   |MNE-CPP Examples                  |
| --------------------------- | -------------------------------------- | ------------------------------------- | -------------------------------- |
|Windows |Copied to `mne-cpp/bin`. windeployqt on MNE-CPP libraries which were already copied to `mne-cpp/bin`. Afterwards, Qt libs reside in `mne-cpp/bin` as well. | windeployqt on MNE-CPP all applications. | windeployqt on MNE-CPP all examples. |
|Linux |Reside in `mne-cpp/lib`. |[linuxdeployqt](https://github.com/probonopd/linuxdeployqt){:target="_blank" rel="noopener"} on MNE Scan. The Qt and MNE-CPP libs are looked up via `LD_LIBRARY_PATH`. `RPATH` is set to point to `mne-cpp/lib` folder. | The Qt and MNE-CPP libs are looked up via `LD_LIBRARY_PATH`. `RPATH` is set to point to `mne-cpp/lib` folder. |
|MacOS |Reside in `mne-cpp/lib`. |Some applications are created as .app or .dmg (MNE Scan, MNE Analyze) via macdeployqt. macdeployqt also copies in Qt and MNE-CPP libraries to the .app and .dmg bundles. Needed resources are copied from `mne-cpp/resources` to .app folders. | Examples are not build as .app or .dmg bundles. The Qt and MNE-CPP libs are looked up via `DYLD_LIBRARY_PATH`. |

## Packaging/Distribution

Creating setups, zipped folders for uploading and distribution.

|Platform                     |MNE-CPP Libraries                       |MNE-CPP Applications                   |MNE-CPP Examples                  |
| --------------------------- | -------------------------------------- | ------------------------------------- | -------------------------------- |
|Windows |Libraries are packaged as well since they reside in the `mne-cpp/bin` folder. |The `mne-cpp/bin` folder including the MNE-CPP libraries is is zipped as a whole and uploaded as a release asset to GitHub. | Examples are packaged as well since they reside in the `mne-cpp/bin` folder. |
|Linux |Libraries are compressed to a tar.gz file and uploaded as a release asset to GitHub.. |The whole `mne-cpp/bin` and `mne-cpp/lib` folder are compressed into a single .tar.gz file and uploaded as a release asset to GitHub. |Examples are compressed to a tar.gz file and uploaded as a release asset to GitHub. |
|MacOS |Libraries are not packaged. They can only be found as part of the .dmg bundles. |MNE Scan and MNE Analyze created as .dmg files and uploaded as a release asset to GitHub. |Examples are not packaged. |
