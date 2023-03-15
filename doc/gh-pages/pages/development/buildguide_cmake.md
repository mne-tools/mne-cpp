---
title: Building with CMake 
parent: Build from Source
grand_parent: Development
nav_order: 1
---
# Building with CMake

## Get a compiler

Make sure you have one of the following compilers installed:

| Windows | Linux | MacOS |
|---------|-------|-------|
| min. MSVC 2015 (We recommend the [MSVC 2019 Community Version](https://visualstudio.microsoft.com/vs/older-downloads/){:target="_blank" rel="noopener"} compiler. During install exclude everything except for VC++, Win 10 SDK and ATL support) | min. [GCC 5.3.1](https://gcc.gnu.org/releases.html){:target="_blank" rel="noopener"} | min. [Clang 3.5](https://developer.apple.com/xcode/){:target="_blank" rel="noopener"}|

## Get Qt

### Download the Qt installer

Qt is the only dependency you will need to install. Go to the Qt download section and download the [Qt installer](https://www.qt.io/download-qt-installer?hsCtaTracking=9f6a2170-a938-42df-a8e2-a9f0b1d6cdce%7C6cb0de4f-9bb5-4778-ab02-bfb62735f3e5){:target="_blank" rel="noopener"}.

### Install Qt 5

Please note that Qt 5.10.0 or higher is needed in order to have full Qt3D support. Qt 6 is not yet supported. Install the Qt version with the minimum of the following features (uncheck all other boxes) to a path without white spaces:

- Qt/5.15.1/MSVC 2019 64-bit (Make sure to select the correct version based on your compiler)
- Qt/5.15.1/QtCharts

After the installation is finished make sure to add the Qt bin folder (e.g. `<QtFolder>\5.15.1\msvc2019_64\bin`) to your `PATH` variable. On Linux and MacOS you might also need to add the Qt lib folder (e.g. `<QtFolder>\5.15.1\msvc2019_64\lib`) to the `LD_LIBRARY_PATH` and `DYLD_LIBRARY_PATH`, respectivley.

## Get the Source Code

Fork [MNE-CPP's main repository](https://github.com/mne-tools/mne-cpp){:target="_blank" rel="noopener"} to your own GitHub account. For a detailed guide on how to fork a repository, we recommend checking out the [guide on the official GitHub website](https://help.github.com/en/github/getting-started-with-github/fork-a-repo){:target="_blank" rel="noopener"}.

Clone the fork to your local machine:

```
git clone https://github.com/<YourGitUserName>/mne-cpp.git
```

Setup a new remote pointing to MNE-CPP's main repository:

```
git remote add upstream https://github.com/mne-tools/mne-cpp.git
```

Every time you want to update to the newest changes use:

```
git fetch --all
git rebase upstream/main
```

## Compile the Source Code

### Via QtCreator (recommended)

| **Please note:** If you are working on an operating system on a "non-western" system, i.e. Japan, you might encounter problems with unicode interpretation. Please do the  following: Go to Control Panel > Language and Region > Management tab > Language Settings for non-Unicode Programs > Set to English (U.S.) > Reboot your system. |

1. Open mne-cpp in QtCreator. To open the porject select the top CMakeLists.txt file in the `src` folder. 
2. The first time you open the project you will be prompted to configure the project with a pre-defined kit. Select the appropriate kit, e.g., `Desktop Qt 5.15.1 MSVC2019 64bit` and configure the project.
3. In QtCreator select the Release mode in the lower left corner.
4. In the Qt Creator's Projects window, right mouse click on the top level MNE-CPP tree item and select Run CMake. Wait until progress bar in lower right corner turns green (this step may take some time).
5. Right mouse click again and then hit Build (this step may take some time). Wait until progress bar in lower right corner turns green.
6. After the build process is finished, go to the `mne-cpp/bin` folder. All applications and libraries should have been created throughout the build process.

For building on CentOS and other linux distros with older gcc versions, check if RHEL Developer Toolsets are available with newer versions. In Qt Creator, under `Tools > Options > Kits > Compilers`, you can add a new compiler, and then add it to your build kit in `Tools > Options > Kits > Kits`.

### Via Command Line

Navigate to the mne-cpp base project folder.

We provide a cross-platform build script to automatically run CMake and build. This can be run with:
```
./tools/building/build_project.bat
```

Alternatively, you can manually run:
```
cmake -B build -src src -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The building can be parallelized by adding `--parallel n` to run *n* parallel builds when calling, ex. `cmake --build build --parallel 6`. This is greatly recommended as it speeds up building and is done for you in the provided build script based on the number of available cores.

### Running Applications

Once built, applications can be run from within QtCreator using the run button on the bottom left side. To instead run applications from the command line, navigate to `out/Release/apps` and run the program, ex. `./mne_scan`.

## Test the Build

You might have to add the folders including the Qt libraries to your OS's corresponding environment variables. In order to run the examples you must download the MNE-Sample-Data-Set from [here](https://osf.io/86qa2/download){:target="_blank" rel="noopener"} and extract the files to `mne-cpp/out/Release/resources/MNE-sample-data`. Once finished you can try to run one of the examples, e.g., ex_disp_3D. If the build was successful the example should start and display a window including a 3D brain as well as a source localization result.
