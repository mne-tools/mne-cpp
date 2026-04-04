# Changelog

All notable changes to MNE-CPP will be documented in this file.

## [2.1.0] - 2026-04-10

### New Features

- **MNE Browse overhaul**
  - QRhi-based 2D channel rendering for Metal/Vulkan/D3D12 performance
  - ICA browser with component visualization, rejection, and original-data reset
  - Epoch review workflow with evoked computation and histogram view
  - Annotation workflow with FIF interoperability and auto-merge
  - Virtual channel derivations with weighted reference workflows
  - Covariance matrix heatmap visualization and whitening controls (W key)
  - Projector dialog (J/Shift+J) and SSP toggle
  - 2D layout view with mouse-wheel zoom, auto-scale, and channel scaling spinboxes
  - Dark mode, drag-and-drop file open, recent files list
  - GPU clipping/z-score shaders, fullscreen (F11), zen mode (F)
  - Linear detrending (D key cycles None/DC/Linear)
  - GFP overlay on butterfly view
  - Interactive ruler tool with amplitude snap and semi-transparent overlay
  - Double-click bad-channel marking, click-to-toggle bad channels
  - Stimulus lane above time ruler with event markers
  - Per-description event/annotation filtering and visibility toggles
  - Channel sort by type, epoch grid lines (G), overview bar toggle (O), scroll speed control
  - Source-estimate export workflow
  - Session filtering workflow
  - Settings persistence across sessions via QSettings
  - Event file saving to FIF and ASCII formats
  - WASM loading screen and optimized WebAssembly build

- **MNE Analyze Studio**
  - Agent-based workflow engine with grounded LLM safety
  - Multi-provider planner profiles (OpenAI Responses API)
  - Reusable inspect 3D surface views
  - Extension-based view architecture with hosted view tooling
  - VS Code chat UX integration and `.mna` project format

- **BIDS library** (`mne_bids`): New library for reading and writing BIDS-formatted datasets with testframe

- **Beamformers**: Added beamformer classes to the `inv` library with testframes

- **DSP library enhancements**
  - `WelchPsd`: Welch power spectral density estimation
  - `MorletTfr`: Morlet wavelet time-frequency representation
  - `xDAWN`: xDAWN spatial filtering for ERP denoising
  - KMeans: Cosine and correlation distance normalization implemented
  - Real-time covariance: Channel picks applied (MEG/EEG only)

- **Logging**: Renamed `ApplicationLogger` to `MNELogger` with `MNE_LOG_LEVEL` and `MNE_LOG_MODE` environment variables

### CI/CD

- Qt 6.11 support with auto-detection in `init.sh`
- Build Qt Installer Framework 4.11.0 toolchains from source
- Offline self-contained installers (.run, .dmg, .exe)
- Nightly WASM builds deployed alongside stable releases
- Reusable Qt toolchain assets consumed by CI workflows
- Eigen downloaded at configure time (no longer bundled)

### Bug Fixes

- Fixed Metal/QRhi rendering on macOS: native window, infinite repaint loop
- Fixed butterfly whitening scale and circular channel arrangement
- Fixed 2D layout channel filtering, scale persistence, and item sizing
- Fixed crosshair repaint cascade, split MEG grad/mag scale labels
- Fixed DSP SSS and bad-channel regressions
- Fixed DSP pi constants on Windows, params defaults on Clang/GCC
- Fixed Windows linker error for nested `Params` struct exports
- Fixed macOS test flakiness and WASM build errors
- Fixed `mne_anonymize` unique_ptr usage
- Fixed ruler amplitude precision (1 → 3 decimal places)
- Fixed stim chip overlap, event line z-order, and resize resampling
- Fixed scroll pan boundary and smooth inertial scroll

### Documentation

- Updated website to reflect renamed/split libraries
- Updated build guide paths (`tools/` → `scripts/`)
- Fixed release download page for installers and WebAssembly
- Updated Zenodo DOI to 19139238

## [2.0.0] - 2026-03-20

### Versioning

- **MNE-CPP libraries**: Version 2.0.0
- **CLI tools** (51 ported MNE-C tools, `mne_dipole_fit`, `mne_inspect`): Derive version from the MNE-CPP project version (2.0.0) via CMake compile definition
- **MNE Scan**: Version 1.0.0 (first stable release as a standalone application)
- **MNE Analyze**: Version 1.0.0 (first stable release as a standalone application)
- **IConnector plugin interface**: Bumped to `mne_rt_server/2.0`

### Breaking Changes

