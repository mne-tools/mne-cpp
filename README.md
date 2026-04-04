<p align="center">
  <a href="/resources/design/logos/MNE-CPP_Logo.svg"><img src="/resources/design/logos/MNE-CPP_Logo.svg" width="400" height="200" alt="MNE-CPP"></a>
</p>
<p align="center">
<a href="https://github.com/mne-tools/mne-cpp/actions/workflows/main.yml" target="_blank">
    <img src="https://github.com/mne-tools/mne-cpp/actions/workflows/main.yml/badge.svg?branch=main" alt="Release">
</a>
<a href="https://github.com/mne-tools/mne-cpp/actions/workflows/staging.yml" target="_blank">
    <img src="https://github.com/mne-tools/mne-cpp/actions/workflows/staging.yml/badge.svg?branch=staging" alt="Staging">
</a>
<a href="https://codecov.io/gh/mne-tools/mne-cpp/tree/staging" target="_blank">
    <img src="https://codecov.io/gh/mne-tools/mne-cpp/branch/staging/graph/badge.svg" alt="CodeCov">
</a>
<a href="https://github.com/mne-tools/mne-cpp/actions/workflows/codeql.yml" target="_blank">
    <img src="https://github.com/mne-tools/mne-cpp/actions/workflows/codeql.yml/badge.svg" alt="CodeQL">
</a>
<a href="https://scan.coverity.com/projects/mne-tools-mne-cpp" target="_blank">
    <img src="https://scan.coverity.com/projects/8955/badge.svg" alt="Coverity">
</a>
<a href="https://gitter.im/mne-tools/mne-cpp?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge" target="_blank">
    <img src="https://badges.gitter.im/mne-tools/mne-cpp.svg" alt="Gitter">
</a>
</p>

-----------------

MNE-CPP is an open-source, cross-platform C++ framework for real-time and offline processing of MEG, EEG, and related neurophysiological data. It provides modular libraries for building standalone acquisition and analysis applications. For more information and documentation please visit https://mne-cpp.github.io/.

**Try it now** — [mne_browse](https://mne-cpp.github.io/wasm/mne_browse.html) runs entirely in the browser via WebAssembly (no install required).

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

| Platform | Compiler | Min version |
|----------|----------|-------------|
| Windows  | [MSVC](https://visualstudio.microsoft.com/vs/) | 2022 |
| Linux    | [GCC](https://gcc.gnu.org/releases.html) | 13 |
| macOS    | [Xcode (Apple Clang)](https://developer.apple.com/xcode/) | latest |

[Qt](https://www.qt.io/) 6.10 or later is required. The `init` script downloads a compatible version automatically.

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
