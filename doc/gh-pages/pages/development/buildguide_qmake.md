---
title: Building with qmake 
parent: Build from Source
grand_parent: Development
nav_order: 2
---
# Building with qmake

These instructions are here for the sake of helping those who are still using older versions of the project. If possible, consider using the most current version built with CMake.

## Get a Compiler

Make sure you have one of the following compilers installed:

| Windows | Linux | MacOS |
|---------|-------|-------|
| min. MSVC 2015 (We recommend the [MSVC 2019 Community Version](https://visualstudio.microsoft.com/vs/older-downloads/){:target="_blank" rel="noopener"} compiler. During install exclude everything except for VC++, Win 10 SDK and ATL support) | min. [GCC 5.3.1](https://gcc.gnu.org/releases.html){:target="_blank" rel="noopener"} | min. [Clang 3.5](https://developer.apple.com/xcode/){:target="_blank" rel="noopener"}|


## Build from Source (Dynamic Linking)

### Get Qt

#### Download the Qt installer

Qt is the only dependency you will need to install. Go to the Qt download section and download the [Qt installer](https://www.qt.io/download-qt-installer?hsCtaTracking=9f6a2170-a938-42df-a8e2-a9f0b1d6cdce%7C6cb0de4f-9bb5-4778-ab02-bfb62735f3e5){:target="_blank" rel="noopener"}.

#### Install Qt 5

Please note that Qt 5.10.0 or higher is needed in order to have full Qt3D support. Qt 6 is not yet supported. Install the Qt version with the minimum of the following features (uncheck all other boxes) to a path without white spaces:

- Qt/5.15.1/MSVC 2019 64-bit (Make sure to select the correct version based on your compiler)
- Qt/5.15.1/QtCharts

After the installation is finished make sure to add the Qt bin folder (e.g. `<QtFolder>\5.15.1\msvc2019_64\bin`) to your `PATH` variable. On Linux and MacOS you might also need to add the Qt lib folder (e.g. `<QtFolder>\5.15.1\msvc2019_64\lib`) to the `LD_LIBRARY_PATH` and `DYLD_LIBRARY_PATH`, respectivley.

### Get Project Source Code

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

### Compile the Source Code

#### Via QtCreator (recommended)

| **Please note:** If you are working on an operating system on a "non-western" system, i.e. Japan, you might encounter problems with unicode interpretation. Please do the  following: Go to Control Panel > Language and Region > Management tab > Language Settings for non-Unicode Programs > Set to English (U.S.) > Reboot your system. |

1. Go to your cloned repository folder and run the `mne-cpp.pro` file with QtCreator.
2. The first time you open the mne-cpp.pro file you will be prompted to configure the project with a pre-defined kit. Select the appropriate kit, e.g., `Desktop Qt 5.15.1 MSVC2019 64bit` and configure the project.
3. In QtCreator select the Release mode in the lower left corner.
4. In the Qt Creator's Projects window, right mouse click on the top level MNE-CPP tree item and select Run qmake. Wait until progress bar in lower right corner turns green (this step may take some time).
5. Right mouse click again and then hit Build (this step may take some time). Wait until progress bar in lower right corner turns green.
6. After the build process is finished, go to the `mne-cpp/bin` folder. All applications and libraries should have been created throughout the build process.

For building on CentOS and other linux distros with older gcc versions, check if RHEL Developer Toolsets are available with newer versions. In Qt Creator, under `Tools > Options > Kits > Compilers`, you can add a new compiler, and then add it to your build kit in `Tools > Options > Kits > Kits`.

#### Via Command Line

Create a shadow build folder, run `qmake` on `mne-cpp.pro` and build:

```
mkdir mne-cpp_shadow
cd mne-cpp_shadow
<QtFolder>/5.15.1/msvc2019_64/bin/qmake ../mne-cpp/mne-cpp.pro
<QtFolder>/5.15.1/msvc2019_64/bin/jom -j8 # On Windows
make -j8 # On Linux and MacOS
```

#### Running Applications

Once built, applications can be run from within QtCreator using the run button on the bottom left side. To instead run applications from the command line, first execute the ```deploy.bat``` script in ```tools/deployment```.  For more information about this script, see our [Continuous Integration documentation](ci_deployment.md). Once this is done, applications can be executed normally from the command line.

### Test the Build

You might have to add the folders including the Qt libraries to your OS's corresponding environment variables. In order to run the examples you must download the MNE-Sample-Data-Set from [here](https://osf.io/86qa2/download){:target="_blank" rel="noopener"} and extract the files to `mne-cpp/bin/MNE-sample-data`. Once finished you can try to run one of the examples, e.g., ex_disp_3D. If the build was successful the example should start and display a window including a 3D brain as well as a source localization result.


## Build from Source (Static Linking)

This tutorial will show you how to build a static version of MNE-CPP. In order to build statically we need to perform two steps:

 * Build a static version of Qt
 * Compile MNE-CPP with the `static` flag

This tutorial assumes the following folder structure:
```
Git/
├── qt5/
├── qt5_shadow/
├── qt5_wasm_binaries/
├── mne-cpp/
└── mne-cpp_shadow/
```
### Build a Static Version of Qt

#### Get the Qt Source Code

Clone the current Qt 5 version. Qt 6 is currently not supported. Currently, MNE-CPP uses four Qt modules: QtBase, QtCharts, QtSvg and Qt3D. QtBase subdivides in other modules reflecting most of the Qt functionality (core, gui, widgets, etc). In order to setup the sources for Qt 5.15.1 type:

```
git clone https://code.qt.io/qt/qt5.git -b 5.15.1  
cd qt5
```

#### Linux/MacOS

Install OpenGL dependencies (just to make sure). This is only needed on Linux:

```
sudo apt-get install build-essential libgl1-mesa-dev
```

Navigate to the `qt5` folder and init the repository:

```
./init-repository -f --module-subset=qtbase,qtcharts,qtsvg,qt3d
```

Navigate to the parent directory, create a new shadow build folder and cd into it:

```
cd ..
mkdir qt5_shadow
cd qt5_shadow
```

Call configure from the new working directory in order to setup a shadow build:

```
../qt5/configure -static -release -skip webengine -nomake tools -nomake tests -nomake examples -no-dbus -no-ssl -no-pch -opensource -confirm-license -prefix "../qt5_wasm_binaries"
```

Build Qt and install to target (prefix) location afterwards. You can change the `-j8` flag to the number of cores you want to use during compilation:

```
make module-qtbase module-qtsvg module-qtcharts module-qt3d -j8
make install -j8
```

A static Qt version should now be setup in the `qt5_wasm_binaries` folder.

#### Windows 10

Setup the following dependencies:

* Install [Perl](https://www.activestate.com/products/perl/downloads/){:target="_blank" rel="noopener"} and add it to `PATH`
* Install [Python](https://www.python.org/downloads/){:target="_blank" rel="noopener"} and add it to `PATH`
* Install MSVC 2015 or higher (We recommend the [MSVC 2019 Community Version](https://visualstudio.microsoft.com/vs/older-downloads/){:target="_blank" rel="noopener"})
* Install the [Windows 10 SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk/){:target="_blank" rel="noopener"} (can also be installed via the MSVC community edition installer)
* If you want to use multiple cores (MSVC's `nmake` does not support multicore usage), install the [jom compiler](http://download.qt.io/official_releases/jom/jom.zip){:target="_blank" rel="noopener"} and add it to `PATH`

Navigate to the `qt5` folder and init the repository:

```
perl init-repository -f --module-subset=qtbase,qtcharts,qtsvg,qt3d
```

Navigate to the parent directory, create a new shadow build folder and cd into it:

```
cd ..
mkdir qt5_shadow
cd qt5_shadow
```

Setup the visual studio compiler by starting the `VS2019 x64 Native Tools Command Prompt` or by typing (assuming you are using MSVC 2017):

```
cmd.exe /c "call `"C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat`" && set > %temp%\vcvars.txt"
Get-Content "$env:temp\vcvars.txt" | Foreach-Object { if ($_ -match "^(.*?)=(.*)$") { Set-Content "env:\$($matches[1])" $matches[2] } }
```

Call configure from the new working directory in order to setup a shadow build.

```
../qt5/configure.bat -release -static -no-pch -optimize-size -opengl desktop -platform win32-msvc -skip webengine -nomake tools -nomake tests -nomake examples -opensource -confirm-license -prefix "..\qt5_wasm_binaries"
```

Build Qt and install to target (prefix) location afterwards. You can change the `-j8` flag to the number of cores you want to use during compilation:

```
jom -j8
nmake install
```

A static Qt version should now be setup in the `qt5_wasm_binaries` folder.

## Compile MNE-CPP with the `static` flag

Create a shadow build folder, run `qmake` and build MNE-CPP (on Windows use `nmake` or `jom` instead of `make`):

```
mkdir mne-cpp_shadow
cd mne-cpp_shadow
../qt5_wasm_binaries/bin/qmake ../mne-cpp/mne-cpp.pro MNECPP_CONFIG += static
make -j8
```

All MNE-CPP applications (MNE Scan, examples, tests, etc.) should now be in the `mne-cpp/bin` folder.
