---
title: Deployment
parent: Continuous Integration
grand_parent: Develop
nav_order: 2
---

# Deployment

This page explains how MNE-CPP handles deployment on Win, Linux and MacOS.

## Build Rules

All MNE-CPP libraries are built to the `mne-cpp/lib` folder. All applications, test, and examples are built to the `mne-cpp/bin` folder.

## Dependency Solving
MNE-CPP depends on [Qt](https://www.qt.io/){:target="_blank" rel="noopener"} and [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page){:target="_blank" rel="noopener"}. Eigen, as a lightweight template library, [is included in the MNE-CPP repository by default](https://github.com/mne-tools/mne-cpp/tree/master/include/3rdParty/eigen3){:target="_blank" rel="noopener"} and does not need further dependency solving. For Qt dependencies we use Qt specific deployment tools (`windeployqt`, `linuxdeployqt`, `macdeployqt`).

### Windows
The `WinDynamic` workflow run in [release.yml](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/release.yml){:target="_blank" rel="noopener"} is configured to exclude examples as well as tests. Next to `mne-cpp/lib` the MNE-CPP libraries are also copied to the `mne-cpp/bin` folder. This is performed in the libraries' .pro files. 

Dependency solving for libraries and executables is done via the `windeployqt` tool, which is officially developed and maintained by Qt. Calling `windeployqt` is performed in the [deploy_win.bat](https://github.com/mne-tools/mne-cpp/blob/master/tools/deployment/deploy_win.bat){:target="_blank" rel="noopener"} file. `windeployqt` is called on the MNE Scan .exe and the Disp3D .dll file. Disp3D is the most top level library and links against all needed Qt modules. MNE Scan links against all relevant Qt modules application wise. Subsequently, Qt and all needed system libraries reside in the `mne-cpp/bin` folder.

The no longer needed folder `mne-cpp/bin/mne-cpp-test-data`is deleted before packaging. The folder `mne-cpp/bin` is compressed to a .zip file and uploaded as release asset to the corresponding GitHub release.

### Linux
The `LinuxDynamic` workflow run in [release.yml](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/release.yml){:target="_blank" rel="noopener"} is configured to exclude examples as well as tests. The `RPATH` is specified in the executable's .pro file in order to link to the libraries in `mne-cpp/lib`.

Dependency solving for libraries and executables is done via the [linuxdeployqt](https://github.com/probonopd/linuxdeployqt){:target="_blank" rel="noopener"} tool. Calling `linuxdeployqt` is performed in the [deploy_linux](https://github.com/mne-tools/mne-cpp/blob/master/tools/deployment/deploy_linux){:target="_blank" rel="noopener"} file.

The no longer needed folder `mne-cpp/bin/mne-cpp-test-data`is deleted before packaging.  The folders `mne-cpp/bin`, `mne-cpp/lib`, `mne-cpp/plugins`, `mne-cpp/translations` are compressed to a .tar.gz file and uploaded as release assets to the corresponding GitHub release. 

### MacOS
The `MacOSDynamic` workflow run in [release.yml](https://github.com/mne-tools/mne-cpp/blob/master/.github/workflows/release.yml){:target="_blank" rel="noopener"} is configured to exclude examples as well as tests. All applications are build as .app bundles using the `withAppBundles` configuration flag.

Dependency solving for libraries and executables is done via the `macdeployqt` tool, which is officially developed and maintained by Qt. Calling `macdeployqt` is performed in the [deploy_macos](https://github.com/mne-tools/mne-cpp/blob/master/tools/deployment/deploy_macos){:target="_blank" rel="noopener"} file. All MNE-CPP libraries are manually copied to the .app's `Contents/Frameworks` folder. The resource and applications' plugins folders are copied to the corresponding `mne-cpp/bin/<app_name>.app/` folders. 

The no longer needed folders `mne-cpp/bin/mne-cpp-test-data`, `mne-cpp/bin/mne_scan_plugins`, `mne-cpp/bin/mne_analyze_plugins`, `mne-cpp/bin/mne_rt_server_plugins` and `mne-cpp/bin/resources` are deleted before packaging. The folder `mne-cpp/bin` is compressed to a .tar.gz file and uploaded as release assets to the corresponding GitHub release.

## Static Builds

The information above corresponds to deploying the dynamically build version of MNE-CPP. In case of static deployment the process differs a bit. See the [deploy_win_static.bat](https://github.com/mne-tools/mne-cpp/blob/master/tools/deployment/deploy_win_static.bat){:target="_blank" rel="noopener"} , [deploy_macos_static](https://github.com/mne-tools/mne-cpp/blob/master/tools/deployment/deploy_macos_static){:target="_blank" rel="noopener"} and [deploy_linux_static](https://github.com/mne-tools/mne-cpp/blob/master/tools/deployment/deploy_linux_static){:target="_blank" rel="noopener"} files.