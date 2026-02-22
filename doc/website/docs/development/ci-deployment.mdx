---
title: Deployment
sidebar_label: Deployment
---

# Deployment

This page explains how MNE-CPP handles deployment on Win, Linux and MacOS.

## Build Rules

All MNE-CPP libraries are built to the `mne-cpp/lib` folder. All applications, test, and examples are built to the `mne-cpp/out/Release` folder.

## Dependency Solving
MNE-CPP depends on [Qt](https://www.qt.io/) and [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page). Eigen, as a lightweight template library, [is included in the MNE-CPP repository by default](https://github.com/mne-tools/mne-cpp/tree/main/include/3rdParty/eigen3) and does not need further dependency solving. For Qt dependencies we use Qt specific deployment tools (`windeployqt`, `linuxdeployqt`, `macdeployqt`).

### Windows
The `WinDynamic` workflow run in [release.yml](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/release.yml) is configured to exclude examples as well as tests. Next to `mne-cpp/lib` the MNE-CPP libraries are also copied to the `mne-cpp/out/Release` folder.

Dependency solving for libraries and executables is done via the `windeployqt` tool, which is officially developed and maintained by Qt. Calling `windeployqt` is performed in the [deploy.bat](https://github.com/mne-tools/mne-cpp/blob/main/tools/deploy.bat) file with `dynamic` as first argument. `windeployqt` is called on the MNE Scan .exe and the Disp3D .dll file. Disp3D is the most top level library and links against all needed Qt modules. MNE Scan links against all relevant Qt modules application wise. Subsequently, Qt and all needed system libraries reside in the `mne-cpp/out/Release` folder.

The no longer needed folder `resources/data/mne-cpp-test-data`is deleted before packaging. The folder `mne-cpp/out/Release` is compressed to a .zip file and uploaded as release asset to the corresponding GitHub release.

### Linux
The `LinuxDynamic` workflow run in [release.yml](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/release.yml) is configured to exclude examples as well as tests. The `RPATH` is configured by CMake to link to the libraries in `mne-cpp/lib`.

Dependency solving for libraries and executables is done via the [linuxdeployqt](https://github.com/probonopd/linuxdeployqt) tool. Calling `linuxdeployqt` is performed in the deployment script under `tools/deployment`.

The, no longer needed, folder `mne-cpp/resources/data/mne-cpp-test-data` is deleted before packaging.  The folders `mne-cpp/out/Release/apps`, `mne-cpp/out/Release/lib`, `mne-cpp/out/Release/plugins`, `mne-cpp/out/Release/translations` are compressed to a .tar.gz file and uploaded as release assets to the corresponding GitHub release. 

### MacOS
The `MacOSDynamic` workflow run in [release.yml](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/release.yml) is configured to exclude examples as well as tests. All applications are built as `.app` bundles.

Dependency solving for libraries and executables is done via the `macdeployqt` tool, which is officially developed and maintained by Qt. All MNE-CPP libraries are manually copied to the `.app`'s `Contents/Frameworks` folder. The resource and applications' plugin folders are copied to the corresponding `mne-cpp/out/Release/apps/<app_name>.app/` folders.

The, no longer needed, folders `mne-cpp/out/Release/apps/mne_scan_plugins`, `mne-cpp/out/Release/apps/mne_analyze_plugins`, `mne-cpp/out/Release/apps/mne_rt_server_plugins` and the symlink `mne-cpp/out/Release/resources` are deleted before packaging. The folder `mne-cpp/out/Release` is compressed to a .tar.gz file and uploaded as release assets to the corresponding GitHub release.

## Static Builds

The information above corresponds to deploying the dynamically built version of MNE-CPP. In case of static deployment the process differs slightly. See the deployment scripts in `tools/deployment`.

## Deployment Scripts

The deployment scripts live in `tools/deployment`. Platform-specific scripts (`deploy.bat` for Windows, `deploy.sh` for Linux/macOS) handle dependency resolution and packaging.

Each script accepts two arguments:

1. **Linkage type**: `dynamic` (default) or `static`.
2. **Packaging**: Pass `pack` as the second argument to generate a compressed archive (`.zip` or `.tar.gz`) containing all binaries and required libraries.

During development, running the deployment script with just the `dynamic` argument is useful to resolve Qt dependencies for a particular MNE-CPP application without creating a full package.
