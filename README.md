<p align="center">
  <a href="/resources/design/logos/MNE-CPP_Logo.svg"><img src="/resources/design/logos/MNE-CPP_Logo.svg" width="400" height="200" alt="MNE-CPP"></a>
</p>

<p align="center">
  <a href="https://github.com/mne-tools/mne-cpp/releases/latest"><img src="https://img.shields.io/badge/version-2.2.1-blue.svg" alt="Version 2.2.1"></a>&nbsp;
  <a href="https://github.com/mne-tools/mne-cpp/actions/workflows/main.yml"><img src="https://github.com/mne-tools/mne-cpp/actions/workflows/main.yml/badge.svg?branch=main" alt="Release"></a>&nbsp;
  <a href="https://github.com/mne-tools/mne-cpp/actions/workflows/staging.yml"><img src="https://github.com/mne-tools/mne-cpp/actions/workflows/staging.yml/badge.svg?branch=staging" alt="Staging"></a>&nbsp;
  <a href="https://codecov.io/gh/mne-tools/mne-cpp/tree/staging"><img src="https://codecov.io/gh/mne-tools/mne-cpp/branch/staging/graph/badge.svg" alt="CodeCov"></a>
  <br>
  <a href="https://github.com/mne-tools/mne-cpp/actions/workflows/codeql.yml"><img src="https://github.com/mne-tools/mne-cpp/actions/workflows/codeql.yml/badge.svg" alt="CodeQL"></a>&nbsp;
  <a href="https://scan.coverity.com/projects/mne-tools-mne-cpp"><img src="https://scan.coverity.com/projects/8955/badge.svg" alt="Coverity"></a>&nbsp;
  <a href="https://gitter.im/mne-tools/mne-cpp?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge"><img src="https://badges.gitter.im/mne-tools/mne-cpp.svg" alt="Gitter"></a>
</p>

-----------------

MNE-CPP is an open-source, cross-platform C++ framework for real-time and offline processing of MEG, EEG, and related neurophysiological data. It provides modular libraries and ready-to-use applications — from data browsing and 3D visualization to real-time acquisition and source localization. For documentation please visit [mne-cpp.github.io](https://mne-cpp.github.io/).

Try it now
----------

No install required — runs entirely in your browser via WebAssembly. **Your data stays on your machine.**

| | Application | Description | Release | Nightly |
|---|---|---|---|---|
| <a href="https://mne-cpp.github.io/wasm/mne_browse.html"><img src="/src/applications/mne_browse/Resources/Images/ApplicationIcons/icon_browse_64x64.png" width="32" height="32" alt="MNE Browse"></a> | **MNE Browse** | Browse raw MEG/EEG data with filtering, events, averaging, ICA | [**Open**](https://mne-cpp.github.io/wasm/mne_browse.html) | [**Open**](https://mne-cpp.github.io/wasm/dev/mne_browse.html) |
| <a href="https://mne-cpp.github.io/wasm/mne_inspect.html"><img src="/src/applications/mne_inspect/resources/images/appIcons/icon_mne_inspect_64x64.png" width="32" height="32" alt="MNE Inspect"></a> | **MNE Inspect** | 3D visualization of brain surfaces, source estimates, and forward models | [**Open**](https://mne-cpp.github.io/wasm/mne_inspect.html) | [**Open**](https://mne-cpp.github.io/wasm/dev/mne_inspect.html) |

Applications
------------

| Application | Description |
|---|---|
| **MNE Browse** | Raw data browsing, filtering, event detection, averaging, ICA, and covariance computation |
| **MNE Inspect** | Interactive 3D visualization of brain surfaces, source estimates, and forward models |
| **MNE Scan** | Real-time acquisition and processing pipeline — MEGIN, BabyMEG, BrainAmp, eegosports, gUSBAmp, TMSI, Natus, LSL, FieldTrip Buffer |
| **MNE Analyze** | Sensor- and source-level analysis: browsing, filtering, averaging, co-registration, dipole fitting, source localization |
| **MNE Analyze Studio** | Agent-oriented analysis workbench with LLM-driven skill host, neuro kernel, and extension SDK |
| **MNE Dipole Fit** | Sequential equivalent current dipole fitting for focal brain activity |

50+ [command-line tools](https://mne-cpp.github.io/docs/manual/tools-overview) for BEM models, forward/inverse computation, data conversion, anonymization, and streaming — C++ ports of the original [MNE-C](http://www.nmr.mgh.harvard.edu/martinos/userInfo/data/MNE_register/index.php) utilities.

Libraries
---------

| Library | Description |
|---|---|
| **Fiff** | FIFF file I/O and data structures (raw, epochs, evoked, covariance, projections) |
| **Mne** | Core MNE data structures — source spaces, source estimates, hemispheres |
| **Fwd** | Forward modelling — BEM and MEG/EEG lead-field computation |
| **Inv** | Inverse estimation — MNE, dSPM, sLORETA, eLORETA, LCMV/DICS beamformers, RAP MUSIC, dipole fit, HPI |
| **Dsp** | Signal processing — FIR/IIR filtering, ICA, xDAWN, SSS/tSSS, Welch PSD, Morlet TFR, resampling, SPHARA |
| **Conn** | Connectivity — coherence, PLV, PLI, WPLI, cross-correlation, network analysis |
| **Disp3D** | 3D brain visualization (Metal / Vulkan / D3D / OpenGL via Qt RHI) |

All libraries depend only on [Qt](https://www.qt.io/) and [Eigen](http://eigen.tuxfamily.org/). See the [API documentation](https://mne-cpp.github.io/docs/development/api).

Development
-----------

### Build from source

```bash
# Linux / macOS
git clone --recursive https://github.com/mne-tools/mne-cpp.git && cd mne-cpp
./init.sh && cmake --build build/developer-dynamic --parallel
```

```bash
# Windows
git clone --recursive https://github.com/mne-tools/mne-cpp.git && cd mne-cpp
.\init.bat
cmake --build build\developer-dynamic --parallel
```

`init` downloads Qt and Eigen into `src/external/`, then configures CMake. Run `./init.sh --help` for all options (linkage, build type, custom Qt path, etc.).

### Requirements

[CMake](https://cmake.org/download/) ≥ 3.21 and a C++17 compiler:

| Platform | Compiler |
|----------|----------|
| Windows  | [MSVC](https://visualstudio.microsoft.com/vs/) 2022+ |
| Linux    | [GCC](https://gcc.gnu.org/releases.html) ≥ 13 |
| macOS    | [Xcode](https://developer.apple.com/xcode/) ≥ 16 |

For the full build guide see the [documentation](https://mne-cpp.github.io/docs/development/buildguide-cmake).

### Contributing

If you want to contribute to MNE-CPP you can find all the information you need [here](https://mne-cpp.github.io/docs/development/contribute).

Releases
--------

Pre-built binaries for Windows, macOS, and Linux are available on the [download page](https://mne-cpp.github.io/download).

Contact
-------

A list of contact persons can be found [here](https://mne-cpp.github.io/docs/overview#contact).

License
-------

MNE-CPP is licensed under the [BSD-3-Clause license](LICENSE).
