---
title: Build Guide
sidebar_label: Build Guide
---

# WebAssembly Build Guide

This guide shows you how to build MNE-CPP for WebAssembly (Wasm). MNE-CPP uses **multi-threaded** Wasm by default and provides a build script that handles Emscripten setup, CMake configuration, and compilation.

## Prerequisites

### Qt 6 for WebAssembly

You need a Qt 6 installation that includes the **WebAssembly (multi-threaded)** target. Install it via the [Qt Online Installer](https://www.qt.io/download-qt-installer) — select both a desktop target (e.g. `Desktop gcc 64-bit` on Linux or `macOS` on Mac) and the `WebAssembly (multi-threaded)` target. CI currently uses **Qt 6.10.2**.

A desktop (host) Qt is required alongside the Wasm Qt because Qt's cross-compilation needs a host toolchain to build code generators and other tools.

### Emscripten SDK

The build script can automatically download and set up the Emscripten SDK for you. If you prefer to install it manually, see [emscripten.org](https://emscripten.org/docs/getting_started/downloads.html). CI currently uses **emsdk 4.0.7**.

### System Dependencies

On **Linux**, install the following packages:

```bash
sudo apt-get install build-essential libgl1-mesa-dev ninja-build
```

On **macOS**, install Ninja:

```bash
brew install ninja
```

### CMake

[CMake](https://cmake.org/download/) **3.15 or higher** is required.

## Build with the Provided Script

MNE-CPP provides a Wasm build script at `tools/wasm.sh`. From the repository root:

```bash
# Set Qt6_DIR to the folder containing wasm_multithread/ and your host Qt (gcc_64/ or macos/)
export Qt6_DIR=$HOME/Qt/6.10.2

./tools/wasm.sh
```

The script will:
1. Clone and activate the Emscripten SDK (version 4.0.7 by default)
2. Auto-detect the host Qt path from `Qt6_DIR`
3. Run `qt-cmake` with `-DWASM=ON`
4. Build MNE-CPP for WebAssembly

Build output is placed in `out/wasm/`.

### Script Options

| Option | Description |
|--------|-------------|
| `--emsdk <version>`, `-e` | Set the Emscripten version (default: 4.0.7) |
| `--qt <path>`, `-q` | Set the Qt installation path manually |
| `--qt-host <path>` | Set the host Qt path for cross-compilation |
| `--no-log` | Skip writing `build_info.txt` |
| `--help`, `-h` | Show help |

Example with explicit paths:

```bash
./tools/wasm.sh --emsdk 4.0.7 --qt $HOME/Qt/6.10.2 --qt-host $HOME/Qt/6.10.2/gcc_64
```

## Build Manually

If you prefer to run the steps yourself:

```bash
# 1. Set up Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install 4.0.7
./emsdk activate 4.0.7
source ./emsdk_env.sh
cd ..

# 2. Configure and build
$HOME/Qt/6.10.2/wasm_multithread/bin/qt-cmake \
  -DQT_HOST_PATH=$HOME/Qt/6.10.2/gcc_64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DWASM=ON \
  -S src \
  -B build/wasm

cmake --build build/wasm --parallel $(nproc)
```

Adjust `QT_HOST_PATH` to `gcc_64` (Linux) or `macos` (macOS) as appropriate.

## Run an Application

Navigate to the build output directory and start a local web server:

```bash
cd out/wasm/apps
python3 -m http.server
```

Open a Chromium-based browser or Firefox and visit:

```
http://localhost:8000/mne_analyze.html
```

:::caution Cross-Origin Headers
Multi-threaded Wasm requires the server to send the `Cross-Origin-Opener-Policy: same-origin` and `Cross-Origin-Embedder-Policy: require-corp` headers. Python's simple HTTP server does **not** send these by default. For testing, use a tool like [user-agent-cors](https://www.npmjs.com/package/user-agent-cors) or configure your server accordingly. See the [Qt for WebAssembly documentation](https://doc.qt.io/qt-6/wasm.html) for details.
:::

## Live Demo

Live Wasm builds are available at:

[https://mne-cpp.github.io/wasm/mne_analyze.html](https://mne-cpp.github.io/wasm/mne_analyze.html)

## Notes

- Chromium-based browsers and Firefox provide the best compatibility.
- Qt3D / QOpenGLWidget is not supported in WebAssembly builds (`-DNO_OPENGL` is implied by `-DWASM=ON`).
- The Wasm build always uses **static linking** (`BUILD_SHARED_LIBS=OFF`).

## References

- [Qt for WebAssembly](https://doc.qt.io/qt-6/wasm.html)
- [Emscripten documentation](https://emscripten.org/docs/)
- [Qt for WebAssembly wiki](https://wiki.qt.io/Qt_for_WebAssembly)
