---
title: MNE-CPP with QtWasm
parent: Develop
nav_order: 5
---
# MNE-CPP with QtWasm

## Setup Qt Wasm to work with MNE-CPP on Ubuntu 18.04.03 64bit

 * According to the official Qt Wasm (WebAssembly) guide, the preferred emscripten versions are:

   Qt 5.12: 1.38.16

   Qt 5.13: 1.38.27 (multithreading: 1.38.30)

   Qt 5.14: it's complicated (1.38.27)

 | **Please note:** With the versions above some functions are not able to be linked and produce errors. It is possible that some MNE-CPP functions are not compatible with these emscripten versions. However, emscripten version 1.39.3 seems to be working with MNE-CPP code. The following two setups should work: **Qt5.13.2 compiled with em++ 1.39.3 with thread support** and **Qt5.14.0 compiled with em++ 1.39.3 with thread support**. | 

* Get the [emscripten](https://emscripten.org/) compiler:

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

 * Build Qt from source. This is needed since we want to have threading support which is deactivated for the pre-built QtWasm build. Also, the pre-built QtWasm binaries are build with emscripten version 1.38.30 which does not work with MNE-CPP code.

    * Make sure to activate and source the correct emscripten version since the compiler will be used when building qt against wasm. You could also add this to your .basrc file. For example:

      ```
      ./emsdk activate 1.39.3
      source ./emsdk_env.sh
      ```

    * Install some dependencies (just to make sure)

      ```
      sudo apt-get install build-essential libgl1-mesa-dev python
      ```

    * Clone current Qt version. For example Qt 5.14.0:

      ```
      git clone -b 5.14.0
      https://code.qt.io/qt/qt5.git
      cd qt5
      ./init-repository -f --module-subset=qtbase,qtcharts,qtsvg
      ```

    * Navigate to parent directory, create new shadow build folder and cd into it:

      ```
      cd ..
      mkdir qt5_shadow
      cd qt5_shadow
      ```

    * Call configure from new working directory in order to perform a shadow build.

      With thread support (somehow this prevented quick and qml to be build, which are needed for mne_launch for example):

      ```
      ../qt5/configure -opensource -confirm-license -xplatform wasm-emscripten -feature-thread -nomake examples -no-dbus -no-ssl -prefix /home/lorenz/Qt/5.14.0/wasm_em1393_64_withThread
      ```

      Without thread support:

      ```
      ../qt5/configure -opensource -confirm-license -xplatform wasm-emscripten -nomake examples -no-dbus -no-ssl -prefix /home/lorenz/Qt/5.14.0/wasm_em1393_64_withThread
      ```

    * Build Qt and install to target (prefix) location afterwards. For mne-cpp we only need the qt charts, qtsvg and qtbase module (see [https://wiki.qt.io/Qt_for_WebAssembly](https://wiki.qt.io/Qt_for_WebAssembly) for officially supported modules):

      ```
      make module-qtbase module-qtsvg module-qtcharts -j8
      make install -j8
      ```

    * Qt Wasm should now be setup to work with the activated emscripten version

## Building MNE-CPP against QtWasm

 * Navigate to mne-cpp/mne-cpp.pri and add the wasm flag. This will build mne-cpp statically and configure only wasm supported MNE-CPP code.

   ```
   MNECPP_CONFIG += wasm
   ```

 * Create a shadow build folder, run qmake and build mne-cpp:

   ```
   mkdir mne-cpp-shadow
   cd mne-cpp-shadow
   /home/lorenz/Qt/5.14.0/wasm_em1393_64_withThread/bin/qmake
   ../mne-cpp/mne-cpp.pro
   make -j8
   ```

 * This should create the applications featured under applications, e.g. MNE Browse, to be build to mne-cpp/bin

## Running an application

 * Navigate to mne-cpp bin and start a server

   ```
   python3 -m http.server
   ```

 * Go to a suitable web browser (Chromium based and Mozilla browsers seem to work the best) and type:

   ```
   http://localhost:8000/mne_browse.html
   ```

## Example builds

Example builds can be found here (Chromium based and Mozilla browsers seem to work the best):

  [https://mne-cpp.github.io/wasm/mne_browse.html](https://mne-cpp.github.io/wasm/mne_browse.html)

  [https://mne-cpp.github.io/wasm/mne_analyze.html](https://mne-cpp.github.io/wasm/mne_browse.html)

## General notes and helpful information

 * CentOS7 at Martinos does not seem to have a high enough gcc version
     installed.

 * QtWebAssembly does not support concurrent and Qt3D modules.
     [https://forum.qt.io/topic/109166/wasm-support-for-qt3d/4](https://www.qt.io/blog/2019/06/26/qt-webassembly-multithreading)

 * MNE-CPP Code needs to be build statically

 * Thread support forQtWasm can be achieved since Qt5.13. Thread support is deactivated in pre-built binaries. The default Qt for WebAssembly build disables threads by default. To enable, build from source and configure with the **-feature-thread** flag. We’ve found that emscripten 1.38.30 works well for threaded builds. From [https://www.qt.io/blog/2019/06/26/qt-webassembly-multithreading](https://www.qt.io/blog/2019/06/26/qt-webassembly-multithreading)

 * Data access for local files:
     [https://forum.qt.io/topic/104608/access-local-user-file-on-qt-for-web-assembly](https://forum.qt.io/topic/104608/access-local-user-file-on-qt-for-web-assembly) and
     [https://stackoverflow.com/questions/56886410/access-local-user-file-on-qt-for-web-assembly](https://stackoverflow.com/questions/56886410/access-local-user-file-on-qt-for-web-assembly)

 * Emscripten installer only supports 64 bit versions. 32bit versions need to be build from scratch.

 * If you get an idbc error see [https://bugreports.qt.io/browse/QTBUG-79872](https://bugreports.qt.io/browse/QTBUG-79872).

 * For browser support information see [https://caniuse.com/#feat=wasm](https://caniuse.com/#feat=wasm).

## Wasm references

[https://www.qt.io/blog/2018/11/19/getting-started-qt-webassembly](https://www.qt.io/blog/2018/11/19/getting-started-qt-webassembly)

[https://doc.qt.io/qt-5/wasm.html](https://doc.qt.io/qt-5/wasm.html)

[https://medium.com/@jimmychen009/qt-quick-on-the-browser-30d5349c11ec](https://medium.com/@jimmychen009/qt-quick-on-the-browser-30d5349c11ec)

[https://dev.to/captainsafia/why-the-heck-is-everyone-talking-about-webassembly-455a](https://dev.to/captainsafia/why-the-heck-is-everyone-talking-about-webassembly-455a)

[https://wiki.qt.io/Qt_for_WebAssembly](https://wiki.qt.io/Qt_for_WebAssembly)

[https://doc.qt.io/qt-5/qtwebassembly-platform-notes.html](https://doc.qt.io/qt-5/qtwebassembly-platform-notes.html)

[https://www.qt.io/blog/2019/06/26/qt-webassembly-multithreading](https://www.qt.io/blog/2019/06/26/qt-webassembly-multithreading)

[https://forum.qt.io/topic/107689/building-qt-apps-to-wasm/18](https://forum.qt.io/topic/107689/building-qt-apps-to-wasm/18)

[https://doc-snapshots.qt.io/qtcreator-master/creator-setup-webassembly.html](https://doc-snapshots.qt.io/qtcreator-master/creator-setup-webassembly.html)

[https://github.com/msorvig/qt-webassembly-examples](https://github.com/msorvig/qt-webassembly-examples)
