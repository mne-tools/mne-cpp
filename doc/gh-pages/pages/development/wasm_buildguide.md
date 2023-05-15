---
title: Build Guide
parent: WebAssembly
grand_parent: Development
nav_order: 1
---
# WebAssembly Build Guide

This tutorial will show you how to build a Wasm (WebAssembly) version of MNE-CPP. In order to build a Wasm version we need to do three things:

 * Setup the emscripten compiler
 * Build a static wasm version of Qt (with thread support)
 * Compile MNE-CPP with the `wasm` flag

This tutorial runs on Ubuntu 18.04.03 64bit and assumes the following folder structure:
```
Git/
├── emsdk/
├── qt5/
├── qt5_shadow/
├── qt5_wasm_binaries/
├── mne-cpp/
└── mne-cpp_shadow/
```

## Basic Information on Qt Wasm with MNE-CPP 

According to the official [Qt Wasm guide](https://wiki.qt.io/Qt_for_WebAssembly){:target="_blank" rel="noopener"}, the preferred emscripten versions are:

```
Qt 5.12: 1.38.16
Qt 5.13: 1.38.27 (multithreading: 1.38.30)
Qt 5.14: it's complicated (1.38.27)
Qt 5.15: 1.39.7
```

 | **Please note:** With the versions above some functions are not able to be linked and produce errors. It is possible that some MNE-CPP functions are not compatible with these emscripten versions. However, emscripten version 1.39.3 and 1.39.7 seem to be working with MNE-CPP code. The following setups should work: **Qt5.13.2 compiled with em++ 1.39.3 with thread support**, **Qt5.14.2 compiled with em++ 1.39.3 with thread support** and  **Qt5.15.0 compiled with em++ 1.39.7 with thread support**. | 

## Setup the Emscripten Compiler

Get the [emscripten](https://emscripten.org/){:target="_blank" rel="noopener"} compiler:

```
# Get the emsdk repo
git clone https://github.com/emscripten-core/emsdk.git

# Enter that directory and pull
cd emsdk
git pull

# Download and install the latest SDK tools.
./emsdk install 1.39.3

# Make the "latest" SDK "active" for the current user. (writes ~/.emscripten file)
./emsdk activate 1.39.3

# Activate PATH and other environment variables in the current terminal
source ./emsdk_env.sh
```

## Build Qt from Source with Wasm Support

This is needed since we want to have threading support which is deactivated for the pre-built QtWasm build. Also, the pre-built QtWasm binaries are build with emscripten version 1.38.30 which does not work with MNE-CPP code.

Make sure to activate and source the correct emscripten version since the compiler will be used when building qt against wasm. You could also add this to your .basrc file. For example:

```
./emsdk activate 1.39.3
source ./emsdk_env.sh
```

Install some dependencies (just to make sure)

```
sudo apt-get install build-essential libgl1-mesa-dev python
```

Clone the current Qt version. For example Qt 5.14.2:

```
git clone https://code.qt.io/qt/qt5.git -b 5.14.2      
cd qt5
./init-repository -f --module-subset=qtbase,qtcharts,qtsvg
```

Navigate to the parent directory, create a new shadow build folder and cd into it:

```
cd ..
mkdir qt5_shadow
cd qt5_shadow
```

Call configure from the new working directory in order to setup a shadow build (remove the `-feature-thread` flag if you want to build without multithread support):

```
../qt5/configure -opensource -confirm-license -xplatform wasm-emscripten -feature-thread -nomake examples -no-dbus -no-ssl -prefix $PWD/../qt5_wasm_binaries
```

Build Qt and install to target (prefix) location afterwards. For MNE-CPP we only need the qt charts, qtsvg and qtbase module (see [https://wiki.qt.io/Qt_for_WebAssembly](https://wiki.qt.io/Qt_for_WebAssembly){:target="_blank" rel="noopener"} for officially supported modules):

```
make module-qtbase module-qtsvg module-qtcharts -j8
make install -j8
```

A static Qt Wasm version should now be setup in the `qt5_wasm_binaries` folder.

## Building MNE-CPP Against QtWasm

MNE-CPP needs to be build statically. This is automatically done if the `wasm` flag is set. Create a shadow build folder, run `qmake` and build MNE-CPP:

```
mkdir mne-cpp_shadow
cd mne-cpp_shadow
../qt5_wasm_binaries/resources/qmake ../mne-cpp/mne-cpp.pro MNECPP_CONFIG=wasm
make -j8
```

This should build all Wasm enabled applications, e.g. MNE Analyze, to `mne-cpp/out/Release`.

## Run an Application

Navigate to `mne-cpp/out/Release` and start a server:

```
python3 -m http.server
```

Start to a suitable web browser (Chromium based browsers and Mozilla seem to work the best) and type:

```
http://localhost:8000/mne_analyze.html
```

## Example Builds

Example builds can be found here (Chromium based and Mozilla browsers seem to work the best):

  [https://mne-cpp.github.io/wasm/mne_analyze.html](https://mne-cpp.github.io/wasm/mne_analyze.html){:target="_blank" rel="noopener"}

## General Notes and Helpful Information

 * CentOS7 at Martinos does not seem to have a high enough gcc version
     installed.

 * QtWebAssembly does not support concurrent and Qt3D modules.
     [https://forum.qt.io/topic/109166/wasm-support-for-qt3d/4](https://www.qt.io/blog/2019/06/26/qt-webassembly-multithreading){:target="_blank" rel="noopener"}

 * Thread support forQtWasm can be achieved since Qt5.13. Thread support is deactivated in pre-built binaries. The default Qt for WebAssembly build disables threads by default. To enable, build from source and configure with the `-feature-thread` flag. We’ve found that emscripten 1.38.30 works well for threaded builds. From [https://www.qt.io/blog/2019/06/26/qt-webassembly-multithreading](https://www.qt.io/blog/2019/06/26/qt-webassembly-multithreading){:target="_blank" rel="noopener"}

 * Data access for local files:
     [https://forum.qt.io/topic/104608/access-local-user-file-on-qt-for-web-assembly](https://forum.qt.io/topic/104608/access-local-user-file-on-qt-for-web-assembly){:target="_blank" rel="noopener"} and
     [https://stackoverflow.com/questions/56886410/access-local-user-file-on-qt-for-web-assembly](https://stackoverflow.com/questions/56886410/access-local-user-file-on-qt-for-web-assembly){:target="_blank" rel="noopener"}

 * Emscripten installer only supports 64 bit versions. 32bit versions need to be build from scratch.

 * If you get an idbc error see [https://bugreports.qt.io/browse/QTBUG-79872](https://bugreports.qt.io/browse/QTBUG-79872){:target="_blank" rel="noopener"}.

 * For browser support information see [https://caniuse.com/#feat=wasm](https://caniuse.com/#feat=wasm){:target="_blank" rel="noopener"}.

## Wasm References

[https://www.qt.io/blog/2018/11/19/getting-started-qt-webassembly](https://www.qt.io/blog/2018/11/19/getting-started-qt-webassembly){:target="_blank" rel="noopener"}

[https://doc.qt.io/qt-5/wasm.html](https://doc.qt.io/qt-5/wasm.html){:target="_blank" rel="noopener"}

[https://medium.com/@jimmychen009/qt-quick-on-the-browser-30d5349c11ec](https://medium.com/@jimmychen009/qt-quick-on-the-browser-30d5349c11ec){:target="_blank" rel="noopener"}

[https://dev.to/captainsafia/why-the-heck-is-everyone-talking-about-webassembly-455a](https://dev.to/captainsafia/why-the-heck-is-everyone-talking-about-webassembly-455a){:target="_blank" rel="noopener"}

[https://wiki.qt.io/Qt_for_WebAssembly](https://wiki.qt.io/Qt_for_WebAssembly){:target="_blank" rel="noopener"}

[https://doc.qt.io/qt-5/qtwebassembly-platform-notes.html](https://doc.qt.io/qt-5/qtwebassembly-platform-notes.html){:target="_blank" rel="noopener"}

[https://www.qt.io/blog/2019/06/26/qt-webassembly-multithreading](https://www.qt.io/blog/2019/06/26/qt-webassembly-multithreading){:target="_blank" rel="noopener"}

[https://forum.qt.io/topic/107689/building-qt-apps-to-wasm/18](https://forum.qt.io/topic/107689/building-qt-apps-to-wasm/18){:target="_blank" rel="noopener"}

[https://doc-snapshots.qt.io/qtcreator-master/creator-setup-webassembly.html](https://doc-snapshots.qt.io/qtcreator-master/creator-setup-webassembly.html){:target="_blank" rel="noopener"}

[https://github.com/msorvig/qt-webassembly-examples](https://github.com/msorvig/qt-webassembly-examples){:target="_blank" rel="noopener"}
