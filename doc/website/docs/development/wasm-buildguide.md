---
title: Build Guide
sidebar_label: Build Guide
---

# WebAssembly Build Guide

This guide shows you how to build MNE-CPP for WebAssembly (Wasm) using CMake and the Qt-provided Emscripten toolchain.

## Prerequisites

1. **Emscripten SDK (emsdk)** — Install from [emscripten.org](https://emscripten.org/docs/getting_started/downloads.html). Make sure to activate and source the version required by your Qt Wasm build.
2. **Qt for WebAssembly** — Install the Wasm target via the Qt Online Installer (e.g., `Qt 6.x → WebAssembly (single-threaded)` or `WebAssembly (multi-threaded)`). Qt ships a pre-configured CMake toolchain file for Emscripten.
3. **CMake** (3.21 or later) and **Ninja** (recommended generator).

## Activate the Emscripten Environment

Before building, activate the emsdk environment in your terminal:

```bash
cd /path/to/emsdk
./emsdk activate <version>    # use the version matching your Qt Wasm build
source ./emsdk_env.sh
```

## Configure and Build

Create a build directory and configure with CMake, pointing to the Qt Wasm toolchain:

```bash
mkdir build_wasm && cd build_wasm

cmake .. \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/Qt/6.x.x/wasm_singlethread/lib/cmake/Qt6/qt.toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release

ninja
```

Replace `/path/to/Qt/6.x.x/wasm_singlethread` with the actual path to your Qt Wasm installation. For multi-threaded builds, use the `wasm_multithread` target instead.

## Run an Application

Navigate to the build output directory and start a local web server:

```bash
python3 -m http.server
```

Open a Chromium-based browser or Firefox and visit:

```
http://localhost:8000/mne_analyze.html
```

## Example Builds

Live example builds can be found at:

[https://mne-cpp.github.io/wasm/mne_analyze.html](https://mne-cpp.github.io/wasm/mne_analyze.html)

## Notes

- Chromium-based browsers and Firefox provide the best compatibility.
- Multi-threaded Wasm builds require the server to send the `Cross-Origin-Opener-Policy` and `Cross-Origin-Embedder-Policy` headers. See the [Qt for WebAssembly documentation](https://doc.qt.io/qt-6/wasm.html) for details.
- Qt3D is not supported in WebAssembly builds.

## References

- [Qt for WebAssembly](https://doc.qt.io/qt-6/wasm.html)
- [Emscripten documentation](https://emscripten.org/docs/)
- [Qt for WebAssembly wiki](https://wiki.qt.io/Qt_for_WebAssembly)
