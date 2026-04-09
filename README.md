<p align="center">
  <a href="/resources/design/logos/MNE-CPP_Logo.svg"><img src="/resources/design/logos/MNE-CPP_Logo.svg" width="400" height="200" alt="MNE-CPP"></a>
</p>

<p align="center">
  <a href="https://github.com/mne-tools/mne-cpp/releases/latest"><img src="https://img.shields.io/badge/version-2.1.0-blue.svg" alt="Version 2.1.0"></a>&nbsp;
  <a href="https://github.com/mne-tools/mne-cpp/actions/workflows/main.yml"><img src="https://github.com/mne-tools/mne-cpp/actions/workflows/main.yml/badge.svg?branch=main" alt="Release"></a>&nbsp;
  <a href="https://github.com/mne-tools/mne-cpp/actions/workflows/staging.yml"><img src="https://github.com/mne-tools/mne-cpp/actions/workflows/staging.yml/badge.svg?branch=staging" alt="Staging"></a>&nbsp;
  <a href="https://codecov.io/gh/mne-tools/mne-cpp/tree/staging"><img src="https://codecov.io/gh/mne-tools/mne-cpp/branch/staging/graph/badge.svg" alt="CodeCov"></a>
  <br>
  <a href="https://github.com/mne-tools/mne-cpp/actions/workflows/codeql.yml"><img src="https://github.com/mne-tools/mne-cpp/actions/workflows/codeql.yml/badge.svg" alt="CodeQL"></a>&nbsp;
  <a href="https://scan.coverity.com/projects/mne-tools-mne-cpp"><img src="https://scan.coverity.com/projects/8955/badge.svg" alt="Coverity"></a>&nbsp;
  <a href="https://gitter.im/mne-tools/mne-cpp?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge"><img src="https://badges.gitter.im/mne-tools/mne-cpp.svg" alt="Gitter"></a>
</p>

-----------------

MNE-CPP is an open-source, cross-platform C++ framework for real-time and offline processing of MEG, EEG, and related neurophysiological data. It provides modular libraries for building standalone acquisition and analysis applications. For more information and documentation please visit https://mne-cpp.github.io/.

Applications
------------

### Try in the browser (WebAssembly)

No install required — runs entirely in your browser:

