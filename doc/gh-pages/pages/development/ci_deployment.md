---
title: Deployment
parent: Continuous Integration
grand_parent: Development
nav_order: 2
---

# Deployment

This page explains how MNE-CPP handles deployment on Win, Linux and MacOS.

## Build Rules

All MNE-CPP libraries are built to the `mne-cpp/lib` folder. All applications, test, and examples are built to the `mne-cpp/out/Release` folder.

## Dependency Solving
MNE-CPP depends on [Qt](https://www.qt.io/){:target="_blank" rel="noopener"} and [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page){:target="_blank" rel="noopener"}. Eigen, as a lightweight template library, [is included in the MNE-CPP repository by default](https://github.com/mne-tools/mne-cpp/tree/main/include/3rdParty/eigen3){:target="_blank" rel="noopener"} and does not need further dependency solving. For Qt dependencies we use Qt specific deployment tools (`windeployqt`, `linuxdeployqt`, `macdeployqt`).

### Windows
The `WinDynamic` workflow run in [release.yml](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/release.yml){:target="_blank" rel="noopener"} is configured to exclude examples as well as tests. Next to `mne-cpp/lib` the MNE-CPP libraries are also copied to the `mne-cpp/out/Release` folder. This is performed in the libraries' .pro files. 

Dependency solving for libraries and executables is done via the `windeployqt` tool, which is officially developed and maintained by Qt. Calling `windeployqt` is performed in the [deploy.bat](https://github.com/mne-tools/mne-cpp/blob/main/tools/deployment/deploy.bat){:target="_blank" rel="noopener"} file with `dynamic` as first argument. `windeployqt` is called on the MNE Scan .exe and the Disp3D .dll file. Disp3D is the most top level library and links against all needed Qt modules. MNE Scan links against all relevant Qt modules application wise. Subsequently, Qt and all needed system libraries reside in the `mne-cpp/out/Release` folder.

The no longer needed folder `resources/data/mne-cpp-test-data`is deleted before packaging. The folder `mne-cpp/out/Release` is compressed to a .zip file and uploaded as release asset to the corresponding GitHub release.

### Linux
The `LinuxDynamic` workflow run in [release.yml](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/release.yml){:target="_blank" rel="noopener"} is configured to exclude examples as well as tests. The `RPATH` is specified in the executable's .pro file in order to link to the libraries in `mne-cpp/lib`.

Dependency solving for libraries and executables is done via the [linuxdeployqt](https://github.com/probonopd/linuxdeployqt){:target="_blank" rel="noopener"} tool. Calling `linuxdeployqt` is performed in the [deploy.bat](https://github.com/mne-tools/mne-cpp/blob/main/tools/deployment/deploy.bat){:target="_blank" rel="noopener"} script (Remember that in unix/linux environments, the extension of a file has no special meaning, being just part of the filename).

The, no longer needed, folder `mne-cpp/resources/data/mne-cpp-test-data` is deleted before packaging.  The folders `mne-cpp/out/Release/apps`, `mne-cpp/out/Release/lib`, `mne-cpp/out/Release/plugins`, `mne-cpp/out/Release/translations` are compressed to a .tar.gz file and uploaded as release assets to the corresponding GitHub release. 

### MacOS
The `MacOSDynamic` workflow run in [release.yml](https://github.com/mne-tools/mne-cpp/blob/main/.github/workflows/release.yml){:target="_blank" rel="noopener"} is configured to exclude examples as well as tests. All applications are build as .app bundles using the `withAppBundles` configuration flag.

Dependency solving for libraries and executables is done via the `macdeployqt` tool, which is officially developed and maintained by Qt. Calling `macdeployqt` is performed in the [deploy_macos](https://github.com/mne-tools/mne-cpp/blob/main/tools/deployment/deploy.bat){:target="_blank" rel="noopener"} file. All MNE-CPP libraries are manually copied to the .app's `Contents/Frameworks` folder. The resource and applications' plugins folders are copied to the corresponding `mne-cpp/out/Release/apps/<app_name>.app/` folders. 

The, no longer needed, folders `mne-cpp/resources/data/mne-cpp-test-data`, `mne-cpp/bin/mne_scan_plugins`, `mne-cpp/bin/mne_analyze_plugins`, `mne-cpp/bin/mne_rt_server_plugins` and `mne-cpp/bin/resources` are deleted before packaging. The folder `mne-cpp/bin` is compressed to a .tar.gz file and uploaded as release assets to the corresponding GitHub release.

## Static Builds

The information above corresponds to deploying the dynamically build version of MNE-CPP. In case of static deployment the process differs a bit. See the [deploy.bat](https://github.com/mne-tools/mne-cpp/blob/main/tools/deployment/deploy.bat){:target="_blank" rel="noopener"} file.

## Deployment script `deploy.bat`
The script used by MNE-CPP to perform the deployment in the three platforms to which MNE-CPP is compiled is the same `deploy.bat` file, which is inside `tools/deployment` folder. The script will run in all platforms and generate the necesary libraries for the correct execution of any of the project's applications. This script can be run from anywhere in the file system, but do not change the script's location within the project's folder structure, since it depends on it for a correct execution. ```deploy.bat``` takes two input arguments: the first argument specifies the type of linkage to use in the project's applications. The second argument specifies if a compressed folder (zip, tar, ...) should be created with all the necessary files to run the project's application. 
The way this is achieved is by writing `deploy.bat` in a way in which different operating systems running the same script will actually execute different parts of it. If you take a look at its contents it is easy to identify the parts executed by each operating system because they are marked with comments. 

Independently of the type of linkage needed during the deployment, the script is just one script which depends on a couple of input arguments. A first input argument accepts either ```dynamic``` or ```static``` in order to decide which type of linkage to perform. If empty, the linkage option will be set to ```dynamic```. This script can be useful during development. Since it allows a developer to solve the dependencies a particular MNE-CPP application might have by providing the necessary libraries. For this reasons the script has a second input argument. If the second input argument is ```pack``` the script will generate the above mentioned zip/tar file with the binary executable files and libraries needed for a correct execution.