---
title: Build/Run & Deployment & Packaging/Distribution
parent: Develop
nav_order: 2
---
# Build/Run & Deployment & Packaging/Distribution

Everytime a merge to mne-tools/mne-cpp:master occurs the [devbuild workflow in Github Actions](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/devbuilds.yml) is triggered. This workflow builds, solves for dependencies, packages and dsitributes the last commited version to our [development release](https://github.com/mne-tools/mne-cpp/releases). 

New development takes place on master until the developers decide it is time for a new stable release. At that point we merge the current master branch into the stable branch, ask developers and users to test the stable branch, and once we have rough consensus we create a new release on GitHub.

## Build/Run

Compiling MNE-CPP and running applications.

|Platform                     |MNE-CPP Libraries                       |MNE-CPP Applications                   |MNE-CPP Examples                  |
| --------------------------- | -------------------------------------- | ------------------------------------- | -------------------------------- |
|Windows |Build into `mne-cpp/lib` and copied afterwards to `mne-cpp/bin`. Managed in libraries' .pro files. Needed resources are copied from mne-cpp/resources to `mne-cpp/bin`. | Build to `mne-cpp/bin`. Either Qt's lib folder needs to be added to PATH in order to start applications from explorer or application needs to be started from inside QtCreator. Needed resources are copied from mne-cpp/resources to `mne-cpp/bin`. | Build to ``mne-cpp/bin``. Either Qt's lib folder needs to be added to PATH in order to start examples from explorer or examples needs to be started from inside QtCreator. Needed resources are copied from mne-cpp/resources to `mne-cpp/bin`. |
|Linux |Build into `mne-cpp/lib`. Managed in libraries' .pro files. Needed resources are copied from mne-cpp/resources to `mne-cpp/bin`. | Build to `mne-cpp/bin`. Qt's and MNE-CPP's lib folder need to be added to `LD_LIBRARY_PATH` to start applications from navigator. Needed resources are copied from mne-cpp/resources to `mne-cpp/bin`. |Build to `mne-cpp/bin`. Qt's and MNE-CPP's lib folder need to be added to LD_LIBRARY_PATH to start examples from navigator. Needed resources are copied from mne-cpp/resources to `mne-cpp/bin`. |
|MacOS |Build into `mne-cpp/lib`. Managed in libraries' .pro files. Needed resources are copied from mne-cpp/resources to `mne-cpp/bin`. | Build to `mne-cpp/bin`. Build as .app or .dmg (MNE Scan, MNE Analyze). Needed resources are copied from mne-cpp/resources to `mne-cpp/bin`. |Build to `mne-cpp/bin`. Qt's and MNE-CPP's lib folder need to be added to `DYLD_LIBRARY_PATH`. Needed resources are copied from mne-cpp/resources to `mne-cpp/bin`. |

## Solving for Dependencies

Handling dependencies.

|Platform                     |MNE-CPP Libraries                       |MNE-CPP Applications                   |MNE-CPP Examples                  |
| --------------------------- | -------------------------------------- | ------------------------------------- | -------------------------------- |
|Windows |Copied to `mne-cpp/bin`. Managed in libraries' .pro files. windeployqt on MNE-CPP libraries which were already copied to `mne-cpp/bin`. Afterwards, Qt libs reside in `mne-cpp/bin` as well.  |windeployqt on MNE-CPP applications. Managed in .pro files.  |windeployqt on MNE-CPP examples. Managed in .pro files. |
|Linux |Reside in `mne-cpp/lib`. Managed in .pro files. |[linuxdeployqt](https://github.com/probonopd/linuxdeployqt) on MNE Scan. No real individual deployment so far. The Qt and MNE-CPP libs are looked up via `LD_LIBRARY_PATH`. `RPATH` is set to point to `mne-cpp/lib` folder. Managed in .pro files. | No individual deployment. The Qt and MNE-CPP libs are looked up via `LD_LIBRARY_PATH`. RPATH is set to point to `mne-cpp/lib` folder. Managed in .pro files.  |
|MacOS |Reside in `mne-cpp/lib`. Managed in .pro files. |Some applications are created as .app or .dmg (MNE Scan, MNE Analyze). macdeployqt also copies in Qt and MNE-CPP libraries to the .app and .dmg bundles. Needed resources are copied from mne-cpp/resources to .app folders. Managed in .pro files. |No real individual deployment so far. Examples are not build as .app or .dmg bundles. The Qt and MNE-CPP libs are looked up via `DYLD_LIBRARY_PATH`. Managed in .pro files.  |

## Packaging/Distribution

Creating setups, zipped folders for uploading and distribution.

|Platform                     |MNE-CPP Libraries                       |MNE-CPP Applications                   |MNE-CPP Examples                  |
| --------------------------- | -------------------------------------- | ------------------------------------- | -------------------------------- |
|Windows |Libraries are packaged as well since they reside in the `mne-cpp/bin` folder. |The `mne-cpp/bin` folder including the MNE-CPP libraries is is zipped as a whole and uploaded to website. QtInstallerFramework packages everything it needs from `mne-cpp/bin` into a setup file. Upload setup to website. |Examples are packaged as well since they reside in the `mne-cpp/bin` folder. |
|Linux |Libraries are compressed to a tar.gz file. |The whole `mne-cpp/bin` and `mne-cpp/lib` folder are compressed into a single .tar.gz file and uploaded to website. |Examples are compressed to a tar.gz file. |
|MacOS |Libraries are not packaged. They can only be found as part of the .dmg bundles. |MNE Scan and MNE Analyze created as .dmg files and uploaded to website. |Examples are not packaged. |
