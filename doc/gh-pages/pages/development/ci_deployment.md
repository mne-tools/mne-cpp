---
title: Deployment
parent: Continuous Integration
grand_parent: Develop
nav_order: 2
---

# Deployment

|**Please note:** The information below only hold for dynamically linked builds of MNE-CPP. When building statically, no dependency solving is necessary. Due to their larger file size our CI excludes statically pre-built examples and tests from the satic release binaries.|

This page explains how MNE-CPP handles build setups, solves for exernal as well as internal dependencies, prepares resources, and does packaging.

## Build Rules

All MNE-CPP libraries are built to the `mne-cpp/lib` folder. All applications, test, and examples are built to the `mne-cpp/bin` folder.

## Dependency Solving

On Windows and MacOS, dependency solving for libraries and executables is done via the `windeployqt` and `macdeployqt` tools, which are officially developed and maintained by Qt. For Linux we use the unoffical [linuxdeployqt](https://github.com/probonopd/linuxdeployqt){:target="_blank" rel="noopener"} tool. Calling these tools is performed in our workflow [release.yml](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/release.yml){:target="_blank" rel="noopener"} file which again calls the deployment scripts in `tools/deployment`.

### Internal Dependencies (MNE-CPP libraries) 

Applications, tests and examples link against MNE-CPP libraries (internal dependencies). Dependencies between MNE-CPP libraries exist as well and can be seen in the [libraries.pro file](https://github.com/mne-tools/mne-cpp/blob/master/libraries/libraries.pro){:target="_blank" rel="noopener"}. The following table describes how we solve for internal dependencies:

| Platform                    | Dependency solving                     |
| --------------------------- | -------------------------------------- |
| Windows | All MNE-CPP libraries are copied from `mne-cpp/lib` to `mne-cpp/bin` via the library's .pro file. This is needed since Windows does not support rpaths and `windeployqt` only takes care of Qt related dependencies.| 
| Linux | MNE-CPP libraries reside in `mne-cpp/lib`. The `RPATH` is specified in the executable's .pro file in order to link to the libraries in `mne-cpp/lib`. | 
| MacOS | For .app bundles MNE-CPP libraries are copied to the .app `Contents/Frameworks` folder by the [release.yml](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/release.yml){:target="_blank" rel="noopener"} workflow file. For none .app bundles, tests and examples the rpath is setup in the library's and application's .pro file pointing to `mne-cpp/lib`. |

### External Dependencies (Qt, Eigen and System Libraries)

MNE-CPP depends on [Qt](https://www.qt.io/){:target="_blank" rel="noopener"} and [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page){:target="_blank" rel="noopener"}. Eigen, as a lightweight template library, [is included in the MNE-CPP repository by default](https://github.com/mne-tools/mne-cpp/tree/master/include/3rdParty/eigen3){:target="_blank" rel="noopener"} and does not need further dependency solving. For Qt dependencies we do the following:

| Platform                    | Dependency solving                     |
| --------------------------- | -------------------------------------- |
| Windows | Call `windeployqt` on MNE Scan and the Disp3D library DLL. Disp3D is the most top level library and links against all needed Qt modules. MNE Scan links against all relevant Qt modules. Subsequently, Qt and all needed system libraries reside in `mne-cpp/bin`. |
| Linux | Call `linuxdeployqt` on MNE Scan only, see the [release.yml](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/release.yml){:target="_blank" rel="noopener"} workflow file. This will copy Qt libraries to `mne-cpp/lib` and Qt plugins to `mne-cpp/plugins`. The `RPATH` is specified in the executable's .pro file in order to link to the libraries in `mne-cpp/lib`. |
| MacOS | All applications, examples and tests can be created as .app bundles (via the `withAppBundles` flag). In this case `macdeployqt` is used to solve for Qt dependencies, see the [release.yml](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/release.yml){:target="_blank" rel="noopener"} workflow file. Please note, `macdeployqt` only works on .app bundles. In the non .app bundle case you need to specify `export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:<Qt_path>/lib` in orer to find the Qt dependencies. |

## Resource Handling

Files which are needed by applications (layout files, selection groups, etc.) are considrered resources and reside in `mne-cpp/bin/resources`. In case of .app images on MacOS, the needed resources are copied from `mne-cpp/resources` to `mne-cpp/bin/mne_scan.app/MacOs/resources`. 

## Packaging and Uploading

| Platform                    | Packaged folders                      |
| --------------------------- | ------------------------------------ |
| Windows | Folder `mne-cpp/bin` is compressed to a zip file and uploaded as release asset to the corresponding GitHub release. |
| Linux | Folders `mne-cpp/bin`, `mne-cpp/lib`, `mne-cpp/plugins`, `mne-cpp/translations` are compressed to a tar.gz file and uploaded as release assets to the corresponding GitHub release. |
| MacOS | Folders `mne-cpp/bin`, `mne-cpp/lib` are compressed to a tar.gz file and uploaded as release assets to the corresponding GitHub release.  |