| | Application | Description | |
|---|---|---|---|
| <a href="https://mne-cpp.github.io/wasm/mne_browse.html"><img src="/src/applications/mne_browse/Resources/Images/ApplicationIcons/icon_browse_64x64.png" width="32" height="32" alt="MNE Browse"></a> | **MNE Browse** | Browse and inspect raw MEG/EEG data with real-time filtering, event detection, and averaging | [**Open**](https://mne-cpp.github.io/wasm/mne_browse.html) |
| <a href="https://mne-cpp.github.io/wasm/mne_inspect.html"><img src="/src/applications/mne_inspect/resources/images/appIcons/icon_mne_inspect_64x64.png" width="32" height="32" alt="MNE Inspect"></a> | **MNE Inspect** | Interactive 3D visualization of brain surfaces, source estimates, and forward models | [**Open**](https://mne-cpp.github.io/wasm/mne_inspect.html) |

### Desktop applications

| Application | Description |
|---|---|
| **MNE Scan** | Real-time acquisition and processing of MEG/EEG data with a plugin pipeline. Supports MEGIN, BabyMEG, BrainAmp, eegosports, gUSBAmp, TMSI, Natus, LSL, and FieldTrip Buffer. |
| **MNE Analyze** | Sensor- and source-level analysis GUI: raw data browsing, filtering, averaging, co-registration, dipole fitting, and source localization. |
| **MNE Analyze Studio** | Agent-oriented analysis workbench with an LLM-driven skill host, neuro kernel, and extension SDK for building composable analysis workflows. |
| **MNE Dipole Fit** | Sequential equivalent current dipole fitting for localising focal brain activity. |

### Command-line tools

60+ CLI tools for BEM model creation, forward/inverse computation, data conversion, anonymization, and real-time streaming — all C++ ports of the original [MNE-C](http://www.nmr.mgh.harvard.edu/martinos/userInfo/data/MNE_register/index.php) utilities. See the [tools reference](https://mne-cpp.github.io/docs/manual/tools-overview).

Libraries
---------

| Library | Description |
|---|---|
| **Fiff** | FIFF file I/O and data structures (raw, epochs, evoked, covariance, projections) |
| **Mne** | Core MNE data structures — source spaces, source estimates, hemispheres |
| **Fwd** | Forward modelling — BEM and MEG/EEG lead-field computation |
| **Inv** | Inverse source estimation — MNE, dSPM, sLORETA, eLORETA, LCMV/DICS beamformers, RAP MUSIC, dipole fit, HPI |
| **Dsp** | Digital signal processing — FIR/IIR filtering, ICA, xDAWN, SSS/tSSS, Welch PSD, Morlet TFR, spectrogram, resampling, bad-channel detection, SPHARA |
| **Conn** | Functional connectivity — coherence, PLV, PLI, WPLI, cross-correlation, and network analysis |
| **Disp3D** | 3D brain visualization (Metal/Vulkan/D3D/OpenGL via Qt RHI) |

All libraries depend only on [Qt](https://www.qt.io/) and [Eigen](http://eigen.tuxfamily.org/). See the [Library API documentation](https://mne-cpp.github.io/docs/development/api).

Quick start
-----------

```bash
# Clone and build (Linux / macOS)
git clone --recursive https://github.com/mne-tools/mne-cpp.git && cd mne-cpp
./init.sh && cmake --build build/developer-dynamic --parallel
```

```bash
# Clone and build (Windows)
git clone --recursive https://github.com/mne-tools/mne-cpp.git && cd mne-cpp
.\init.bat
cmake --build build\developer-dynamic --parallel
```

`init` downloads Qt and Eigen into `src/external/`, then runs CMake configure. Run `./init.sh --help` to see all options (linkage, build type, custom Qt path, etc.).

Requirements
------------

[CMake](https://cmake.org/download/) ≥ 3.21 and a C++17 compiler:

| Platform | Compiler |
|----------|----------|
| Windows  | [MSVC](https://visualstudio.microsoft.com/vs/) 2022 or later |
| Linux    | [GCC](https://gcc.gnu.org/releases.html) ≥ 13 |
| macOS    | [Xcode](https://developer.apple.com/xcode/) ≥ 16 |

Qt and Eigen are downloaded automatically by the `init` script.

For the full build guide see the [documentation](https://mne-cpp.github.io/docs/development/buildguide-cmake).

Releases
--------

Pre-built binaries for Windows, macOS, and Linux are available on the [download page](https://mne-cpp.github.io/download).

Get involved
------------

If you want to contribute to MNE-CPP you can find all necessary information [here](https://mne-cpp.github.io/docs/development/contribute).

Contact
-------

A list of contact persons can be found [here](https://mne-cpp.github.io/docs/overview#contact).

Citation
--------

If you use MNE-CPP in your work, please cite:

> Dinh, C., Esch, L., Mäki-Marttunen, V., Bott, F.-S., Lim, R., Yang, D., Drechsler, R., & Haueisen, J. (2021). **MNE-CPP: A cross-platform open-source C++ framework for real-time neurophysiology.** *Frontiers in Neuroscience*, 15, 663795. https://doi.org/10.3389/fnins.2021.663795

```bibtex
@article{dinh2021mnecpp,
  title     = {MNE-CPP: A cross-platform open-source C++ framework for real-time neurophysiology},
  author    = {Dinh, Christoph and Esch, Lorenz and M{\"a}ki-Marttunen, Vera and Bott, Faris-Sven and Lim, Robert and Yang, Daniel and Drechsler, Rolf and Haueisen, Jens},
  journal   = {Frontiers in Neuroscience},
  volume    = {15},
  pages     = {663795},
  year      = {2021},
  publisher = {Frontiers},
  doi       = {10.3389/fnins.2021.663795}
}
```

License
-------

MNE-CPP is **BSD-licensed** (3-clause):

    Copyright (c) 2011-2026, authors of MNE-CPP. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

    3. Neither the name of the copyright holder nor the names of its
       contributors may be used to endorse or promote products derived from
       this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
