---
title: Deployment
parent: Continuous Integration
grand_parent: Develop
nav_order: 2
---

# Deployment

This page explains how MNE-CPP handles build setups, solves for exernal as well as internal dependencies, prepares resources, and does packaging.

## Build Rules

All MNE-CPP libraries are built to the `mne-cpp/lib` folder. All applications, test, and examples are built to the `mne-cpp/bin` folder.

## Dependency Solving

On Windows and MacOS, dependency solving for libraries and executables is done in the executable's corresponding `.pro` file. Here, the `windeployqt` and `macdeployqt` tools are used, which are officially developed and maintained by Qt. Two functions `defineReplace(macDeployArgs)` and `defineReplace(winDeployArgs)` are provided in the [mne-cpp.pri](https://github.com/mne-tools/mne-cpp/blob/master/mne-cpp.pri){:target="_blank" rel="noopener"} file, which are used to deploy executables on Windows and MacOS. For Linux we use the unoffical [linuxdeployqt](https://github.com/probonopd/linuxdeployqt){:target="_blank" rel="noopener"} tool which is executed in the [release.yml](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/release.yml){:target="_blank" rel="noopener"} workflow file.

### Internal Dependencies (MNE-CPP libraries) 

Applications, tests, and examples, link against MNE-CPP libraries (internal dependencies). Dependencies between MNE-CPP libraries exist as well and can be seen in the [libraries.pro file](https://github.com/mne-tools/mne-cpp/blob/master/libraries/libraries.pro){:target="_blank" rel="noopener"}. The following table describes how we solve for internal dependencies:

| Platform                    | Dependency solving                     |
| --------------------------- | -------------------------------------- |
| Windows | `windployqt` is called on all MNE-CPP libraries. Also, all MNE-CPP libraries are copied from `mne-cpp/lib` to `mne-cpp/bin` via the library's .pro file. This is needed since `windeployqt` only takes care of Qt related dependencies.| 
| Linux | MNE-CPP libraries reside in `mne-cpp/lib`. `QMAKE_RPATHDIR` is specified in the executable's .pro file in order to link to the libraries in `mne-cpp/lib`. | 
| MacOS | MNE-CPP libraries are copied to the .app `Frameworks` folder by `macdeployqt`. Tests and examples are created as normal executables and therefore need `DYLD_LIBRARY_PATH` to include the `mne-cpp/lib` folder. |

### External Dependencies (Qt, Eigen, and System Libraries)

As of right now MNE-CPP depends on [Qt](https://www.qt.io/){:target="_blank" rel="noopener"} and [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page){:target="_blank" rel="noopener"}. Eigen, as a lightweight template library, [is included in the MNE-CPP repository by default](https://github.com/mne-tools/mne-cpp/tree/master/include/3rdParty/eigen3){:target="_blank" rel="noopener"} and does not need further dependency solving. For Qt dependencies we do the following:

| Platform                    | Dependency solving                     |
| --------------------------- | -------------------------------------- |
| Windows | Call `windeployqt` on MNE-CPP applications, tests, and examples. Subsequently, Qt and needed system libraries reside in `mne-cpp/bin`. |
| Linux | Call `linuxdeployqt` on MNE Scan only. The Qt and MNE-CPP libraries are looked up via `RPATH`, which is set to point to the `mne-cpp/lib` folder. |
| MacOS | Call `windeployqt` on MNE-CPP applications, tests, and examples. Applications (MNE Scan, MNE Analyze, etc.) are created as .app bundles via `macdeployqt`. Tests and examples are created as normal executables and therefore need `DYLD_LIBRARY_PATH` to include the `mne-cpp/lib` folder. |

## Resource Handling

Files which are needed by the applications, e.g., layout files, selection groups and so on, are considrered resources and reside in `mne-cpp/resources`. Needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/resources`. In case of .app images on MacOS, e.g., MNE Scan, the needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/mne_scan.app/MacOs/resources`. 

| **Please note:** If you need to add new resources, please add them to the `mne-cpp/resources` and **NOT** to the `mne-cpp/bin/resources` folder. Also, make sure to include them in your .pro file as well. | 

## Packaging and Uploading

| Platform                    | Packaged folders                      |
| --------------------------- | ------------------------------------ |
| Windows | Folder `mne-cpp/bin` is compressed to a zip file and uploaded as release asset to the corresponding GitHub release. |
| Linux | Folders `mne-cpp/bin`, `mne-cpp/lib`, `mne-cpp/plugins`, `mne-cpp/translations` are compressed to a tar.gz file and uploaded as release assets to the corresponding GitHub release. |
| MacOS | Folders `mne-cpp/bin`, `mne-cpp/lib` are compressed to a tar.gz file and uploaded as release assets to the corresponding GitHub release.  |

## Static Builds

The above information only holds for dynamically linked builds of MNE-CPP. When building statically, no dependency solving is necessary. Please note that our CI excludes examples and tests from static release binaries, since they would consume too much space.