- **Library renames**: `connectivity` → `conn`, `communication` → `com`, `rtprocessing` → `dsp`, `inverse` → `inv`, `disp3D_rhi` → `disp3D`
- **Class renames**: Inverse library classes now use `Inv` prefix (`InvRapMusic`, `InvMinimumNorm`, `InvSourceEstimate`, `InvHpiFit`, …)
- **Moved classes**: `InvInverseOperator`, `InvMeasData`, `InvMeasDataSet` moved from `inv` to `mne` library (as `MNEInverseOperator`, `MNEMeasData`, `MNEMeasDataSet`); backward-compatible forwarding headers provided
- **Moved classes**: `FwdForwardSolution` moved to `MNEForwardSolution` in the `mne` library; forwarding header in `fwd/` for backward compatibility
- **Removed**: `InvCorSourceEstimate` class (superseded by unified `InvSourceEstimate`)
- **Split**: `MNEMath` split into `Linalg` and `Numerics` in the `math` library
- **Moved**: `events` library moved into `mne_analyze` application (no longer a standalone library)
- **Qt requirement**: Qt 6.10.0 minimum (Qt 5 no longer supported)
- **Eigen upgrade**: Eigen 5.0.1

### New Features

- **51 CLI tools**: Ported all MNE-C command-line tools to modern C++ with `QCommandLineParser` and `ApplicationLogger`
  - Conversion: `mne_edf2fiff`, `mne_ctf2fiff`, `mne_kit2fiff`, `mne_brain_vision2fiff`, and more
  - Forward modeling: `mne_forward_solution`, `mne_setup_forward_model`, `mne_prepare_bem_model`, `mne_make_source_space`, `mne_make_sphere_bem`, `mne_average_forward_solutions`
  - Inverse: `mne_compute_raw_inverse`, `mne_compute_mne`, `mne_inverse_operator`
  - Preprocessing: `mne_anonymize`, `mne_process_raw`, `mne_compensate_data`
  - Surface: `mne_surf2bem`, `mne_check_surface`, `mne_watershed_bem`, `mne_flash_bem`
  - Info: `mne_show_fiff`, `mne_compare_fif_files`, `mne_list_source_space`, `mne_list_bem`
  - Server: `mne_rt_server`
  - Simulation: `mne_simulate_data`
- **Unified InvSourceEstimate**: Single class with composition layers for grid data, focal dipoles, couplings, connectivity, positions, and metadata; includes tokenization support for streaming
- **RHI-based 3D rendering**: Migrated from Qt3D to QRhi pipeline with holographic rendering mode, Fresnel effects, dipole overlay, and async STC loading
- **Flexible clustering**: Variable cluster sizes in source space computation
- **MNE Scan improvements**: Plugin loading progress on splash screen, macOS app bundle icons
- **LSL library modernized**: Added `lsl_` prefix, comprehensive tests, updated to LSL 1.17.5

### Refactoring & Modernization

- **Forward library (fwd)**: Removed ~700 lines of dead code; replaced `printf` → `qInfo`/`qWarning`; replaced `goto` → early return/lambdas; replaced C macros (`ARSINH`, `EPS`) → `constexpr`/`inline`; modernized `FwdBemModel` members from raw pointers to Eigen types and `std::shared_ptr`
- **FIFF I/O consolidation**: Moved duplicated FIFF reading functions to library classes (`FiffStream::read_bad_channels`, `MNECovMatrix::read`, `MNERawInfo::find_meas_info`)
- **Thread safety**: Fixed thread-unsafe `static float*` in `MNEProjOp::project_vector` using `thread_local`
- **Network input validation**: Added bounds checking in BabyMEG client byte conversion functions
- **Sparse matrix crash fix**: Guarded `MNEHemisphere::operator==` against segfault on default-constructed sparse matrices
- **Copyright**: Updated to 2026 across all library headers
- **Qt6 serialization**: Updated `QDataStream` version from `Qt_5_1` to `Qt_6_0` in rt_server
- **Centralized CLI versioning**: All 51 CLI tools now derive their `PROGRAM_VERSION` from the CMake project version (`MNE_CPP_VERSION`) instead of hardcoded strings
- **Format specifier fix**: Fixed `%d` → `%lld` for `qsizetype` in BabyMEG client warnings

### CI/CD

- Restructured to Main-Staging-Feature branch model
- Build matrix: Ubuntu 24.04, macOS 26, Windows 2025 with Qt 6.10.0 and Qt 6.10.2
- CodeQL and Coverity static analysis (weekly)
- Code coverage tracking with 35% library threshold
- CPackIFW-based installer generation (.run, .dmg, .exe)
- WebAssembly build support

### Documentation

- Docusaurus-based documentation site
- Organized CLI tools by domain category in sidebar
- Added MNE handbook reference pages (cookbook, FIFF format, sample dataset, data conversion)
- Completed Doxygen documentation for `InvRapMusic`, `InvMinimumNorm`, `InvDipole`

### Bug Fixes

- Fixed crash in `make_inverse_operator` with non-surface-oriented forward solutions
- Fixed segfault in forward computation
- Fixed FIFF double I/O, `make_dir` EOF, `write_cov` packing, and `source_cov` kind
- Fixed `MNECTFCompDataSet` copy constructor member initialization
- Fixed coordinate transformation handling in 3D visualization
- Fixed inflated brain alignment and surface projection tolerances
- Fixed floating-point comparison tolerances in forward solution tests
- Fixed sphere simplex test convergence
