---
title: Static Build Guide
parent: Develop
nav_order: 4
---
# Static Build Guide

This tutorial will show you how to build a static version of MNE-CPP. In order to build statically we need to do two things:

 * Build a static version of Qt
 * Compile MNE-CPP with the `static` flag

This tutorial assumes the following folder structure:
```
Git/
├── qt5/
├── qt5_shadow/
├── qt5_binaries/
├── mne-cpp/
└── mne-cpp_shadow/
```

## Build a static version of Qt

### Linux/MacOS

* Install OpenGL dependencies (just to make sure). This is only needed on Linux:

    ```
    sudo apt-get install build-essential libgl1-mesa-dev
    ```

* Clone current Qt version. Currently, MNE-CPP uses four Qt modules: QtBase, QtCharts, QtSvg and Qt3D. QtBase includes most of the Qt functionality (core, gui, widgets, etc). In order to setup the sources for Qt 5.14.2 you would type:

    ```
    git clone https://code.qt.io/qt/qt5.git -b 5.14.2  
    cd qt5
    ./init-repository -f --module-subset=qtbase,qtcharts,qtsvg,qt3d
    ```

* Navigate to parent directory, create new shadow build folder and cd into it:

    ```
    cd ..
    mkdir qt5_shadow
    cd qt5_shadow
    ```

* Call configure from new working directory in order to perform a shadow build.

    ```
    ../qt5/configure -static -release -prefix "../qt5_binaries" -skip webengine -nomake tools -nomake tests -nomake examples -no-dbus -no-ssl -no-pch -opensource -confirm-license
    ```

* Build Qt and install to target (prefix) location afterwards. You can change the `-j8` flag to the number of cores you want to use during compilation:

    ```
    make module-qtbase module-qtsvg module-qtcharts module-qt3d -j8
    make install -j8
    ```

* A static Qt version should now be setup in the `qt5_binaries` folder.

### Windows

It's complicated. A guide on how to build Qt statically on Windows wil follow soon.

## Compile MNE-CPP with the static flag

* Create a shadow build folder, run qmake and build MNE-CPP (on Windows use `nmake` or `jom` instead of `make`):

    ```
    mkdir mne-cpp_shadow
    cd mne-cpp_shadow
    ../qt5_binaries/bin/qmake ../mne-cpp/mne-cpp.pro MNECPP_CONFIG += static
    make -j8
    ```

* All MNE-CPP applications (MNE Scan, examples, tests, etc.) should now be in the `mne-cpp/bin` folder.
